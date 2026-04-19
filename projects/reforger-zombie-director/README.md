# OpenClaw Zombie Director

`OpenClaw Zombie Director` is a Reforger-side director scaffold for zombie-heavy scenarios.
It is designed to work in three modes:

1. Standalone: the director uses player position, movement, zone tags, cooldowns, and budgets.
2. Scenario-driven: other Reforger scripts call the local API to push high-level hints such as `reinforce_zone` or `stage_ambush`.
3. OpenClaw-assisted: the game polls a small local REST bridge, so OpenClaw can watch the session and inject intent without clicking the Game Master UI.

This package is intentionally reusable rather than hard-wired to one zombie asset pack. You plug in your own zombie group or character prefabs through the template list on the game mode component.

## What is included

- A native addon scaffold with `addon.gproj`
- `OCD_ZombieDirectorGameModeComponent`
- `OCD_ZombieDirectorZoneComponent`
- Budget, template, hint, and snapshot data models
- A REST bridge client for OpenClaw or any other external director
- A script API that other mods can call directly
- A local OpenClaw skill plus PowerShell bridge service

## Core design

The mod assumes the best zombie experience comes from authored zones plus dynamic direction:

- Zones tell the system where pressure should come from.
- Templates tell it what kind of wave can spawn there.
- Budgets and cooldowns prevent spam.
- Static waypoint names let mission makers define sweeps that feel deliberate.
- External hints let OpenClaw or other systems escalate the right zone at the right moment.

## Recommended setup in Reforger Workbench

1. Add `OCD_ZombieDirectorGameModeComponent` to your `SCR_BaseGameMode`.
2. Place simple helper entities anywhere you want the director to operate from.
3. Add `OCD_ZombieDirectorZoneComponent` to each helper entity.
4. Tag zones with values like `base`, `field`, `village`, `road`, `fallback`, `chokepoint`.
5. If you want authored sweeps, place waypoint entities in the world and list their names on the zone.
6. Fill the game mode component's template array with your zombie prefabs.

## Template strategy

Use two template families:

- Group prefabs for strong authored sweeps.
- Individual zombie prefabs for ambient pressure and flank spawns.

Group-prefab templates are the preferred way to get "spawn and sweep" behavior because the director can attach the zone's static waypoint chain to the spawned group.

## OpenClaw bridge

The addon can poll a REST bridge, but the bridge is now disabled by default so standalone servers do not have to change anything.
When enabled, the default bridge base URL remains `http://127.0.0.1:18890/`.
The bundled skill under `workspace/skills/reforger-zombie-director` starts a local bridge that exposes:

- `GET /reforger/zombie-director/health`
- `GET /reforger/zombie-director/hints?afterSeq=N`
- `POST /reforger/zombie-director/hints`
- `POST /reforger/zombie-director/snapshot`
- `GET /reforger/zombie-director/snapshot`

That lets OpenClaw act like a pacing director:

- "base nearly captured -> reinforce zone_farmhouse"
- "players entering open ground -> stage ambush on zone_wheatfield"
- "session is too quiet -> force spike in zone_railcut"

The wire format is documented in [openclaw-rest-contract.md](docs/openclaw-rest-contract.md).

## Dedicated and cloud servers

Yes, this design can run on dedicated and cloud-hosted Reforger servers.
The director logic is server-authoritative and both the core addon and the Dead Everon overlay ship a `HEADLESS` configuration in `addon.gproj`.
The core spawn path now uses native runtime entity spawning and runtime waypoint attachment rather than editor-only placement components, which is the part that matters for dedicated and cloud deployment.

Use one of these deployment modes:

- Standalone or scenario-driven: leave `m_RestBridge.m_bEnabled = false` and let the server run only on local director logic plus Reforger-side hints.
- Same-host OpenClaw: enable the bridge on the component or upload a profile override file that keeps `m_sBaseUrl = http://127.0.0.1:18890/`.
- Remote OpenClaw or external AI director: upload a profile override file that points `m_sBaseUrl` at a reachable `https://...` bridge and sets `m_sToken`.

