#include "precompiled.h"

Event<size_t> g_OnClientStartSpeak;
Event<size_t> g_OnClientStopSpeak;
VoiceTranscoderAPI g_voiceTranscoderAPI;

size_t VoiceTranscoderAPI::GetMajorVersion()
{
	return VOICETRANSCODER_API_VERSION_MAJOR;
}

size_t VoiceTranscoderAPI::GetMinorVersion()
{
	return VOICETRANSCODER_API_VERSION_MINOR;
}

bool VoiceTranscoderAPI::IsClientSpeaking(size_t clientIndex)
{
	if (clientIndex < 1 || clientIndex > (size_t)g_RehldsSvs->GetMaxClients())
		return false;
	return g_Players[clientIndex - 1].IsSpeaking();
}

IEvent<size_t>& VoiceTranscoderAPI::OnClientStartSpeak()
{
	return g_OnClientStartSpeak;
}

IEvent<size_t>& VoiceTranscoderAPI::OnClientStopSpeak()
{
	return g_OnClientStopSpeak;
}

void VoiceTranscoderAPI::MuteClient(size_t clientIndex)
{
	if (clientIndex < 1 || clientIndex > (size_t)g_RehldsSvs->GetMaxClients())
		return;
	g_Players[clientIndex - 1].Mute();
}

void VoiceTranscoderAPI::UnmuteClient(size_t clientIndex)
{
	if (clientIndex < 1 || clientIndex > (size_t)g_RehldsSvs->GetMaxClients())
		return;
	g_Players[clientIndex - 1].UnMute();
}

bool VoiceTranscoderAPI::IsClientMuted(size_t clientIndex)
{
	if (clientIndex < 1 || clientIndex > (size_t)g_RehldsSvs->GetMaxClients())
		return false;
	return g_Players[clientIndex - 1].IsMuted();
}

void VoiceTranscoderAPI::PlaySound(size_t /*receiverClientIndex*/, const char* /*soundFilePath*/)
{
	// Not implemented
}

void VoiceTranscoderAPI::BlockClient(size_t clientIndex)
{
	if (clientIndex < 1 || clientIndex > (size_t)g_RehldsSvs->GetMaxClients())
		return;
	g_Players[clientIndex - 1].Block();
}

void VoiceTranscoderAPI::UnblockClient(size_t clientIndex)
{
	if (clientIndex < 1 || clientIndex > (size_t)g_RehldsSvs->GetMaxClients())
		return;
	g_Players[clientIndex - 1].Unblock();
}

bool VoiceTranscoderAPI::IsClientBlocked(size_t clientIndex)
{
	if (clientIndex < 1 || clientIndex > (size_t)g_RehldsSvs->GetMaxClients())
		return false;
	return g_Players[clientIndex - 1].IsBlocked();
}
