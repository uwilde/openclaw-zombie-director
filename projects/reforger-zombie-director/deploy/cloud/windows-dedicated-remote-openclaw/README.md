# Cloud Deployment Bundle

This bundle targets one concrete production layout:

- `arma-reforger-gm01`: a Windows cloud VM running the Arma Reforger dedicated server
- `openclaw-control01`: a separate Windows control host running the director bridge and OpenClaw
- `director.example.com`: an HTTPS endpoint on the control host that reverse-proxies to the local bridge

This is the layout that keeps the game server stable and headless while still letting OpenClaw act as the pacing director.

## How the pieces connect

1. The Reforger server runs the `OpenClaw Zombie Director` mod.
2. The mod polls `https://director.example.com/reforger/zombie-director/hints`.
3. The mod posts snapshots back to `https://director.example.com/reforger/zombie-director/snapshot`.
4. OpenClaw runs on `openclaw-control01` and talks to the same bridge, normally over loopback.
5. The bridge is the only thing both sides share. OpenClaw does not connect directly to the Reforger server.

## Bundle contents

- `bridge/remote-director-bridge.ps1`
  Standalone bridge service with configurable bind URL, token auth, and local state files.
- `bridge/director-bridge.config.example.json`
  Concrete control-host bridge config.
- `bridge/Caddyfile.example`
  Reverse-proxy example for exposing the bridge over HTTPS.
- `bridge/install-bridge-task.ps1`
  Optional Scheduled Task installer for the control host.
- `server/rest-bridge.profile-override.example.json`
  Preferred runtime override file that the server operator uploads into the Reforger profile directory.
- `server/game-mode-rest-bridge.example.json`
  Author-time example values for `OCD_ZombieDirectorGameModeComponent` when you want to bake the bridge into a published mission.
- `server/addons-required-dead-everon-bacon.txt`
  Current concrete addon set for the Dead Everon + Bacon deployment.
- `openclaw/render-openclaw-director-session.ps1`
  Renders a ready-to-paste OpenClaw director prompt from the bridge config.
- `openclaw/start-openclaw-director.ps1`
  Starts the OpenClaw dashboard and renders the current director prompt.
- `install-control-host.ps1`
  One-step control-host setup that runs the configurator, optionally installs the bridge Scheduled Task, and starts the OpenClaw side.
- `package-cloud-bundle.ps1`
  Repackages this folder into a zip.

## Server-side application

The preferred install path is now:

1. publish the mission with `OCD_ZombieDirectorGameModeComponent`
2. keep `m_sConcreteProfileId` set correctly for the scenario
3. leave the bridge disabled by default on the component
4. let the server operator upload a profile-side override file

The game mode component should keep:

- `m_sConcreteProfileId = dead_everon_bacon_concrete`
- `m_RestBridge.m_sProfileOverridePath = openclaw-zombie-director/rest-bridge.override.json`
- `m_RestBridge.m_bEnabled = 0`

Then the operator can upload a file as:

- `<server profile>/openclaw-zombie-director/rest-bridge.override.json`

That lets local servers, Nitrado-style file-manager hosts, and FTP-based hosts enable OpenClaw or another AI director without republishing the mission.

`server/game-mode-rest-bridge.example.json` is still included for mission authors who deliberately want to bake the bridge values into a published scenario, but the runtime override file is the plug-and-play path.

## Who actually needs the hostname and token

Only the server operator or mission author needs them.

- Regular players joining the server do not need a hostname.
- Regular players joining the server do not need a token.
- The hostname/token are only for the server-side bridge link between the mod and the external director service.

If you want anyone to be able to deploy their own copy of this stack, they need a simple setup step, not hard-coded shared values.
Use the configurator:

```powershell
powershell -ExecutionPolicy Bypass -File ".\configure-cloud-bundle.ps1"
```

That generates:

- `bridge/director-bridge.config.json`
- `server/rest-bridge.profile-override.generated.json`
- `bridge/Caddyfile.generated`
- `openclaw/openclaw-director-session.generated.md`

## Control-host quick start

Fastest path:

```powershell
powershell -ExecutionPolicy Bypass -File ".\install-control-host.ps1"
```

That handles config generation, bridge task install, and OpenClaw session setup in one step.

Useful flags:

- `-SkipScheduledTask` if you only want to start the bridge for the current session
- `-NoOpenClaw` if you only want the bridge and generated server override file
- `-ListenPrefix` and `-ClientBaseUrl` if the default local port `18890` is already in use on the control host

Manual path:

1. Copy `bridge/director-bridge.config.example.json` to `bridge/director-bridge.config.json`.
2. Replace:
   - `director.example.com`
   - the token value
3. Start the bridge:

```powershell
powershell -ExecutionPolicy Bypass -File ".\bridge\remote-director-bridge.ps1" -Action start -ConfigPath ".\bridge\director-bridge.config.json"
```

4. Health check:

```powershell
powershell -ExecutionPolicy Bypass -File ".\bridge\remote-director-bridge.ps1" -Action health -ConfigPath ".\bridge\director-bridge.config.json"
```

5. Start OpenClaw and render the director prompt:

```powershell
powershell -ExecutionPolicy Bypass -File ".\openclaw\start-openclaw-director.ps1" -BridgeConfigPath ".\bridge\director-bridge.config.json"
```

If `openclaw` is not on `PATH` on the control host, pass the full CLI path:

```powershell
powershell -ExecutionPolicy Bypass -File ".\openclaw\start-openclaw-director.ps1" -BridgeConfigPath ".\bridge\director-bridge.config.json" -OpenClawCommand "C:\Users\you\.local\bin\openclaw.cmd"
```

## Game-server quick start

1. Install the addons listed in `server/addons-required-dead-everon-bacon.txt`.
2. Ensure the published mission/save includes `OCD_ZombieDirectorGameModeComponent` and the correct `m_sConcreteProfileId`.
3. Upload `server/rest-bridge.profile-override.generated.json` as:
   `<server profile>/openclaw-zombie-director/rest-bridge.override.json`
4. Make sure the game server can reach `https://director.example.com/`.
5. For self-managed servers, `<server profile>` is the folder passed to Reforger with `-profile`.
6. For hosted panels, use the provider file browser or FTP to place that file under the server profile/config area.

If you are handing this bundle to another admin, also point them at:

- [Server Operator Quick Start](../../docs/server-operator-quick-start.md)
- [Hosted Panel Quick Start](../../docs/hosting-panel-quick-start.md)

## If you do not want OpenClaw

You can still use this mod without OpenClaw:

- leave the bridge disabled on the game mode component, or
- keep the bridge enabled and post hints from another service

The mod itself does not require OpenClaw. OpenClaw is only one possible external director.
