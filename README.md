# ReVoiceNext

[![GitHub license](https://img.shields.io/github/license/Mrbxx/ReVoiceNext.svg?longCache=true&style=flat-square)](https://github.com/Mrbxx/ReVoiceNext/blob/master/LICENSE)
[![GitHub issues](https://img.shields.io/github/issues/Mrbxx/ReVoiceNext.svg?longCache=true&style=flat-square)](https://github.com/Mrbxx/ReVoiceNext/issues)
[![GitHub stars](https://img.shields.io/github/stars/Mrbxx/ReVoiceNext.svg?longCache=true&style=flat-square)](https://github.com/Mrbxx/ReVoiceNext/stargazers)

A Metamod plugin for [ReHLDS](https://github.com/rehlds/ReHLDS) that enables voice chat between Steam (SILK/Opus) and non-Steam (Speex) clients, with full [ReAPI](https://github.com/rehlds/ReAPI) integration.

## Features

- **Voice transcoding** — Steam clients (SILK/Opus) and non-Steam clients (Speex) can talk with each other transparently
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

## Fixes

- [**CS 1.6 / Steam crash on Linux when a non-Steam client speaks**](https://github.com/ValveSoftware/halflife/issues/3898) — when a non-Steam player (Speex) talks, the server forwards raw Speex data to Steam clients; Linux Steam clients crash on receiving a codec they don't expect. ReVoiceNext transcodes Speex → Opus in real time so Steam clients always receive the correct codec.
- [**Voice flood vulnerability**](https://github.com/rehlds/ReVoice/issues/30) — an attacker could flood the server with voice packets, causing lag and kicking all players. ReVoiceNext fixes the rate limiter so flood packets are blocked before they can overflow the network buffer.

## Based on

- **[ReVoice](https://github.com/rehlds/ReVoice)** by ReHLDS Team — latest master (`9db23f1`, 2025-04-05)
- **[revoice-plus](https://github.com/Garey27/revoice-plus)** by Garey27 — latest master (`f286851`, 2023-03-13)

## License

[GNU General Public License v3](LICENSE)
