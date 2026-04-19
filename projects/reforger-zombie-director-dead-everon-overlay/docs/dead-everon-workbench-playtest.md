# Dead Everon Workbench Playtest

The Dead Everon Workbench surface for the director now lives in the dedicated
overlay addon:

- `X:\OpenClaw\workspace\projects\reforger-zombie-director-dead-everon-overlay`

## Recommended world

Use the default overlay world:

- `Worlds/DeadEveronDirector/DeadEveronDirector.ent`

This sub-scene inherits the packed Dead Everon map save:

- `"{C1B9167BF961081D}Saves/Dead Everon Map.ent"`

Workbench URI:

- `enfusion://WorldEditor/Worlds/DeadEveronDirector/DeadEveronDirector.ent`

Why this is the default:

- it avoids the additional `GameMaster.layer` load
- it eliminates the extra vehicle and duplicate-component noise tied to the
  packed Game Master layer
- it still gives you an editable overlay-owned layer for bespoke placements

## Legacy world

Use this only when you specifically need to compare behavior against the packed
Game Master save:

- `Worlds/DeadEveronDirector/DeadEveronDirectorGameMaster.ent`

Parent:

- `"{28C64AE6FF448403}Saves/Dead Everon Game Master.ent"`

## Prerequisites on this machine

- Dead Everon and its dependencies must be mounted in `C:\Users\UWilde\Documents\My games\ArmaReforgerWorkbench\addons`
- the base game `data` addon must be mounted into `Arma Reforger Tools\Workbench\addons\data`
- `BaconZombies_622120A5448725E3` must be mounted because the default concrete profile uses Bacon prefabs

Concrete addon chain observed during the verified Dead Everon load here:

- `DeadEveron_5D58A217A9611AFB`
- `WolfsBuildingPack_5D84C738432BFE6A`
- `RailwayGenerator_5F0D245931200FD1`
- `iRON79Compositions_60A68ED7293C691A`
- `CrocellsWorkshopofHorrors_6206C7238516657B`
- `BaconZombies_622120A5448725E3`

The minimal-stack audit for this exact setup is documented in:

- `docs/minimal-stack-audit.md`

Repeat that audit with:

- `powershell -ExecutionPolicy Bypass -File "X:\OpenClaw\workspace\projects\reforger-zombie-director-dead-everon-overlay\tools\audit-minimal-stack.ps1"`

## Current status

- the OpenClaw Zombie Director scripts compile in Workbench
- the overlay world loads Dead Everon through a dedicated addon instead of the core director project
- the map-parent world is the healthiest currently available authoring surface without unpacking the original terrain addon
- the default overlay world and the smoke world now both use the overlay-owned `Soldiers` navmesh at `Saves/Navmesh/dead_everon_smoke_soldiers_patch.nmn`
- the overlay-owned worlds no longer need the old packed `Saves/nav soldier.nmn` file for infantry routing
- Workbench can now overwrite the existing overlay navmesh path directly when you save back into `Saves/Navmesh/dead_everon_smoke_soldiers_patch.nmn`
- the corridor-fix patch for tiles `257,214 -> 268,223` is saved into that overlay navmesh
- a fresh default-world play pass after that patch produced:
  - first lab-corridor wave at `18:16:40`
  - first village-edge sweep at `18:17:01`
  - second village-edge sweep at `18:17:19`
  - no new `PATHFINDING(E): Incorrect tile position calculated for idx: <...>` lines in that post-patch run
- `tools/stage-generated-navmesh.ps1` is still the fallback when Workbench saves to the wrong addon root instead of the existing overlay path

## Remaining upstream issues

These still show up during world initialization and are not coming from the
OpenClaw director scripts:

- resource GUID or name mismatches inside the packed Dead Everon map content
- obsolete packed navmesh files such as `Saves/nav btr.nmn` and `Saves/lowres.nmn` still exist in the mounted Dead Everon content, but the overlay-owned worlds no longer bind them directly for infantry authoring
- some duplicate component conflicts that remain inside the packed map stack

## Smoke-world-specific note

`DeadEveronDirectorSmoke.ent` is now intentionally infantry-biased.

- `Soldiers` points at the overlay-staged smoke navmesh
- `BTRlike` and `LowRes` are not explicitly bound there, so they initialize empty instead of loading the packed obsolete Dead Everon files
- this keeps the smoke pass focused on zombie infantry routing rather than vehicle/navmesh compatibility

## Default-world note

`DeadEveronDirector.ent` now follows the same infantry-biased binding strategy.

- `Soldiers` points at `Saves/Navmesh/dead_everon_smoke_soldiers_patch.nmn`
- `BTRlike` and `LowRes` are not explicitly bound there, so the default authoring world no longer directly references the packed obsolete Dead Everon navmesh files

The detailed authoring flow is documented in:

- `docs/navmesh-authoring.md`

What changed with the split:

- Game Master-specific conflicts are isolated to the legacy world instead of the default authoring target
- the core director addon no longer depends on Dead Everon
- the overlay keeps map-specific world files and dependency noise contained

## Limitation

The original Dead Everon packed layers are still not directly editable. Your
own placements belong in:

- `Worlds/DeadEveronDirector/DeadEveronDirector_Layers/default.layer`
