# GAM415-FirstPerson - Fantasy Portal Refinement

This Unreal Engine 5.8 C++ artifact refines the GAM 415 gallery prototype into a compact fantasy-FPS systems demonstration for GAM 495.

## Start here: refinement proof

- **Evidence matrix:** [`evidence/GAM495_REFINEMENT_EVIDENCE.md`](evidence/GAM495_REFINEMENT_EVIDENCE.md)
- **38-second labeled gameplay video:** [`media/evidence/yun_daniel_artifact1_refinement_evidence.mp4`](media/evidence/yun_daniel_artifact1_refinement_evidence.mp4)
- **Screenshots:** [`media/evidence/`](media/evidence/)

## Refined systems

- **Arcane primary spell:** the hero starts every life with the spell weapon. Projectile mesh, impact decal, and Niagara effect share one configurable blue spell color. Decals expire after eight seconds.
- **Arcane Trial AI objective:** the HUD shows a short, completable objective sized from the NPC spawners in the level. Only player kills advance it; the recorded run shows `0/2 -> COMPLETE 2/2`.
- **Portal mobility ability:** Q blink with a walkable-ground check, six-second cooldown, invalid activation state, and spawn/death/respawn lifecycle reset.
- **Native HUD:** top-center objective, center crosshair/blue X hit marker, and bottom-center portal ready/cooldown/blocked status. Existing Blueprint HUD integration remains available.
- **Linked portal pairs:** full relative position/orientation mapping, controller view rotation, movement velocity mapping, safe destination capsule sweep, and scene-capture transform parity.

## Run

1. Open `firstperson415.uproject` in Unreal Engine 5.8.
2. Open `/Game/Variant_Shooter/Lvl_Shooter` for the spell, AI objective, and Q ability.
3. Open `/Game/FinalProject/Lvl_FinalProject` for linked portals and graphics demonstrations.
4. **Controls:** left mouse = spell fire, Q = portal mobility, standard WASD/mouse = movement/look.

## Build

```text
Engine/Build/BatchFiles/Build.bat firstperson415Editor Win64 Development -Project=<path>/firstperson415.uproject -WaitMutex -NoHotReloadFromIDE
```

Validated with the installed Unreal Engine 5.8 editor target on Windows. Both maps launched in standalone game mode. The final labeled capture is 38.29 seconds at 1280x720/24 fps. A packaged Windows build was not produced.

## Source map

| System | Primary files |
|---|---|
| Spell identity / decal lifetime | `Variant_Shooter/Weapons/ShooterProjectile.h/.cpp` |
| Q ability / starting spell / lifecycle | `Variant_Shooter/ShooterCharacter.h/.cpp` |
| Native HUD and hit marker | `Variant_Shooter/UI/ShooterNativeHUD.h/.cpp` |
| AI objective | `Variant_Shooter/ShooterGameMode.h/.cpp`, `Variant_Shooter/AI/ShooterNPC*` |
| Linked portal transforms | `Portal.h/.cpp` |
