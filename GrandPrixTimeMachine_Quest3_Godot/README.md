# Grand Prix Time Machine — v0.15 Quest 3 standalone build

This is a **Godot 4.6.3 OpenXR port** of the prototype, created because this environment
cannot install or activate Unity. The original Unity v0.2 project remains untouched.

## App

The program creates a stylized 1976 Japanese GP diorama, four original 1970s open-wheel
car models, a nine-chapter documentary playback and a floating action-button interface.
**v0.15 remains a VR-first release.** Its default CI builds a standalone
Quest 3 OpenXR APK without the OpenXR Vendors plugin, which is **not yet bundled**
because native versions 5.1.0 and 5.0.1 crashed the Linux Godot 4.6.3 headless
editor while importing. **MR unverified:** actual Meta passthrough has not been
integrated or physically tested. See `docs/MR_CI_BLOCKER.md` for the exact
reproduction and safe recovery plan. The scene has no furniture anchoring.
The target is a native, on-headset Android ARM64 app with no PCVR streaming or
network requirement. v0.5 and v0.6 signed APKs were built October 29 and 30, 2026. Each new version requires its own passing public GitHub Actions run before delivery.

**The circuit, car positions, model details and playback time are illustrative, not
reconstructed historical telemetry.** Narrative beats are sourced; see
`docs/HISTORICAL_SOURCES.md`.

## New in v0.15: the exhibit reaches a deliberate ending

The 138-second editorial presentation now plays **once from the opening rain delay
through the championship conclusion**. It no longer wraps silently back to the
start, which could make the historical story feel like an endless loop. At the
end, the final World Champion chapter remains visible and playback stops.
The state display changes to **REPLAY READY**. Press PLAY or A/X to return to
the opening chapter and begin a fresh run. A manual pause before the ending still
resumes from the same position.

The nine chapter beats and illustrative race animation are unchanged. This is a
playback and exhibit-flow improvement, not a claim of lap-accurate telemetry.

## Previously in v0.14: native XR frame pacing, focus-safe replay and app FPS diagnostics

Following official Godot 4.6 guidance, desktop VSync is disabled only when
OpenXR successfully initializes. On `session_begun`, Godot's physics tick rate
is synchronized to the runtime-reported display refresh rate if it is plausible
(60–144 Hz). An unsupported report leaves defaults alone; the app does not
force a new headset refresh mode.

When the headset loses focus (e.g. opening Quest system menu or removal), replay
pauses, seeks cancel and both controller latches reset. Playback resumes on
refocus *only if it was playing before focus loss*.

The in-headset diagnostic shows app-loop FPS after warmup, and occasional
`GPTM PERF` log lines record sampled FPS, worst frame interval and runtime
refresh rate. This is **not** compositor FPS or proof of headset performance.
The logs contain no device serials and are never uploaded automatically.
These changes have **not** been tested on a physical Quest 3.

References:
- https://docs.godotengine.org/en/4.6/tutorials/xr/setting_up_xr.html
- https://docs.godotengine.org/en/4.6/tutorials/xr/a_better_xr_start_script.html

## Previously in v0.13: local headset startup diagnostic and improved historical precision

The Easy Install ZIP includes an optional **VERIFY_QUEST3_STARTUP.cmd** on Windows.
After installing and authorizing USB debugging, double-click this to launch the
app via ADB and check whether its process still exists five seconds later.
It generates a timestamped **local-only** report from the app's PID-filtered
Android logs, with no automatic upload. This check does **not** prove visible
rendering, controller action mapping, safety boundary handling or passthrough.
Review any report before sharing it.

The lap-68 chapter now specifically notes Hunt's left-front tyre deflation,
and the fightback identifies Regazzoni and Jones, following a contemporary
December 1976 Motor Sport report in docs/HISTORICAL_SOURCES.md.
No game positions are presented as historical telemetry.

## Previously in v0.12: seated-height comfort and live XR status

Move either controller thumbstick **up/down** to raise or lower the miniature by
8 cm per fresh flick, clamped to ±32 cm. Left/right still step one chapter.
The stronger axis wins on diagonal motion to reduce accidental skips.
On desktop preview use Up/Down arrows to test the same feature.

An in-headset status line updates four times per second with VR mode,
left/right controller tracking and board elevation. It can help diagnose
tracking loss on real hardware without needing a PC screen. This is a
**runtime diagnostic, not evidence that Quest 3 hardware has been tested**.
The label says MR (UNVERIFIED) only if a passthrough environment reports
active. True Meta passthrough is **not bundled or certified** in this build.

