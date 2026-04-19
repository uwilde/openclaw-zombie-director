# OpenClaw Zombie Director Dead Everon Overlay

This addon is the map-specific Dead Everon authoring surface for the reusable
`OpenClaw Zombie Director` core addon.

Use this overlay when you want to:

- open Dead Everon in Workbench without unpacking the original workshop terrain
- place bespoke encounter anchors, route points, and supporting entities in your own layer
- keep Dead Everon dependency noise out of the map-agnostic core director addon

## Included worlds

- `Worlds/DeadEveronDirector/DeadEveronDirector.ent`
  Recommended default. Inherits the packed Dead Everon map save:
  `"{C1B9167BF961081D}Saves/Dead Everon Map.ent"`
- `Worlds/DeadEveronDirector/DeadEveronDirectorGameMaster.ent`
  Legacy/debug option. Inherits the packed Dead Everon Game Master save:
  `"{28C64AE6FF448403}Saves/Dead Everon Game Master.ent"`

The default Workbench URI is:

`enfusion://WorldEditor/Worlds/DeadEveronDirector/DeadEveronDirector.ent`

## Dependencies

- Base game data addon: `58D0FB3206B6F859`
- Core director addon: `6F8B1A34C9D24011`
- Dead Everon: `5D58A217A9611AFB`
- Bacon Zombies: `622120A5448725E3`

Dead Everon itself transitively mounts:

- `WolfsBuildingPack_5D84C738432BFE6A`
- `RailwayGenerator_5F0D245931200FD1`
- `iRON79Compositions_60A68ED7293C691A`
- `CrocellsWorkshopofHorrors_6206C7238516657B`

That transitive chain is the current lower bound for a real Dead Everon playtest
without forking or unpacking the map addon. See
[`docs/minimal-stack-audit.md`](X:/OpenClaw/workspace/projects/reforger-zombie-director-dead-everon-overlay/docs/minimal-stack-audit.md)
for the verified April 17, 2026 audit and
[`tools/audit-minimal-stack.ps1`](X:/OpenClaw/workspace/projects/reforger-zombie-director-dead-everon-overlay/tools/audit-minimal-stack.ps1)
to rerun it.

## Current state

- The overlay keeps the Dead Everon dependency chain out of the core addon.
- The map-parent world is the cleaner authoring baseline because it avoids the
  extra `GameMaster.layer` conflicts.
- The remaining resource GUID mismatches and obsolete navmesh warnings are still
  upstream in the packed Dead Everon map content. This overlay cannot rewrite
  those packed source layers.
