# Meta Quest passthrough: experimental build failure, 1 November 2026

## Purpose
A real-world Quest 3 passthrough experience remains a goal, not a shipped
feature. The v0.8 **release** is safe immersive VR; experimental MR
toolchain changes were not promoted to a download.

## Reproduction (public Actions)
- Official Godot 4.6.3 runtime, `barichello/godot-ci:4.6.3`,
  ubuntu-24.04 container.
- Downloaded official `GodotVR/godot_openxr_vendors` binary addons
  at versions 5.1.0 and 5.0.1 and validated their GitHub-published
  SHA-256 digests before loading.
- Installed under `addons/godotopenxrvendors`, enabled
  `xr/openxr/extensions/meta/passthrough`, configured Android Gradle export.
- Both addon versions loaded project scripts and then caused a fatal editor
  shutdown on `godot --headless --editor --path . --quit --verbose`:
  `FATAL: Index p_index = 1 is out of bounds (size() = 0)`, followed by
  `Class 'MetaEditorExportPlugin' is not exposed` and SIGSEGV (exit 132).
- Source tests passed; native export and physical headset were **not**
  reached in either MR experiment.

Evidence:
- [Version 5.1.0 failed build](https://github.com/lasmith1689-sys/iphone-native-game-ports-public/actions/runs/41316542248).
- [Version 5.0.1 failed build](https://github.com/lasmith1689-sys/iphone-native-game-ports-public/actions/runs/41316919754).

References:
- https://godotvr.github.io/godot_openxr_vendors/manual/meta/passthrough.html
- https://github.com/GodotVR/godot_openxr_vendors/releases/tag/5.1.0-stable
- https://github.com/GodotVR/godot_openxr_vendors/releases/tag/5.0.1-stable

## Next experiments (separate from release)
1. Reproduce with a minimal plugin-only project under a Godot 4.6.3
   GUI editor runner (Xvfb if required); collect symbols/backtrace.
2. Test alternative Godot/plugin versions on minimal source.
3. Promote only after plugin initialization, Android Gradle export,
   manifest checks and signing succeed; retain VR fallback.
4. Confirm true passthrough, controller interactions and boundaries
   on actual Quest 3 before calling the MR feature device-tested.

No actual Quest headset runtime was tested during these experiments.
