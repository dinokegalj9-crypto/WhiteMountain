# White Mountain (prototype)

UE5 C++ prototype of shinobi traversal and escape scoring. Untested: written without access to Unreal, expect a few compile fixes on first build. Croatian setup guide: `UPUTE.md`.

## Source map (`Source/WhiteMountain`)

| File | Role |
|---|---|
| `ShinobiCharacter` | player: camera, run/sprint/crouch, runtime Enhanced Input bindings, hard falls + ukemi, takedowns (E Silent Sleep 4 s silent, F Pressure Strike 1 s noisy) |
| `ShinobiTraversalComponent` | climb (tagged `Climbable`), ledge hang/shimmy, mantle/vault, wall jump, rope, rope ladder, kaginawa, tightrope balance, stamina |
| `ShinobiRope` / `ShinobiGrapplePoint` / `ShinobiTightrope` | traversal actors; none touch the navmesh |
| `EscapeScoreComponent` | escape points, chain multiplier, move unlocks |
| `ShinobiTrainer` | village master (Traversal / Weapons / Herbs) that sells moves |
| `ShinobiEnemy` / `ShinobiEnemyController` | guard types and Patrol → Chase → Frustrated/Search → Return; gives up when the navmesh has no (or an absurd) route; can be taken down from behind, lies unconscious, calm guards who spot a body investigate |
| `ShinobiInteractable` | base for things used with E; the closest usable one wins |
| `ShinobiEscapeRoute` | ordered checkpoints; calm runs are rehearsals (max 3), chased runs pay a bonus scaled by rehearsals |
| `ShinobiOilTrap` / `ShinobiDropLog` | escape traps: oil that knocks chasing guards down; a log ramp pushed down from the roof, which removes the guards' navmesh route |
| `ShinobiAlarmObjective` | item to steal; raises the alarm for nearby guards |
| `ShinobiArrow` | cosmetic arrow; archers (`ShinobiEnemyController::TickBow`) aim, then roll hit chance (distance, sprint, crouch, traversal, route rehearsal) |
| `ShinobiHUD` | Canvas HUD, no UMG needed |
| `TraversalTestCourse` | grey-box course with 8 stations (7 = town escape), guards, ropes and a master |

Tags: `Climbable` on an actor or component makes a wall climbable; `NoClimb` forbids climbing and ledge grabs.
