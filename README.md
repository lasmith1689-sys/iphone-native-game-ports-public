# Grand Prix Time Machine | public Quest 3 development branch

This isolated public branch contains a native standalone Godot 4.6.3 Quest 3
historical racing exhibit. The repository's default branch remains unchanged.

The [first verified v0.5 build](https://github.com/lasmith1689-sys/iphone-native-game-ports-public/actions/runs/40946624259)
passed Android APK export, apksigner validation, 69 source tests and Windows
one-click installer packaging on October 29, 2026.

**v0.6** adds independent left/right controller rays, hover indication,
six more readable controls including recenter, 14 low-poly trees and reduced
room-scale growth to help keep the exhibit comfortable. See the
[Actions history](https://github.com/lasmith1689-sys/iphone-native-game-ports-public/actions/workflows/grand-prix-quest3.yml)
for the newer `GrandPrixTimeMachine_Quest3_v0.6_Easy_Install` artifact
**only after that build succeeds**. Source ZIPs are not installable APKs.

[Project README](GrandPrixTimeMachine_Quest3_Godot/README.md) |
[Windows installation](GrandPrixTimeMachine_Quest3_Godot/install/READ_ME_FIRST.txt) |
[Physical Quest acceptance](GrandPrixTimeMachine_Quest3_Godot/docs/QUEST_ACCEPTANCE.md) |
[Historical research](GrandPrixTimeMachine_Quest3_Godot/docs/HISTORICAL_SOURCES.md)

**v0.8** adds controller timeline scrubbing and a checkerboard finish line.
This is a **VR-first release**, not MR: the OpenXR Vendors plugin crashed
the Godot 4.6.3 Linux headless editor in both the 5.1.0 and 5.0.1
experiments, so release CI keeps its previous verified direct-APK export.
See [the MR integration blocker](GrandPrixTimeMachine_Quest3_Godot/docs/MR_CI_BLOCKER.md).
Physical headset startup, controllers and passthrough remain untested.
All geometry and on-track animations are illustrative.

**v0.11 source changes:** A/X playback shortcut, per-hand thumbstick chapter
navigation, and a small instanced rain effect for the early Fuji 1976 chapters.
A passing public CI job verifies compilation, not physical Quest 3 operation.
