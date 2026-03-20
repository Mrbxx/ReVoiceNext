#include "precompiled.h"

void SV_DropClient_hook(IRehldsHook_SV_DropClient *chain, IGameClient *cl, bool crash, const char *msg)
{
	CRevoicePlayer *plr = GetPlayerByClientPtr(cl);

	plr->OnDisconnected();

	chain->callNext(cl, crash, msg);
}

void CvarValue2_PreHook(const edict_t *pEnt, int requestID, const char *cvarName, const char *cvarValue)
{
	CRevoicePlayer *plr = GetPlayerByEdict(pEnt);
	if (plr->GetRequestId() != requestID) {
		RETURN_META(MRES_IGNORED);
	}

	const char *lastSeparator = strrchr(cvarValue, ',');
	if (lastSeparator)
	{
		int buildNumber = atoi(lastSeparator + 1);
		if (buildNumber > 4554) {
			plr->SetCodecType(vct_opus);
		}
	}

	RETURN_META(MRES_IGNORED);
}

static int ResamplePCM(const int16_t *in, int inSamples, int srcRate, int16_t *out, int outMax, int dstRate)
{
	if (srcRate == dstRate) {
		int n = inSamples < outMax ? inSamples : outMax;
		memcpy(out, in, n * sizeof(int16_t));
		return n;
	}

	int outSamples = (int)((int64_t)inSamples * dstRate / srcRate);
	if (outSamples > outMax)
		outSamples = outMax;

	for (int i = 0; i < outSamples; i++) {
		int64_t pos = (int64_t)i * srcRate;
		int idx = (int)(pos / dstRate);
		int frac = (int)(pos % dstRate);
		int16_t a = (idx < inSamples) ? in[idx] : 0;
		int16_t b = (idx + 1 < inSamples) ? in[idx + 1] : 0;
		out[i] = (int16_t)(a + (int32_t)(b - a) * frac / dstRate);
	}

	return outSamples;
}

int TranscodeVoice(CRevoicePlayer *srcPlayer, const char *srcBuf, int srcBufLen, IVoiceCodec *srcCodec, IVoiceCodec *dstCodec, char *dstBuf, int dstBufSize)
{
	static char decodedBuf[65536];
	static int16_t resampledBuf[32768];

	int numDecodedSamples = srcCodec->Decompress(srcBuf, srcBufLen, decodedBuf, sizeof(decodedBuf));
	if (numDecodedSamples <= 0)
		return 0;

	const char *encodeBuf = decodedBuf;
	int encodeSamples = numDecodedSamples;

	int srcRate = srcCodec->SampleRate();
	int dstRate = dstCodec->SampleRate();

	if (srcRate != dstRate) {
		encodeSamples = ResamplePCM((const int16_t *)decodedBuf, numDecodedSamples, srcRate, resampledBuf, (int)(sizeof(resampledBuf) / sizeof(int16_t)), dstRate);
		if (encodeSamples <= 0)
			return 0;
		encodeBuf = (const char *)resampledBuf;
	}

	int compressedSize = dstCodec->Compress(encodeBuf, encodeSamples, dstBuf, dstBufSize, false);
	if (compressedSize <= 0)
		return 0;

	return compressedSize;
}

