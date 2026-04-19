# Server Operator Quick Start

This is the short path for people who want to run `OpenClaw Zombie Director` on a local or dedicated server.

`OpenClaw Zombie Director` is **experimental**.

## Pick one mode

- `Without OpenClaw`: install the scenario and dependencies, leave the bridge disabled, and start the server.
- `With OpenClaw or another AI`: do the same, then upload one profile-side override file.

## 1. Install the required addons

For the current concrete Dead Everon + Bacon stack, the addon chain is listed in:

- [addons-required-dead-everon-bacon.txt](../deploy/cloud/windows-dedicated-remote-openclaw/server/addons-required-dead-everon-bacon.txt)

## 2. Use a scenario built with these defaults

The scenario should keep:

- `m_sConcreteProfileId = dead_everon_bacon_concrete`
- `m_RestBridge.m_bEnabled = 0`
- `m_RestBridge.m_sProfileOverridePath = openclaw-zombie-director/rest-bridge.override.json`

That makes the scenario safe for servers that do not want any external AI.

## 3. If you do not want OpenClaw

Start the server normally. No extra bridge setup is required.

## 4. If you do want OpenClaw

Run one of the control-host bundles:

- [Windows control-host bundle](../deploy/cloud/dist/openclaw-zombie-director-cloud-windows-dedicated-remote-openclaw.zip)
- [Linux server bundle](../deploy/cloud/dist/openclaw-zombie-director-cloud-linux-dedicated-remote-openclaw.zip)

On the control host, run:

```powershell
powershell -ExecutionPolicy Bypass -File ".\install-control-host.ps1"
```

That generates:

- the bridge config for the control host
- the OpenClaw session prompt
- the server override file to upload

Upload the generated file to:

- `<server profile>/openclaw-zombie-director/rest-bridge.override.json`

## 5. Start or restart the game server

Once the override file is present, the mod will load it on server startup and enable the external bridge path.

## Notes

- Regular players do not need the hostname or token.
- Only the server operator does.
- OpenClaw is optional. Any other service can use the same REST bridge contract.
- Headless servers should use the direct-hook bridge path, not desktop vision or UI clicking.
- This project is still **experimental**.
