# Minimal Stack Audit

Verified against the Dead Everon overlay load on April 17, 2026, using:

- `X:\OpenClaw\workspace\projects\reforger-zombie-director\addon.gproj`
- `X:\OpenClaw\workspace\projects\reforger-zombie-director-dead-everon-overlay\addon.gproj`
- `C:\Users\UWilde\Documents\My games\ArmaReforgerWorkbench\logs\logs_2026-04-17_13-31-07\console.log`

## Step 1 result

The minimal playable stack is established on the OpenClaw side.

Direct addons in the intended stack:

- `data` `58D0FB3206B6F859`
- `OpenClaw Zombie Director` `6F8B1A34C9D24011`
- `OpenClaw Zombie Director - Dead Everon Overlay` `3A1D4E6C8B7F2059`
- `Dead Everon` `5D58A217A9611AFB`
- `Bacon Zombies` `622120A5448725E3`

What cannot be removed without forking or unpacking Dead Everon:

- `WolfsBuildingPack` `5D84C738432BFE6A`
- `RailwayGenerator` `5F0D245931200FD1`
- `iRON79Compositions` `60A68ED7293C691A`
- `CrocellsWorkshopofHorrors` `6206C7238516657B`

Those four addons are not being pulled in by the OpenClaw director addons. They are transitive dependencies declared by `Dead Everon` itself.

## What was verified

- The core addon depends only on the base game data addon.
- The overlay addon depends only on the base game data addon, the core director addon, and `Dead Everon`.
- `Bacon Zombies` remains optional for opening the overlay world, but required for the concrete Bacon-prefab zombie templates to spawn for live playtests.
- The verified Workbench load did not mount any unexpected workshop addons beyond the `Dead Everon` transitive dependency chain.
- The overlay world still opens successfully as `World Editor - OpenClaw-Zombie-Director-Dead-Everon/Worlds/DeadEveronDirector/DeadEveronDirector.ent`.

## Error classification

These findings come from the verified overlay load log and are grouped by likely impact.

### Cosmetic startup scan noise

These do not block the first playable pass:

- `metafile without corresponding resource` in `WolfsBuildingPack`
- `resource not registered` entries from `WolfsBuildingPack`
- `duplicate GUID found` entries in `CrocellsWorkshopofHorrors`
- `resource not registered` entries for the OpenClaw JSON preset files

Interpretation:

- safe to ignore while validating world load, spawn flow, and director logic
- worth cleaning only if we later need a quieter authoring environment

### Asset binding and visual integrity risk

These are upstream content mismatches that can affect local props or materials:

- `Wrong GUID/name for resource ... Failed to open`
- `Wrong GUID for resource ... in property "MeshMaterial"`
- `Can't remap ... Source material do not exist`

Interpretation:

- the world still loads
- some structures, clutter, or decorative props may render incorrectly
- these are not automatic blockers for the director smoke pass unless they touch the specific encounter spaces we test

### AI pathing risk

These are the first errors to treat as gameplay-relevant:

- `PATHFINDING(E): NavmeshCustomLinkComponent couldn't find dependent class!`

Interpretation:

- these can break traversal around specific door prefabs
- this matters for zombie sweep behavior near the affected structures
- step two should avoid those structures until we confirm whether the affected areas overlap our first encounter zones

### Local interaction and destructible risk

These are likely limited to specific placed assets rather than whole-map failure:

- `component MeshObject cannot be combined with component MeshObject`
- `component Hierarchy cannot be combined with component Hierarchy`
- `component RplComponent cannot be combined with component RplComponent`
- `Destructible ... doesn't have a initial and final phase`
- `SCR_DestructionMultiPhaseComponent can't be attached`
- `User action '#AR-UserAction_Open' created with no ActionsManagerComponent present`

Interpretation:

- some destructibles, doors, gates, or replicated props may be unreliable
- this is a real map-content risk, but it is still local rather than systemic
- encounter spaces that depend on those props should be validated individually

## Practical verdict

Step one is complete.

The smallest realistic stack for this project is not five isolated addons in practice; it is the five intended addons plus the four workshop addons that `Dead Everon` transitively requires. That is the hard lower bound unless we fork or replace the map.

That means step two can proceed now, but it should focus on:

- one concrete zombie spawn from a Bacon template
- one director-triggered spawn
- one waypoint sweep
- one or two hand-picked encounter spaces that do not rely on obviously broken path-link or interactable-heavy structures

## Repeatable audit

Use the audit helper to re-check this state after any dependency or map change:

```powershell
powershell -ExecutionPolicy Bypass -File "X:\OpenClaw\workspace\projects\reforger-zombie-director-dead-everon-overlay\tools\audit-minimal-stack.ps1"
```
