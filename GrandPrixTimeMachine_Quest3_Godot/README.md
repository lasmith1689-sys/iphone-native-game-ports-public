# Grand Prix Time Machine — v0.6 Quest 3 standalone build

This is a **Godot 4.6.3 OpenXR port** of the prototype, created because this environment
cannot install or activate Unity. The original Unity v0.2 project remains untouched.

## App

The program creates a stylized 1976 Japanese GP diorama, four original 1970s open-wheel
car models, a six-chapter documentary playback and a floating action-button interface.
In Quest, it attempts passthrough if the OpenXR runtime exposes alpha blending,
otherwise it uses a dark immersive VR environment. **Meta Quest passthrough also
requires the Godot OpenXR Vendors plugin, which is not yet bundled in this
prototype. Therefore current builds must be regarded as VR-first, with MR unverified.**
The target is a native, on-headset Android ARM64 app with no PCVR streaming or
network requirement. The v0.5 APK was built October 29, 2026. Only an independently successful v0.6 GitHub Actions run proves the updated APK was built.

**The circuit, car positions, model details and playback time are illustrative, not
reconstructed historical telemetry.** Narrative beats are sourced; see
`docs/HISTORICAL_SOURCES.md`.

## Controls

- Aim EITHER controller at the six colored buttons and press trigger. Each press selects one action. Hovered buttons enlarge for visual feedback.
- Press B or Y to recenter. CENTER is also a visible button.
- The pointer hides when tracking is lost. Keep a safe Quest boundary.
- The fixed-scale floating control rail remains accessible when the diorama expands.
- Green = play/pause; grey = previous chapter; gold = next chapter;
  blue = expand/shrink; red = 1x/2x/4x speed.
- In desktop preview: Space/N/P/M/F/R = pause/next/previous/scale/speed/recenter.
- The virtual model DOES NOT COLLIDE with real furniture or people. Remain seated or
  standing still, use Guardian, and never walk around the enlarged circuit expecting
  your physical space to be clear.

Hand tracking and room surface anchoring are **not implemented** in this port.
The procedural Fuji track layout and race-car positions remain illustrative.

## Build

The workflow `.github/workflows/grand-prix-quest3.yml` runs source tests, imports the Godot
project, exports a debug-signed ARM64 Quest APK with OpenXR, then verifies its
signature and packs the verified APK with the Windows one-click USB installer.
The successful CI artifact is `GrandPrixTimeMachine_Quest3_Easy_Install.zip`,
which contains `GrandPrixTimeMachine_Quest3.apk`, checksums and a Windows installer.
**An APK has NOT yet been produced.** Public CI is configured to build on
source changes without consuming the exhausted private-repo minute allowance.
Review `docs/QUEST_ACCEPTANCE.md` before describing it as headset-ready.

To build on a Godot 4.6.3 + Android SDK machine:

```sh
python3 -m unittest discover -s tests -p "test_*.py" -v
godot --headless --editor --path . --quit
godot --headless --path . --export-debug Quest3 build/GrandPrixTimeMachine_Quest3.apk
```

A compiled APK MUST pass install, startup and performance checks on a Quest 3 before
we describe the app as device-tested. CI can validate compilation but cannot
emulate the headset sensors, passthrough compositor or controller actions.

## Easy Quest 3 installation (when the real APK exists)

The project includes `install/INSTALL_ON_QUEST3.cmd`, a Windows 11 launcher,
`install/INSTALL_ON_QUEST3.ps1`, and `install/READ_ME_FIRST.txt`. When cloud
compilation succeeds, CI will bundle these **together with the signed APK** in
one Easy Install ZIP. Developer Mode must be enabled once, then the user
can unzip it, connect their Quest with a USB-C data cable, double-click the
launcher and accept USB debugging on the headset. The script checks the SHA-256,
downloads official Google Android platform-tools if needed, installs and
verifies `com.gptimemachine.quest3` automatically. For macOS, use Meta Quest
Developer Hub or SideQuest Advanced Installer. No Unity, Godot or tethering is
required to run the installed application. **Do not install source ZIPs.**

Advanced manual fallback with Android SDK installed: `adb install -r GrandPrixTimeMachine_Quest3.apk`.