void SV_ParseVoiceData_emu(IGameClient *cl)
{
	char chReceived[4096];
	unsigned int nDataLength = g_RehldsFuncs->MSG_ReadShort();

	if (nDataLength > sizeof(chReceived)) {
		g_RehldsFuncs->DropClient(cl, FALSE, "Invalid voice data\n");
		return;
	}

	g_RehldsFuncs->MSG_ReadBuf(nDataLength, chReceived);

	if (g_pcv_sv_voiceenable->value == 0.0f) {
		return;
	}

	CRevoicePlayer *srcPlayer = GetPlayerByClientPtr(cl);

	if (srcPlayer->IsBlocked())
		return;

	if (!srcPlayer->IsSpeaking())
		g_OnClientStartSpeak(cl->GetId() + 1);

	srcPlayer->Speak();
	srcPlayer->SetLastVoiceTime(g_RehldsSv->GetTime());
	srcPlayer->IncreaseVoiceRate(nDataLength);

	if (srcPlayer->IsMuted())
		return;

	// Two separate output buffers: Opus for Steam clients, Speex for non-Steam
	static char opusBuf[4096];
	static char speexBuf[4096];

	char *opusData = nullptr;
	char *speexData = nullptr;
	int opusDataLen = 0;
	int speexDataLen = 0;

	switch (srcPlayer->GetCodecType())
	{
	case vct_silk:
	{
		if (nDataLength > MAX_SILK_DATA_LEN || srcPlayer->GetVoiceRate() > MAX_SILK_VOICE_RATE)
			return;

		opusDataLen = TranscodeVoice(srcPlayer, chReceived, nDataLength, srcPlayer->GetSilkCodec(), srcPlayer->GetOpusCodec(), opusBuf, sizeof(opusBuf));
		speexDataLen = TranscodeVoice(srcPlayer, chReceived, nDataLength, srcPlayer->GetSilkCodec(), srcPlayer->GetSpeexCodec(), speexBuf, sizeof(speexBuf));
		opusData = opusBuf;
		speexData = speexBuf;
		break;
	}
	case vct_opus:
	{
		if (nDataLength > MAX_OPUS_DATA_LEN || srcPlayer->GetVoiceRate() > MAX_OPUS_VOICE_RATE)
			return;

		// Pass Opus through for Steam clients, transcode to Speex for non-Steam
		opusData = chReceived;
		opusDataLen = nDataLength;
		speexDataLen = TranscodeVoice(srcPlayer, chReceived, nDataLength, srcPlayer->GetOpusCodec(), srcPlayer->GetSpeexCodec(), speexBuf, sizeof(speexBuf));
		if (speexDataLen > 0)
			speexData = speexBuf;
		break;
	}
	case vct_speex:
	{
		if (nDataLength > MAX_SPEEX_DATA_LEN || srcPlayer->GetVoiceRate() > MAX_SPEEX_VOICE_RATE)
			return;

		// Pass Speex through for non-Steam clients, transcode to Opus for Steam
		speexData = chReceived;
		speexDataLen = nDataLength;
		opusDataLen = TranscodeVoice(srcPlayer, chReceived, nDataLength, srcPlayer->GetSpeexCodec(), srcPlayer->GetOpusCodec(), opusBuf, sizeof(opusBuf));
		if (opusDataLen > 0)
			opusData = opusBuf;
		break;
	}
	default:
		return;
	}

	int maxclients = g_RehldsSvs->GetMaxClients();
	for (int i = 0; i < maxclients; i++)
	{
		CRevoicePlayer *dstPlayer = &g_Players[i];
		IGameClient *dstClient = dstPlayer->GetClient();

		if (!((1 << i) & cl->GetVoiceStream(0)) && dstPlayer != srcPlayer)
			continue;

		if (!dstClient->IsActive() || !dstClient->IsConnected())
			continue;

		if (dstPlayer == srcPlayer && !dstClient->GetLoopback())
			continue;

		char *sendBuf;
		int nSendLen;
		switch (dstPlayer->GetCodecType())
		{
		case vct_silk:
		case vct_opus:
			sendBuf = opusData;
			nSendLen = opusDataLen;
			break;
		case vct_speex:
			sendBuf = speexData;
			nSendLen = speexDataLen;
			break;
		default:
			sendBuf = nullptr;
			nSendLen = 0;
			break;
		}

		if (sendBuf == nullptr || nSendLen == 0)
			continue;

		sizebuf_t *dstDatagram = dstClient->GetDatagram();
		if (dstDatagram->cursize + nSendLen + 4 < dstDatagram->maxsize) {
			g_RehldsFuncs->MSG_WriteByte(dstDatagram, svc_voicedata);
			g_RehldsFuncs->MSG_WriteByte(dstDatagram, cl->GetId());
			g_RehldsFuncs->MSG_WriteShort(dstDatagram, nSendLen);
			g_RehldsFuncs->MSG_WriteBuf(dstDatagram, nSendLen, sendBuf);
		}
	}
}

void StartFrame_PostHook()
{
	int maxclients = g_RehldsSvs->GetMaxClients();
	for (int i = 0; i < maxclients; i++) {
		CRevoicePlayer *player = &g_Players[i];
		IGameClient *client = player->GetClient();

		if (!client->IsActive() || !client->IsConnected())
			continue;

		if (!player->CheckSpeaking() && player->IsSpeaking()) {
			player->SpeakDone();
			g_OnClientStopSpeak(client->GetId() + 1);
		}
	}

	SET_META_RESULT(MRES_IGNORED);
}