The preferred public-server operator path is a profile-side JSON override file:

- component default: `m_RestBridge.m_bEnabled = false`
- component default: `m_RestBridge.m_sProfileOverridePath = openclaw-zombie-director/rest-bridge.override.json`
- operator file location: `$profile:openclaw-zombie-director/rest-bridge.override.json`

That means a mission author can publish one safe default scenario and each server owner can opt into OpenClaw or another AI director by uploading one file, without republishing the mission.

Important limitation:

- Headless cloud servers cannot use OpenClaw's desktop-vision or Game Master UI-clicking path. On dedicated/cloud infrastructure, the supported model is the REST bridge plus direct hook mod, not GUI automation.

Operational notes:

- The server must have the same map and zombie-pack dependencies installed.
- The server needs outbound network access to the bridge URL if the bridge is remote.
- Off-loopback bridge deployments should sit behind TLS and real auth; the token field here is only a minimal control.

See [dedicated-cloud-servers.md](docs/dedicated-cloud-servers.md) for the concrete deployment checklist.
For the short operator path, use [server-operator-quick-start.md](docs/server-operator-quick-start.md) and [hosting-panel-quick-start.md](docs/hosting-panel-quick-start.md).

## Researched presets

The repo now includes a preset pass built around real workshop traction rather than generic assumptions:

- Pack profiles: [zombie-pack-profiles.json](X:/OpenClaw/workspace/projects/reforger-zombie-director/presets/zombie-pack-profiles.json)
- Map profiles: [zombie-map-profiles.json](X:/OpenClaw/workspace/projects/reforger-zombie-director/presets/zombie-map-profiles.json)
- Combined bundle presets: [director-profile-bundles.json](X:/OpenClaw/workspace/projects/reforger-zombie-director/presets/director-profile-bundles.json)
- Research notes: [popular-zombie-packs-and-maps.md](X:/OpenClaw/workspace/projects/reforger-zombie-director/docs/popular-zombie-packs-and-maps.md)

The preset set is centered on:

- Packs: Bacon Zombies, ToH Zombie, ARMST PLATFORM - Mutants, WCS_Zombies, Zombie Characters
- Maps: Everon, Arland, Dead Everon, Dead Everon 2, Woodbury

I also recorded ReforgedZ_Zombies as a high-traction pack that is excluded from reusable public presets because its workshop page restricts unauthorised hosting and deployment.

There is now one concrete authored pass, not just abstract slot planning:

- [dead-everon-bacon-concrete.md](X:/OpenClaw/workspace/projects/reforger-zombie-director/docs/dead-everon-bacon-concrete.md) wires Dead Everon to the locally verified Bacon Zombies resources.
- `dead_everon_bacon_concrete` emits actual resource names, not only abstract template slots.
- The Dead Everon map profile now includes reusable zone archetypes for perimeters, village edges, forest pursuit, quarantine corridors, checkpoint chokes, lab exteriors, and refugee roads.
- The game mode component can now bootstrap Dead Everon military-base zones at runtime when `m_sConcreteProfileId` is set to `dead_everon_bacon_concrete`.
- That runtime path now runs on a faster cadence, smooths zone pressure, deduplicates repeated hints, and generates local event spikes for `village-edge`, `forest-trail`, `roadblock`, and `lab/quarantine` situations.
- The Dead Everon concrete runtime also emits script-authored encounter clusters aligned to each real military-base transform:
  `checkpoint_gate`, `village_edge`, `forest_trail`, and `lab_corridor`.
- Those generated encounter zones carry route points, so spawn placement and threat direction feel more deliberate even when the packed workshop map does not expose editable `.layer` files.
- Base capture progress still injects local hints for `dead_everon_checkpoint_mix`, `dead_everon_quarantine_spike`, `dead_everon_tier1_sweep`, and `dead_everon_horde_push`.

