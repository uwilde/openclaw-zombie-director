# Dead Everon Overlay Navmesh Authoring

This overlay now has one verified infantry navmesh authoring path shared by the
default overlay world and the smoke world.

## What is verified

As of April 18, 2026 on this machine:

- `DeadEveronDirectorSmoke.ent` can load an overlay-owned `Soldiers` navmesh
  from `Saves/Navmesh/dead_everon_smoke_soldiers_patch.nmn`
- `DeadEveronDirector.ent` now points at that same overlay-owned `Soldiers`
  navmesh instead of the packed Dead Everon infantry navmesh
- the old `Saves/nav soldier.nmn is obsolete` error is gone in editor load and
  play load for the smoke world
- the zombie director still spawns Bacon compatibility groups and applies
  dynamic sweep paths in play mode
- a tighter corridor patch regenerated for tiles `257,214 -> 268,223` saved
  directly back into `Saves/Navmesh/dead_everon_smoke_soldiers_patch.nmn`
- a fresh default-world play pass after that save produced the first
  `smoke_lab_corridor` wave at `18:16:40` and the first
  `smoke_village_edge` sweep at `18:17:01` without any new
  `Incorrect tile position calculated` errors in the session log

The smoke layer intentionally no longer binds explicit `BTRlike` or `LowRes`
navmesh files. Those two navmesh worlds initialize empty in the smoke test
surface because the smoke pass is infantry-only.

## Current limitation

Workbench can now save directly into the overlay when you select the existing
overlay navmesh path from the chooser:

- `Saves/Navmesh/dead_everon_smoke_soldiers_patch.nmn`

The old staging problem still applies when you use `Save Navmesh -> Other` and
let Workbench resolve a new path against the mounted base addon root.

## Working workflow

1. Open `Worlds/DeadEveronDirectorSmoke/DeadEveronDirectorSmoke.ent`.
2. Open `Tools -> Navmesh Tool`.
3. Select `Soldiers`.
4. Generate either:
   - the broad smoke baseline with:
     - from tile x `170`
     - from tile y `200`
     - to tile x `330`
     - to tile y `330`
   - or the corridor-fix patch with:
     - from tile x `257`
     - from tile y `214`
     - to tile x `268`
     - to tile y `223`
5. Save the generated navmesh from Workbench.
6. Prefer the existing overlay path when prompted:
   - `Saves/Navmesh/dead_everon_smoke_soldiers_patch.nmn`
7. Only use post-save staging when Workbench writes into the wrong addon root:

```powershell
powershell -ExecutionPolicy Bypass -File "X:\OpenClaw\workspace\projects\reforger-zombie-director-dead-everon-overlay\tools\stage-generated-navmesh.ps1" -DestinationBaseName dead_everon_smoke_soldiers_patch -CleanupSource
```

That command auto-detects the latest generated `.nmn` under the Workbench base
addon mount, copies it into:

- `Saves/Navmesh/dead_everon_smoke_soldiers_patch.nmn`
- `Saves/Navmesh/.NavData/dead_everon_smoke_soldiers_patch`

and removes the misplaced source artifact if `-CleanupSource` is supplied.

## World bindings

Both overlay-owned authoring worlds now point `Soldiers` at:

- `Saves/Navmesh/dead_everon_smoke_soldiers_patch.nmn`

That means a simple world reload is enough after staging for either:

- `Worlds/DeadEveronDirector/DeadEveronDirector.ent`
- `Worlds/DeadEveronDirectorSmoke/DeadEveronDirectorSmoke.ent`

## Residual runtime risk

The corridor-focused patch removed the specific first-wave tile errors that were
previously reproducing in the validated `18:14` to `18:17` default-world pass.

What still remains true:

- this is a targeted infantry patch, not a full-map navmesh rebuild
- broader Dead Everon content noise still exists outside the director-owned
  overlay
- any new high-value encounter lane should still be smoke-tested after its
  anchors or sweep paths move
