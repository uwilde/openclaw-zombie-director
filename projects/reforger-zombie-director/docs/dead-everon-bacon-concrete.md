# Dead Everon + Bacon Zombies Concrete Profile

This is the first non-generic profile in the preset set.
It is built against one real map and one real zombie stack that are both installed locally on this PC:

- Map: `Dead Everon` `5D58A217A9611AFB`
- Zombie stack: `Bacon Zombies` `622120A5448725E3`

The point of this pass is to stop talking about abstract slots like `walker_basic` and instead emit templates that already carry real prefab resources and Dead Everon-specific zone archetypes.

There is now also a runtime bootstrap path in the director itself:

- Set `m_sConcreteProfileId` on `OCD_ZombieDirectorGameModeComponent` to `dead_everon_bacon_concrete`.
- The director will generate starter zones around Dead Everon military bases at runtime.
- It will also watch base capture progress and queue local hints against those generated zones.
- The live loop now runs faster, smooths zone scores, suppresses duplicate hints, and emits short-lived local events for `village-edge`, `forest-trail`, `roadblock`, and `lab/quarantine` pressure.
- Because Dead Everon is installed here as a packed workshop terrain rather than an extracted Workbench project, the current "hand-authored" pass is implemented as script-authored encounter clusters aligned to real military-base transforms instead of direct `.layer` edits.

## Scenario resources

- Scenario config: `{850C623AD7B97DC7}Missions/DeadEveron.conf`
- Recommended Workbench overlay world: `{C1B9167BF961081D}Saves/Dead Everon Map.ent`
- Legacy debug Game Master world: `{28C64AE6FF448403}Saves/Dead Everon Game Master.ent`
- Recommended overlay addon: `workspace/projects/reforger-zombie-director-dead-everon-overlay`

## Concrete templates

- `dead_everon_civilian_drifters`
  Uses `Zombie_CIV.et` for ruins, roads, treelines, and village edges.
- `dead_everon_checkpoint_mix`
  Uses `Group_Zombies_CIV.et` as the compatibility-safe routed checkpoint reinforcement group.
- `dead_everon_quarantine_spike`
  Uses `Zombie_CIV.et` only, so quarantine spikes stay compatible with the current Reforger build.
- `dead_everon_tier1_sweep`
  Uses `Group_Zombies_CIV.et` for authored routed sweeps through villages and roads.
- `dead_everon_ussr_patrol_large`
  Is retained as a legacy compatibility alias and now resolves to the same stable `Group_Zombies_CIV.et` group path.
- `dead_everon_horde_push`
  Uses repeated `Zombie_CIV.et` spawns for a large pressure pulse without depending on Bacon's older special-unit mixes.
- `dead_everon_heavy_brute`
  Is retained as a legacy compatibility alias and now resolves to the stable `Group_Zombies_CIV.et` group path.

## Zone archetypes

The Dead Everon map profile now includes these zone archetypes:

- `dead_everon_base_perimeter`
- `dead_everon_village_edge`
- `dead_everon_forest_pursuit`
- `dead_everon_checkpoint_choke`
- `dead_everon_quarantine_corridor`
- `dead_everon_lab_exterior`
- `dead_everon_refugee_road`

Each archetype is expressed in the same terms as `OCD_ZombieDirectorZoneComponent`:

- tags
- radius
- spawn ring
- despawn distance
- local budget cap
- cooldown
- ambient, field, and base weights
- allowed template ids

## Build command

```powershell
powershell -ExecutionPolicy Bypass -File "X:\OpenClaw\workspace\skills\reforger-zombie-director\scripts\build-director-profile.ps1" -ProfileId dead_everon_bacon_concrete
```

Use the resulting JSON as the concrete authoring baseline for a Dead Everon mission instead of starting from the generic research bundles.

## Runtime zone id pattern

When the runtime bootstrap is enabled, generated military-base zones use:

- `dead_everon_base_<callsign>`

The concrete runtime now also emits these aligned encounter zones per base, depending on base size:

- `dead_everon_base_<callsign>_checkpoint_gate`
- `dead_everon_base_<callsign>_village_edge`
- `dead_everon_base_<callsign>_forest_trail`
- `dead_everon_base_<callsign>_lab_corridor`

Those encounter zones carry scripted route points used for route-aware spawn placement and tighter local event triggers. Direct world-waypoint attachment is still available when a mission author later adds named waypoints in Workbench.
