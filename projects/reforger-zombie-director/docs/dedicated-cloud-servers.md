# Dedicated and Cloud Servers

`OpenClaw Zombie Director` can run on dedicated and cloud-hosted Reforger servers.
The important distinction is between:

- the **director mod**, which is server-authoritative and works headless
- the **OpenClaw desktop-control path**, which does not apply to a headless server

## What works

- Native director logic running on the Reforger server
- Native runtime spawning and sweep waypoint attachment without `SCR_PlacingEditorComponent` or `SCR_EditableEntityComponent`
- Reforger-side hints via `OCD_ZombieDirectorAPI`
- REST bridge polling from the server to a local or remote external director
- OpenClaw running on another machine and pushing high-level hints through the bridge

## What does not

- OpenClaw watching a cloud server desktop and clicking the Game Master UI on a true headless host
- Any workflow that depends on a visible Windows desktop when the server has none

For dedicated/cloud use, the correct pattern is:

1. the server runs the director addon
2. the server polls the bridge
3. OpenClaw or another AI pushes hints to that bridge
4. the mod performs the actual in-game spawning

## Deployment patterns

### 1. Standalone dedicated server

Use this when you want no external AI service at all.

- Install the director addon, map addon, and zombie-pack addon on the server.
- Keep `m_RestBridge.m_bEnabled = false`.
- Use only local zone scoring plus scenario-side hints.

### 2. Same-host bridge

Use this when the Reforger server and the bridge run on the same Windows VM or box.

- Upload `$profile:openclaw-zombie-director/rest-bridge.override.json`
- Keep `baseUrl = http://127.0.0.1:18890/` in that file
- Keep the bridge bound to loopback only.
- Run OpenClaw or the bridge worker on that same host.

### 3. Remote bridge

Use this when the Reforger server is in the cloud and OpenClaw runs elsewhere.

- Upload `$profile:openclaw-zombie-director/rest-bridge.override.json`
- Set `baseUrl` to a reachable remote bridge such as `https://director.example.com/`
- Set `token`
- Ensure the Reforger server can make outbound requests to that host
- Put the bridge behind TLS and proper auth

Example bridge settings:

```json
{
  "kind": "openclaw-zombie-director.rest-bridge-override",
  "version": 1,
  "enabled": 1,
  "baseUrl": "https://director.example.com/",
  "hintRoute": "reforger/zombie-director/hints",
  "snapshotRoute": "reforger/zombie-director/snapshot",
  "token": "replace-me",
  "pollIntervalSeconds": 0.75,
  "postSnapshots": 1
}
```

## Server checklist

- The server has the same addon dependency chain as the scenario you built against.
- The map addon and zombie asset pack are installed on the server.
- The game mode uses `OCD_ZombieDirectorGameModeComponent`.
- The zone/template setup exists in the mission or runtime bootstrap you expect.
- The game mode keeps `m_RestBridge.m_sProfileOverridePath = openclaw-zombie-director/rest-bridge.override.json`.
- The server can resolve and reach the configured bridge hostname.
- The bridge token matches on both sides.
- If you are using a rented host, the server owner can upload the override file through the provider file browser or FTP instead of republishing the mission.

## Notes for this repo

- The core addon at `workspace/projects/reforger-zombie-director` is map-agnostic.
- The Dead Everon overlay addon at `workspace/projects/reforger-zombie-director-dead-everon-overlay` is for that specific authored stack.
- Both `addon.gproj` files include a `HEADLESS` configuration.
- The remaining Bacon warning noise is mostly a Workbench and packed-addon issue, not a headless-server blocker. See [bacon-compatibility-notes.md](bacon-compatibility-notes.md).

## Practical recommendation

For public or rented servers, keep the game server dumb and stable:

- run the mod on the server
- run OpenClaw and the bridge on a separate control machine or service
- let the server consume high-level hints only

That is the path that scales beyond a local Workbench or desktop session.
