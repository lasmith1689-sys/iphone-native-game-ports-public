# Grand Prix Time Machine | Quest 3 public build branch

This branch contains only the standalone Quest 3 Godot project and its build automation.
The default branch of this repository is unchanged.

The **Fuji 1976** scene features a miniature racing diorama, original low-poly cars,
six sourced historical chapters, OpenXR support, and a Windows one-click installer
script. Racing positions and circuit geometry are illustrative rather than
historically reconstructed telemetry.

**Build state:** the [successful October 29 public CI run](https://github.com/lasmith1689-sys/iphone-native-game-ports-public/actions/runs/40945928592)
exported and debug-signed an ARM64 Android APK, and packaged it with a one-click
Windows installer. Its artifact is named `GrandPrixTimeMachine_Quest3_v0.5_Easy_Install`.
The 69 automated source tests passed. Headset startup, controller tracking and
passthrough performance still require physical Quest testing. Do not sideload a
source-only ZIP; use the **Easy Install** archive containing an actual APK.

See [Quest 3 installation steps](GrandPrixTimeMachine_Quest3_Godot/install/READ_ME_FIRST.txt)
and [device verification checklist](GrandPrixTimeMachine_Quest3_Godot/docs/QUEST_ACCEPTANCE.md).