For map-specific Workbench authoring on top of Dead Everon, use the sibling
overlay addon at `workspace/projects/reforger-zombie-director-dead-everon-overlay`.
That overlay owns the Dead Everon world files and keeps this core addon
map-agnostic. Its default authoring world is
`Worlds/DeadEveronDirector/DeadEveronDirector.ent`, which is a sub-scene over the
packed Dead Everon map save rather than the noisier Game Master save. The
verified Workbench URI remains:
`enfusion://WorldEditor/Worlds/DeadEveronDirector/DeadEveronDirector.ent`

Use the helper script to build a merged recommendation payload for a specific combo:

```powershell
powershell -ExecutionPolicy Bypass -File "X:\OpenClaw\workspace\skills\reforger-zombie-director\scripts\build-director-profile.ps1" -ProfileId dead_everon_2_bacon_mutants
```

For the concrete Dead Everon + Bacon Zombies stack:

```powershell
powershell -ExecutionPolicy Bypass -File "X:\OpenClaw\workspace\skills\reforger-zombie-director\scripts\build-director-profile.ps1" -ProfileId dead_everon_bacon_concrete
```

To enable the built-in runtime bootstrap in Workbench:

1. Add `OCD_ZombieDirectorGameModeComponent` to your game mode.
2. Set `m_sConcreteProfileId` to `dead_everon_bacon_concrete`.
3. Fill the template list with the concrete template ids from the generated profile.

That gives you base-centered starter zones plus script-authored Dead Everon encounter clusters even before you hand-author custom zone entities.

## Example use from another Reforger script

```c
OCD_ZombieDirectorAPI.QueueLocalHint(
	"reinforce_zone",
	"zone_farmhouse",
	"z_fast",
	12,
	2.0,
	"base nearly captured"
);
```

## Important caveats

- The core addon now depends only on the base game `data` addon again. Dead Everon-specific Workbench assets live in the separate overlay addon.
- Workbench on this machine still needs the base game `data` addon mounted into `Arma Reforger Tools\Workbench\addons\data`.
- The Dead Everon overlay needs `DeadEveron` mounted in `C:\Users\UWilde\Documents\My games\ArmaReforgerWorkbench\addons`. The concrete `dead_everon_bacon_concrete` preset also needs `BaconZombies` mounted when you actually want to spawn those Bacon prefabs.
- `DeadEveron` itself transitively mounts `WolfsBuildingPack`, `RailwayGenerator`, `iRON79Compositions`, and `CrocellsWorkshopofHorrors`. That is currently unavoidable without replacing or forking the map addon.
- The local Bacon compatibility layer removes the director-owned warning paths, but a small number of warnings still come from Bacon's packed addon content rather than this repo. See [bacon-compatibility-notes.md](docs/bacon-compatibility-notes.md).
- You still need actual zombie prefabs from your chosen zombie asset pack or your own content.
- The original Dead Everon packed layers are not directly editable; map-specific edits belong in the overlay addon's sub-scene layer.
- The cleaner default for Workbench is now the Dead Everon map parent, not the Game Master save. That removes the extra `GameMaster.layer` conflicts, but the packed Dead Everon map stack still throws upstream resource GUID mismatches and obsolete navmesh warnings during initialization.

## Suggested next pass in Workbench

1. Open the sibling overlay project at `workspace/projects/reforger-zombie-director-dead-everon-overlay`.
2. Load `Worlds/DeadEveronDirector/DeadEveronDirector.ent` in World Editor.
3. Keep `DeadEveronDirectorGameMaster.ent` only for legacy/debug comparison against the Game Master save.
4. Attach the game mode component and add real zombie prefabs.
5. Place custom encounter anchors and route points in the overlay-owned layer.
6. Validate group-prefab sweeps against those authored placements.
7. Tune zone radii, cooldowns, and budget caps per zone type.
