# START HERE

`OpenClaw Zombie Director` is **experimental**.

Expect rough edges, balancing changes, and workflow changes while the mod is still being hardened for public use.

## What this is

`OpenClaw Zombie Director` is a zombie-focused Arma Reforger director mod that can run:

- on local servers
- on dedicated/cloud servers
- with no AI at all
- with OpenClaw
- with another external AI through the same REST bridge

## Current supported public stack

This release is centered on one experimental public stack:

- Dead Everon
- Bacon Zombies
- OpenClaw Zombie Director
- OpenClaw Zombie Director Dead Everon

## If you are just joining a server

You do not need OpenClaw.
You do not need a bridge token.
You do not need to configure anything.

You only need the server to be running the mod stack correctly.

## If you are running a server without OpenClaw

1. Install the published scenario and required addons.
2. Leave the bridge disabled.
3. Start the server.

That is the safest setup right now.

## If you are running a server with OpenClaw

1. Open one of the bundles in `bundles/`.
2. On the control host, run:

```powershell
powershell -ExecutionPolicy Bypass -File ".\\install-control-host.ps1"
```

3. Take the generated file:
   `server/rest-bridge.profile-override.generated.json`
4. Upload it to the game server as:
   `<server profile>/openclaw-zombie-director/rest-bridge.override.json`
5. Restart the game server.

## Read next

- `SUPPORTED.md`
- `PUBLIC STACK.md`
- `docs/server-operator-quick-start.md`
- `docs/hosting-panel-quick-start.md`
- `workshop-release-description.md`

## Important note

This project is still **experimental**.
Treat it as a serious prototype, not a final polished release.
