# Quest 3 delivery acceptance | Grand Prix Time Machine v0.7

**Source-only packages are not installable.** The actual deliverable is
a signed Android ARM64 APK in a Windows Easy Install ZIP.

The Oct 29 2026 [v0.5 build](https://github.com/lasmith1689-sys/iphone-native-game-ports-public/actions/runs/40946624259)
passed 69/69 automated checks, Godot parser, headless startup, Android export,
apksigner and ZIP integrity. v0.6 also passed public CI on October 30, 2026.
v0.7 requires its own green source/test and Android build before release.

## Automated release gates

- Python unit tests: source, history, geometry, independent controller latches,
  scene/hand aim poses, Android export presets and installer.
- Godot 4.6.3 editor import/parser, and startup smoke test must show no script
  errors. (Neither proves XR hardware behavior.)
- Export ARM64 APK, run Android verifier and Android apksigner, calculate SHA256.
- Package installer CMD, PS1, installation guide, APK and checksum into
  an integrity-checked ZIP.
- Build job depends on passing Python source tests.

## Physical Quest 3 acceptance — NOT YET PERFORMED

1. With Developer Mode freely enabled once, sideload via Windows USB-C.
2. Launch entirely on Quest without a PC/Quest Link.
3. Both controllers show rays only when tracked and activate all six buttons; simultaneous presses should not double-skip.
4. Holding trigger must NOT repeatedly skip chapters. B/Y recenters the model.
5. SCALE keeps the interface readable and reachable; performance 72+ FPS.
6. Story correctly notes Andretti won the race and Hunt was third to become
   champion; virtual racing is illustrative, not accurate telemetry.
7. Opaque VR must work. True Quest passthrough requires OpenXR Vendors plugin
   and device verification; not implemented or confirmed in this version.
8. Reinstall upgrade: debug signing keys can differ between CI runs. Require typed REPLACE permission before uninstalling old app (and deleting its local data).

**Never claim physical headset, controller tracking or MR testing without
evidence from an actual Quest 3.**