## Previously in v0.11: accessible controller navigation and light rain

Press **A or X** to toggle playback without needing to aim at a tiny target.
Flick either controller thumbstick left or right to skip one historical chapter. Flick up or down to raise or lower the diorama.
Release it back near the center before another chapter step. Each hand has a
separate edge latch, and only one action can occur per rendered frame. Trigger
selection and B/Y recenter still work, and the visible help now mentions the
shortcuts.

Rain now moves across the early Fuji story chapters through one **48-instance
MultiMesh** (no imported textures, particle plugin or third-party assets).
Rain transforms update at most 25 times per second while playback runs, freeze
when paused, and turn off in later drying-track chapters. This procedural rain
is an aesthetic illustration, not a weather reconstruction.

## Previously in v0.10: race director

Nine researched beats now cover the rain-delayed start, Lauda's withdrawal,
drying asphalt, tyre trouble, Andretti's lead, Hunt's lap-68 stop and the
title fightback. Track shading and an on-screen title-stakes ticker follow
the editorial story. Hunt briefly stops in a stylized pit-lane vignette; this
is NOT historical vehicle telemetry or lap-by-lap racing.

## Previously in v0.9: interactive driver spotlight

Point either tracked controller at a moving miniature car and press trigger to
view official Fuji 1976 driver, team, race number, result and lap count. The
selected car gets a gold marker. The viewer never moves or chases a car,
avoiding artificial locomotion. Press **D** on desktop to cycle driver cards.
Official results are factual, but car motions and track shape are illustrative.

## Controls

- Aim EITHER controller at a moving car for its driver card, or at the six colored buttons, and press trigger. Each press selects one action. Hovered buttons enlarge and the hint names the hovered action. Simultaneous presses invoke at most one action per frame.
- Press B or Y to recenter. CENTER is also a visible button.
- Press A or X to pause/play, or flick either thumbstick left/right for one previous/next chapter step. Flick up/down to adjust diorama height; the floating controls stay at a fixed distance from your head.
- The pointer hides when tracking is lost. Keep a safe Quest boundary.
- Aim at the diorama’s gold timeline, hold trigger to scrub, and release to pause.
- The fixed-scale floating control rail remains accessible when the diorama expands.
- The documentary plays once; at its end, PLAY or A/X restarts from the opening chapter.
- Green = play/pause; grey = previous chapter; gold = next chapter;
  blue = expand/shrink; red = 1x/2x/4x speed.
- In desktop preview: Space/N/P/M/F/R/D = pause/next/previous/scale/speed/recenter/next driver; Up/Down arrows adjust diorama height.
- The virtual model DOES NOT COLLIDE with real furniture or people. Remain seated or
  standing still, use Guardian, and never walk around the enlarged circuit expecting
  your physical space to be clear.

Hold a controller trigger while aiming at the gold timeline to seek directly
to any part of the story. Releasing the trigger leaves playback paused.
The v0.8 miniature adds a low-cost checkerboard start/finish marker.
Hand tracking and room surface anchoring are **not implemented** in this port.
The race display now shows 73 laps, official 1976 entrant numbers (Andretti 5, Depailler 4, Hunt 11, Lauda 1), and a stylized six-wheeled Tyrrell for Depailler. The procedural Fuji track layout and race-car positions remain illustrative.

## Build

The workflow `.github/workflows/grand-prix-quest3.yml` runs source tests, imports the Godot
project, exports a debug-signed ARM64 Quest APK with OpenXR, then verifies its
signature and packs the verified APK with the Windows one-click USB installer.
The successful CI artifact is `GrandPrixTimeMachine_Quest3_Easy_Install.zip`,
which contains `GrandPrixTimeMachine_Quest3.apk`, checksums and a Windows installer.
**Earlier APKs were produced; each new release still requires a green CI build.** Public CI is configured to build on
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
one Easy Install ZIP, including the optional Windows headset-startup diagnostic. Developer Mode must be enabled once, then the user
can unzip it, connect their Quest with a USB-C data cable, double-click the
launcher and accept USB debugging on the headset. The script checks the SHA-256,
downloads official Google Android platform-tools if needed, installs and
verifies `com.gptimemachine.quest3` automatically. For a debug-signature mismatch, it offers an optional typed REPLACE confirmation before removing the old app and its local data. For macOS, use Meta Quest
Developer Hub or SideQuest Advanced Installer. No Unity, Godot or tethering is
required to run the installed application. **Do not install source ZIPs.**

Advanced manual fallback with Android SDK installed: `adb install -r GrandPrixTimeMachine_Quest3.apk`.
