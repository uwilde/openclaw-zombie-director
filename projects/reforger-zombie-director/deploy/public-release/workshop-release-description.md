# Workshop And Release Description

Use the text below as the copy-ready public description.

## Workshop Title

`OpenClaw Zombie Director (Experimental)`

## Workshop Short Description

Experimental zombie director mod for Arma Reforger. Works on local and dedicated servers, with or without OpenClaw or another external AI.

## Workshop Description

```md
## Experimental

OpenClaw Zombie Director is **experimental**.

It is already usable, but it is still being hardened for public deployment. Expect balance changes, compatibility fixes, and setup improvements over time.

## What It Does

OpenClaw Zombie Director is a zombie-focused encounter director for Arma Reforger.

It can:

- spawn and pressure players based on location and movement
- reinforce contested areas
- trigger sweeps, spikes, and horde-style pressure
- run with no external AI at all
- optionally accept high-level direction from OpenClaw or another AI through a REST bridge

## Supported Server Styles

- local hosted servers
- self-managed dedicated servers
- cloud/dedicated hosts
- hosted panels such as Nitrado, GPORTAL, GTX-style setups, and similar providers that allow normal mod/scenario install plus config/profile file upload

## OpenClaw Is Optional

This mod does **not** require OpenClaw.

You can run it:

- standalone
- with Reforger-side hints only
- with OpenClaw
- with another external director service using the same bridge contract

## Current Public Stack

The current concrete authored stack is centered on:

- Dead Everon
- Bacon Zombies

## Important Limitation

On true headless servers, the supported OpenClaw path is the direct-hook REST bridge path.
Desktop vision and Game Master UI clicking are **not** the intended cloud-server workflow.

## Basic Install

### If you do not want OpenClaw

1. Install the scenario and required addons.
2. Leave the bridge disabled.
3. Start the server.

### If you do want OpenClaw

1. Set up the control-host bundle.
2. Run `install-control-host.ps1`.
3. Upload the generated server override file to:
   `<server profile>/openclaw-zombie-director/rest-bridge.override.json`
4. Restart the server.

## Notes

- Regular players do not need any bridge hostname or token.
- Only the server operator does.
- This project is still **experimental**.
```

## Release Page Description

```md
## OpenClaw Zombie Director (Experimental)

This release packages the current public-use build of OpenClaw Zombie Director.

### Experimental Status

This project is still **experimental**.

It works, but the public deployment flow is still being refined. Expect follow-up fixes for compatibility, balance, hosting workflows, and packaging.

### Included

- public quick-start docs
- hosted-panel quick-start docs
- Windows control-host bundle
- Linux dedicated-server bundle

### Use Without OpenClaw

You can install the scenario/addons and run the mod with no external AI.

### Use With OpenClaw

Run the control-host bundle, generate the server override file, upload it to the server profile, and restart the server.

### Server Admin Install

1. Install the required scenario/addon stack.
2. If running without OpenClaw, leave the bridge disabled.
3. If running with OpenClaw, run:

   `powershell -ExecutionPolicy Bypass -File ".\install-control-host.ps1"`

4. Upload:

   `server/rest-bridge.profile-override.generated.json`

   to:

   `<server profile>/openclaw-zombie-director/rest-bridge.override.json`

5. Restart the game server.
```

## Admin Install Steps

Use these steps exactly in release notes, readmes, or pinned server-admin comments:

```text
OpenClaw Zombie Director is experimental.

Without OpenClaw:
1. Install the scenario and required addons.
2. Leave the bridge disabled.
3. Start or restart the server.

With OpenClaw:
1. Open the control-host bundle.
2. Run install-control-host.ps1 on the control machine.
3. Upload server/rest-bridge.profile-override.generated.json to:
   <server profile>/openclaw-zombie-director/rest-bridge.override.json
4. Start or restart the server.

Regular players do not need the bridge hostname or token.
Only the server operator does.
```
