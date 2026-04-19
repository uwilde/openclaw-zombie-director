# OpenClaw Zombie Director

`OpenClaw Zombie Director` is an **experimental** zombie encounter director for Arma Reforger.

It is designed to work on:

- local hosted servers
- self-managed dedicated servers
- cloud-hosted servers
- hosted panels that allow normal mod/scenario install plus profile/config file upload

It can run:

- without OpenClaw
- with OpenClaw through a REST bridge
- with another external AI that uses the same bridge contract

## Experimental Status

This project is still **experimental**.

It is already usable, but you should expect balance changes, compatibility fixes, setup improvements, and packaging cleanup over time.

## Current Public Stack

The current public release is centered on one documented experimental stack:

- `OpenClaw Zombie Director`
- `OpenClaw Zombie Director Dead Everon`
- `Dead Everon`
- `Bacon Zombies`

That is the canonical stack for the current release candidate.

## Quick Start

### Players

If you are only joining a server, you do not need OpenClaw or any bridge credentials. The server operator handles that.

### Server Admins Without OpenClaw

1. Install the published scenario and required addons.
2. Leave the bridge disabled.
3. Start the server.

That is the safest and simplest path right now.

### Server Admins With OpenClaw

1. Use one of the control-host bundles in the project release package.
2. Run `install-control-host.ps1` on the control machine.
3. Upload the generated override file to:
   `<server profile>/openclaw-zombie-director/rest-bridge.override.json`
4. Restart the game server.

## Repo Layout

- [projects/reforger-zombie-director](projects/reforger-zombie-director) - core director addon, docs, presets, bundles, and public release packaging
- [projects/reforger-zombie-director-dead-everon-overlay](projects/reforger-zombie-director-dead-everon-overlay) - Dead Everon overlay addon and navmesh/world authoring content

## Start Here

- [Start Here](projects/reforger-zombie-director/deploy/public-release/START%20HERE.md)
- [Supported](projects/reforger-zombie-director/deploy/public-release/SUPPORTED.md)
- [Public Stack](projects/reforger-zombie-director/deploy/public-release/PUBLIC%20STACK.md)
- [Server Operator Quick Start](projects/reforger-zombie-director/docs/server-operator-quick-start.md)
- [Hosting Panel Quick Start](projects/reforger-zombie-director/docs/hosting-panel-quick-start.md)
- [Publish-Safe Scenario Defaults](projects/reforger-zombie-director/docs/publish-safe-scenario-defaults.md)

## Releases

Use the GitHub Releases page for packaged public artifacts, including the public release zip and control-host bundles.

## OpenClaw Is Optional

This mod does **not** require OpenClaw.

OpenClaw is an optional external director path. The mod can also run standalone or accept hints from another service that follows the same REST contract.