void Rehlds_HandleNetCommand(IRehldsHook_HandleNetCommand *chain, IGameClient *cl, int8 opcode)
{
	const int clc_voicedata = 8;
	if (opcode == clc_voicedata) {
		SV_ParseVoiceData_emu(cl);
		return;
	}

	chain->callNext(cl, opcode);
}

qboolean ClientConnect_PreHook(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128])
{
	CRevoicePlayer *plr = GetPlayerByEdict(pEntity);
	plr->OnConnected();

	RETURN_META_VALUE(MRES_IGNORED, TRUE);
}

void ServerActivate_PostHook(edict_t *pEdictList, int edictCount, int clientMax)
{
	MESSAGE_BEGIN(MSG_INIT, svc_stufftext);
	WRITE_STRING("VTC_CheckStart\n");
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_INIT, svc_stufftext);
	WRITE_STRING("vgui_runscript\n");
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_INIT, svc_stufftext);
	WRITE_STRING("VTC_CheckEnd\n");
	MESSAGE_END();

	Revoice_Exec_Config();
	SET_META_RESULT(MRES_IGNORED);
}

void OnClientCommandReceiving(edict_t *pClient)
{
	CRevoicePlayer *plr = GetPlayerByEdict(pClient);
	const char *command = CMD_ARGV(0);

	if (FStrEq(command, "VTC_CheckStart")) {
		plr->SetCheckingState(1);
		plr->SetCodecType(vct_speex);
		RETURN_META(MRES_SUPERCEDE);
	}
	else if (plr->GetCheckingState()) {
		if (FStrEq(command, "vgui_runscript")) {
			plr->SetCheckingState(2);
			RETURN_META(MRES_SUPERCEDE);
		}
		else if (FStrEq(command, "VTC_CheckEnd")) {
			plr->SetCodecType(plr->GetCheckingState() == 2 ? vct_opus : vct_speex);
			plr->SetCheckingState(0);
			RETURN_META(MRES_SUPERCEDE);
		}
	}

	RETURN_META(MRES_IGNORED);
}

void SV_WriteVoiceCodec_hooked(IRehldsHook_SV_WriteVoiceCodec *chain, sizebuf_t *sb)
{
	IGameClient *cl = g_RehldsFuncs->GetHostClient();
	CRevoicePlayer *plr = GetPlayerByClientPtr(cl);

	switch (plr->GetCodecType())
	{
		case vct_silk:
		case vct_opus:
		case vct_speex:
		{
			g_RehldsFuncs->MSG_WriteByte(sb, svc_voiceinit);
			g_RehldsFuncs->MSG_WriteString(sb, "voice_speex");	// codec id
			g_RehldsFuncs->MSG_WriteByte(sb, 5);				// quality
			break;
		}
		default:
			LCPrintf(true, "SV_WriteVoiceCodec() called on client(%d) with unknown voice codec\n", cl->GetId());
			break;
	}
}

bool Revoice_Load()
{
	if (!Revoice_Utils_Init())
		return false;

	if (!Revoice_RehldsApi_Init()) {
		LCPrintf(true, "Failed to locate REHLDS API\n");
		return false;
	}

	if (!Revoice_ReunionApi_Init())
		return false;

	Revoice_Init_Cvars();
	Revoice_Init_Config();
	Revoice_Init_Players();

	if (!Revoice_Main_Init()) {
		LCPrintf(true, "Initialization failed\n");
		return false;
	}

	g_RehldsApi->GetFuncs()->RegisterPluginApi("VoiceTranscoder", &g_voiceTranscoderAPI);

	return true;
}

bool Revoice_Main_Init()
{
	g_RehldsHookchains->SV_DropClient()->registerHook(&SV_DropClient_hook, HC_PRIORITY_DEFAULT + 1);
	g_RehldsHookchains->HandleNetCommand()->registerHook(&Rehlds_HandleNetCommand, HC_PRIORITY_DEFAULT + 1);
	g_RehldsHookchains->SV_WriteVoiceCodec()->registerHook(&SV_WriteVoiceCodec_hooked, HC_PRIORITY_DEFAULT + 1);

	return true;
}

void Revoice_Main_DeInit()
{
	g_RehldsHookchains->SV_DropClient()->unregisterHook(&SV_DropClient_hook);
	g_RehldsHookchains->HandleNetCommand()->unregisterHook(&Rehlds_HandleNetCommand);
	g_RehldsHookchains->SV_WriteVoiceCodec()->unregisterHook(&SV_WriteVoiceCodec_hooked);

	Revoice_DeInit_Cvars();
}
