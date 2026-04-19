# Publish-Safe Scenario Defaults

`OpenClaw Zombie Director` should publish with conservative defaults.

This is especially important because the project is still experimental.

## Recommended published defaults

For the public Dead Everon scenario:

- `m_sConcreteProfileId = dead_everon_bacon_concrete`
- `m_RestBridge.m_bEnabled = false`
- `m_RestBridge.m_sProfileOverridePath = openclaw-zombie-director/rest-bridge.override.json`
- `m_bRunSmokeBootstrap = false`
- `m_bRelocateFirstPlayer = false`

## Why these are the safe defaults

- The server works without OpenClaw or any other external AI.
- Server admins can opt into OpenClaw later by uploading one profile-side override file.
- The scenario does not move players around for smoke testing on live servers.
- The scenario does not assume a bridge hostname or token at publish time.

## Current concrete Dead Everon world

The production world is:

- [DeadEveronDirector.ent](X:/OpenClaw/workspace/projects/reforger-zombie-director-dead-everon-overlay/Worlds/DeadEveronDirector/DeadEveronDirector.ent:1)

Its game mode layer now keeps the public-safe setting:

- [gamemode.layer](X:/OpenClaw/workspace/projects/reforger-zombie-director-dead-everon-overlay/Worlds/DeadEveronDirector/DeadEveronDirector_Layers/gamemode.layer:18)

## Operator opt-in path

If a server owner wants OpenClaw or another AI director:

1. keep the published scenario unchanged
2. generate `rest-bridge.profile-override.generated.json` from the control-host bundle
3. upload it as:
   `$profile:openclaw-zombie-director/rest-bridge.override.json`

That is the intended public deployment path.
