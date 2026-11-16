# GAM 495 Artifact 1 - Refinement Evidence

This page maps the approved Module One refinement plan to concrete source changes and run evidence. It is intended to make each enhancement easy to locate.

## Quick review

- **Labeled gameplay video (38.29 s):** [`media/evidence/yun_daniel_artifact1_refinement_evidence.mp4`](../media/evidence/yun_daniel_artifact1_refinement_evidence.mp4)
- **Shooter map:** `/Game/Variant_Shooter/Lvl_Shooter`
- **Portal gallery map:** `/Game/FinalProject/Lvl_FinalProject`
- **Build target:** `firstperson415Editor Win64 Development`, Unreal Engine 5.8

## Approved-plan evidence matrix

| Approved refinement | Before | Refined implementation | Evidence |
|---|---|---|---|
| 1. Fantasy hero FPS with primary spell and portal movement | The player had to find a template weapon pickup; projectile color was randomized. | Player/respawn receives `BP_ShooterWeapon_Rifle` as the primary arcane spell. Q invokes portal mobility. | `ShooterCharacter.cpp` (`GrantStartingSpell`, `TryActivatePortalAbility`); video 0:00-0:30. |
| 2. Ability lifecycle: input, cooldown, valid/invalid states, death/respawn reset | No hero mobility ability or state lifecycle. | Native Q fallback plus optional Enhanced Input; walkable-ground validation; 6 s cooldown; invalid activation does not consume cooldown; state resets per life. | `ShooterCharacter.h/.cpp`; `ShooterPlayerController.cpp`; [`04-portal-cooldown.png`](../media/evidence/04-portal-cooldown.png) OCR: `Q PORTAL 5.4s`; [`05-portal-ready.png`](../media/evidence/05-portal-ready.png) OCR: `Q PORTAL READY`. Runtime log: blink 987 cm, retry rejected with 4.7 s remaining, ready after 6 s. |
| 3. HUD: cooldown, crosshair, hit feedback | No portal-state HUD or native fallback. | Native C++ HUD draws objective, crosshair, portal state, and a brief blue X/full-screen hit confirmation on enemy hits. Existing Blueprint HUD remains supported. | `ShooterNativeHUD.h/.cpp`; `ShooterPlayerController.cpp`; [`01-hud-objective.png`](../media/evidence/01-hud-objective.png); [`02-spell-hit-marker.png`](../media/evidence/02-spell-hit-marker.png). |
| 4. Rotated portal transform for position, rotation, velocity, and scene capture | Teleport moved to the destination arrow and copied only its yaw; scene capture used a simple offset. | Full relative transforms drive player pose and render-target camera; velocity and controller rotation map through the pair; destination capsule sweep selects a safe exit. | `Portal.h/.cpp`; video 0:30-0:38. Runtime evidence: destination yaw `0 -> 90`; velocity `V(X=694.32, Z=-61.68) -> V(Y=694.32, Z=-61.68)`. |
| 5. Consistent spell identity and finite decals | Every projectile chose a random color and impact decals had no finite lifetime. | One configurable `SpellColor` drives projectile DMI, decal DMI, and Niagara `RandomColor`; `DecalLifeSeconds` defaults to 8 s. | `ShooterProjectile.h/.cpp`; video spell segment; [`02-spell-hit-marker.png`](../media/evidence/02-spell-hit-marker.png). |
| 6. Short objective with existing AI, gameplay video, README | The shooter template contained AI and team score, but no explicit GAM 495 objective or labeled evidence. | `Arcane Trial` counts only AI enemies killed by a player. Goal derives from the two NPC spawners in this level, so the displayed objective is completable (`0/2 -> 1/2 -> COMPLETE 2/2`). | `ShooterGameMode.h/.cpp`, `ShooterNPC.h/.cpp`, `ShooterNPCSpawner.h`; [`03-objective-complete.png`](../media/evidence/03-objective-complete.png); video; this README/evidence page. |

## Runtime verification facts

The final standalone runs produced these observed log transitions:

```text
HeroSpell: equipped primary spell BP_ShooterWeapon_Rifle_C
ArcaneTrial: level spawns 2 enemies, goal 2
ArcaneTrial: enemies defeated 1 / 2
ArcaneTrial: enemies defeated 2 / 2 - COMPLETE
PortalAbility: blink 987 cm to walkable ground, cooldown 6.0s started
PortalAbility: rejected (cooling down), cooldown remaining 4.7s
PortalAbility: cooldown complete, ready
PortalDemo: rotated destination arrow to yaw 90
Portal: Portal_BP_C_0 -> Portal_BP_C_1, portal yaw 0 -> 90,
        velocity V(X=694.32, Z=-61.68) -> V(Y=694.32, Z=-61.68)
```

Windows OCR verified these on-screen states from recorded frames:

```text
ARCANE TRIAL ... AI ENEMIES 0/2
ARCANE TRIAL COMPLETE 2/2 ENEMIES DEFEATED
Q PORTAL 5.4s
Q PORTAL READY
```

## Video timeline

| Time | Visible evidence |
|---|---|
| 0:00-0:04 | Native objective, portal-ready text, crosshair, starting spell |
| 0:04-0:14 | Spell fire, blue impact effects, enemy-only X hit marker |
| 0:14-0:24 | Arcane Trial progress and `COMPLETE 2/2` |
| 0:24-0:30 | Q blink, visible cooldown, rejected retry, ready state |
| 0:30-0:38 | Rotated linked-portal traversal through the real overlap path |

## Verification boundary

- Confirmed: source build, standalone run, visible HUD states, player-only AI objective completion, Q cooldown lifecycle, starting spell, rotated portal transform/velocity mapping, and labeled gameplay video.
- Source-confirmed but not time-lapse captured: the decal call receives `DecalLifeSeconds = 8.0f`.
- Not produced: a packaged Windows build. The editor target and standalone game were used for verification.
- Development-only evidence helpers are enclosed in `#if !UE_BUILD_SHIPPING`; they rotate the camera or start a real portal overlap for repeatable capture. The gameplay systems themselves do not depend on those helpers.

## Media integrity

- Final video: H.264, 1280x720, 24 fps, 38.291667 s, 3,812,734 bytes.
- Video SHA-256: `7baf2ffa5ab38cb1e085194b8b7b031ce1b4088b214f2a757888a006383ac6db`.
