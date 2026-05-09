# ReVoiceNext

[![GitHub license](https://img.shields.io/github/license/Mrbxx/ReVoiceNext.svg?longCache=true&style=flat-square)](https://github.com/Mrbxx/ReVoiceNext/blob/master/LICENSE)
[![GitHub issues](https://img.shields.io/github/issues/Mrbxx/ReVoiceNext.svg?longCache=true&style=flat-square)](https://github.com/Mrbxx/ReVoiceNext/issues)
[![GitHub stars](https://img.shields.io/github/stars/Mrbxx/ReVoiceNext.svg?longCache=true&style=flat-square)](https://github.com/Mrbxx/ReVoiceNext/stargazers)

A Metamod plugin for [ReHLDS](https://github.com/rehlds/ReHLDS) that enables voice chat between Steam (SILK/opus) and nonsteam (speex) clients, with full [ReAPI](https://github.com/rehlds/ReAPI) integration.

## Features

- **Voice transcoding** — Steam clients (SILK/opus) and nonsteam clients (speex) can talk with each other transparently
- **Codec auto-detection** — detects client codec support via the VTC_Check mechanism
- **Speaking detection** — fires events when a player starts or stops talking
- **Mute** — mute players (voice not forwarded)
- **ReAPI integration** — exposes `IVoiceTranscoderAPI` via ReHLDS `RegisterPluginApi`; use [ReAPI](https://github.com/rehlds/ReAPI) for scripting support

## ReAPI scripting

```pawn
#include <reapi>

public VTC_OnClientStartSpeak(const index) { /* player started talking */ }
public VTC_OnClientStopSpeak(const index)  { /* player stopped talking */ }

public func(index)
{
    VTC_MuteClient(index);
    VTC_UnmuteClient(index);
    VTC_IsClientMuted(index);
    VTC_IsClientSpeaking(index);
}
```

## Fixes vs original ReVoice

- [**CS 1.6 / Steam crash on Linux**](https://github.com/ValveSoftware/halflife/issues/3898) - transcodes speex to opus so Steam clients don't crash when a nonsteam player talks.
- [**Voice flood**](https://github.com/rehlds/ReVoice/issues/30) - rate limiter now actually drops flood packets before they lag out the server.
- **Server hang on malformed packet** - deprecated PLT_Silk/PLT_OPUS opcodes caused StreamDecode to loop forever, pinning a CPU core until the server was killed.
- **Rate limiter bypass** - m_VoiceRate could wrap past INT_MAX after ~3 min of flooding, disabling the check entirley.
- **Null pointer crash in voice codec hook** - GetHostClient() return value wasn't checked before being passed to GetPlayerByClientPtr.
- **util_syserror broken with LTO** - the original used a null pointer write to force a crash dump, but -flto legally optimizes the whole block away including exit(). Now uses abort().
- **UB in voice stream mask** - 1 << 31 on a signed int is undefined behaviour. Changed to 1U.
- **realloc leak** - CUtlMemory lost the original pointer when realloc failed, leaking the allocation.
- **Garbled audio after packet loss** - opus_decode failures during PLC weren't advancing pWritePos so the next frame overwrote the same postion. Now writes silence and moves on.
- **vsprintf overflow in UTIL_VarArgs** - replaced with vsnprintf.
- **PlaySound() stub** - logs a one-time warning instead of silently doing nothing. Maybe I'll remove it someday, no idea why anyone added it in the first place ;)

## Based on

- **[ReVoice](https://github.com/rehlds/ReVoice)** by ReHLDS Team — latest master (`9db23f1`, 2025-04-05)
- **[revoice-plus](https://github.com/Garey27/revoice-plus)** by Garey27 — latest master (`f286851`, 2023-03-13)

## License

[GNU General Public License v3](LICENSE)
