# Quest 3 delivery acceptance | Grand Prix Time Machine v0.15

**Release status:** The November 8, 2026 v0.15 public Actions run
[42486791626](https://github.com/lasmith1689-sys/iphone-native-game-ports-public/actions/runs/42486791626)
passed source tests, Godot headless startup, Android ARM64 APK export, Android signature verification and Windows installer packaging.
These are **build/CI gates only**. No physical Quest 3 rendering, controllers, frame pacing or passthrough have been verified.

## Automated release gates (required for each new version)

1. Run the entire Python test suite; report actual passing counts, not a historical count.
2. Godot 4.6.3 headless import/parser and scene startup: reject script/runtime errors.
3. Export a signed Android ARM64 OpenXR APK and inspect its manifest and ARM64 native libraries.
4. Verify the Android APK cryptographic signature with `apksigner`, and SHA-256 checksum.
5. Package and integrity-check the ZIP containing APK, checksums, `INSTALL_ON_QUEST3.cmd`, `INSTALL_ON_QUEST3.ps1`, `READ_ME_FIRST.txt`, and the optional `VERIFY_QUEST3_STARTUP.cmd/.ps1`.
6. Verify the exact commit SHA of the passing public workflow on `quest3/grand-prix-time-machine`; do not touch `main` or the private `project-hq` repository.

## Physical Quest 3 acceptance — NOT YET PERFORMED

The following must be observed on a real Quest 3 before claiming headset-tested operation.
Keep results in a locally reviewed, redacted checklist. Do not automatically upload raw Android logs.

1. Enable Developer Mode and authorize USB debugging on the headset. Run the Windows installer using a USB-C data cable. Verify the app appears in the headset's installed/unknown-sources library.
2. Disconnect USB/PC and launch the app **standalone**. Confirm both-eye rendering, stable orientation, readable UI and no immediate crashes. A five-second Android PID check alone is insufficient.
3. Run `VERIFY_QUEST3_STARTUP.cmd` if needed. It produces a local-only report. Review it before sharing. The tool confirms only launch/process survival, not display, tracking or frame rate.
4. Both controllers: tracking-dependent rays, six selectable buttons, hover feedback and trigger press. Ensure simultaneous triggers never double-activate an action; losing tracking hides the ray and resets latches.
5. Timeline: hold trigger to scrub, release to pause. A/X toggles playback; B/Y recenters; either thumbstick left/right advances one chapter per fresh flick. Up/down adjusts diorama height in 8 cm steps, clamped to ±32 cm; diagonal inputs must choose only the dominant axis.
6. Scaling and recentering must leave controls readable/reachable. Test seated and standing-still use, with Quest boundary enabled. Do **not** physically walk around the enlarged circuit expecting room geometry collision.
7. Start at chapter 1 and watch all nine chapters. At 138 presentation seconds the exhibit must stop on **WORLD CHAMPION** and show **REPLAY READY**; PLAY or A/X restarts. A manual pause must resume rather than restart.
8. Confirm historical narrative: Andretti won Fuji; Depailler second; Hunt third and 1976 champion by one point; Lauda withdrew after two laps. The track, vehicle paths, weather visuals and presentation seconds remain illustrative, not reconstructed telemetry.
9. Record actual observed frame rate and headset thermals if suitable on-device tooling is available. Target 72+ FPS. The in-app frame-timing diagnostic is **not** compositor FPS and is not sufficient by itself.
10. Opaque immersive VR must work reliably. Genuine Meta passthrough is **not shipped**: the official OpenXR Vendors plugin previously crashed CI. MR requires a separately validated plugin integration, Android export and actual headset passthrough test.
11. Reinstall an older debug-signed build if necessary. A signing mismatch must require explicit typed `REPLACE` permission before uninstalling the previous app and deleting its local data.

## Release language

- **CI-verified / installable APK**: allowed only after the exact version's passing public workflow and artifact checks.
- **Quest-installed / launched**: allowed only with observed real-device evidence.
- **Quest-rendered / controller-tested / performance-tested**: requires separate observed physical-device evidence for each claim.
- **Mixed reality / passthrough works**: requires actual passthrough integration and a successful physical Quest 3 test.

Do not promote a speculative MR build over the working immersive VR fallback.
