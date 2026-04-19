# Hosted Panel Quick Start

This is the install path for rented servers such as Nitrado, GPORTAL, GTX, and similar providers.

`OpenClaw Zombie Director` is **experimental**.

The important point is that the server operator no longer has to republish the mission just to set a bridge URL or token.

## What the server owner needs

- the published scenario/mod stack
- the required addon dependency chain
- access to the host's file browser or FTP
- outbound HTTPS from the server to the bridge host if using OpenClaw

## Without OpenClaw

1. Install the scenario and required addons on the provider.
2. Leave the bridge disabled.
3. Start the server.

That is the simplest path.

## With OpenClaw

1. Unzip one of the control-host bundles on a separate machine:
   - [Windows bundle](../deploy/cloud/dist/openclaw-zombie-director-cloud-windows-dedicated-remote-openclaw.zip)
   - [Linux server bundle](../deploy/cloud/dist/openclaw-zombie-director-cloud-linux-dedicated-remote-openclaw.zip)
2. Run:

```powershell
powershell -ExecutionPolicy Bypass -File ".\install-control-host.ps1"
```

3. Take the generated file:
   - `server/rest-bridge.profile-override.generated.json`
4. Upload it through the provider file browser or FTP to:
   - `<server profile>/openclaw-zombie-director/rest-bridge.override.json`
5. Restart the server.

## What `<server profile>` means

On self-managed servers, that is the directory passed with Reforger's `-profile` argument.

On hosted panels, this is usually the config/profile area the host exposes through:

- a file manager in the web panel
- SFTP
- FTP

If the host lets you edit `config.json` or upload custom config files, it usually also gives you access to the profile/config area you need for this file.

## Why this is easier now

The scenario can stay published with safe defaults:

- bridge off by default
- one known override file path

Each server owner can opt into OpenClaw by uploading one JSON file instead of editing the scenario itself.

## What still has to be configured by the operator

- the actual bridge hostname
- the shared token
- where the control-host bridge runs

Players joining the server do not need any of that.

## Experimental note

This project is still **experimental**.
Treat the hosted-panel flow as usable, but still evolving.
