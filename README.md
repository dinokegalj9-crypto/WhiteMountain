# White Mountain (prototype)

UE5 C++ prototype of shinobi traversal and escape scoring. Untested: written without access to Unreal, expect a few compile fixes on first build. Croatian setup guide: `UPUTE.md`.

## Source map (`Source/WhiteMountain`)

| File | Role |
|---|---|
| `ShinobiCharacter` | player: camera, run/sprint/crouch, runtime Enhanced Input bindings, hard falls + ukemi |
| `ShinobiTraversalComponent` | climb (tagged `Climbable`), ledge hang/shimmy, mantle/vault, wall jump, rope, rope ladder, kaginawa, tightrope balance, stamina |
| `ShinobiRope` / `ShinobiGrapplePoint` / `ShinobiTightrope` | traversal actors; none touch the navmesh |
| `EscapeScoreComponent` | escape points, chain multiplier, move unlocks |
| `ShinobiTrainer` | village master (Traversal / Weapons / Herbs) that sells moves |
| `ShinobiEnemy` / `ShinobiEnemyController` | guard types and Patrol → Chase → Frustrated/Search → Return; gives up when the navmesh has no (or an absurd) route |
| `ShinobiHUD` | Canvas HUD, no UMG needed |
| `TraversalTestCourse` | grey-box course with 7 stations, guards, ropes and a master |

Tags: `Climbable` on an actor or component makes a wall climbable; `NoClimb` forbids climbing and ledge grabs.
