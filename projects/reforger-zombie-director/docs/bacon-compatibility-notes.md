# Bacon Compatibility Notes

This repo includes a local Bacon compatibility layer under `Prefabs/Compatibility/Bacon`.
Its job is to keep the director usable against the packed `BaconZombies` addon without editing the upstream pack.

## What this local fork fixes

- The director now prefers local compatibility group prefabs instead of mixed direct-prefab spawning paths.
- The director no longer depends on editor-only spawn or delete components, which is the important cloud and dedicated-server fix.
- The repeated REST callback warning path is fixed in the bridge code and is unrelated to Bacon now.

## What warnings can still remain

Two warning classes can still show up with the current packed Bacon addon:

- Workbench catalog warnings for:
  - `Group_Zombies_Horde.et`
  - `Group_Zombies_CIV_Large.et`
- A legacy icon GUID warning when Bacon loads `Prefabs/Weapons/Handgun_Melee_base.et` and resolves `UI/Textures/WeaponIcons/weapon_PM.edds`

## Why these are still present

These warnings originate inside the packed upstream Bacon addon, not in the OpenClaw director source tree.
On this machine the Bacon addon is installed as a packed `data.pak`, so the underlying prefab and texture resources are not directly editable here.

That means this repo can safely:

- wrap Bacon with compatibility prefabs
- avoid the noisiest director-owned spawn paths
- keep the mod working in Workbench and on headless servers

But it cannot fully remove every Bacon pack warning unless one of these changes happens:

1. an unpacked/editable Bacon worktree is available
2. the Bacon authors update those legacy references upstream
3. the concrete profile switches to a different zombie dependency

## Practical interpretation

- The remaining `Group_Zombies_*` warnings are primarily Workbench editor catalog noise.
- The `weapon_PM.edds` warning is noisy but has not blocked live zombie spawns in the validated Dead Everon runs.
- These warnings are not the reason the mod would fail on a cloud server. The important server-side requirement is the runtime spawn path, and that is now native and headless-safe.
