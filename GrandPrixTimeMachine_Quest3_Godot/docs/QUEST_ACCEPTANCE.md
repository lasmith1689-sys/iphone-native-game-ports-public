# Quest 3 delivery acceptance | Grand Prix Time Machine v0.5

**Source is not the deliverable.** The requested deliverable is an Android ARM64
APK that installs and launches on a standalone Meta Quest 3.

## Evidence and gates

| Gate | Evidence required | Current state |
|---|---|---|
| 1. Offline source and track checks | `python -m unittest discover -s tests -p 'test_*.py' -v` green | Passed locally (68/68 on 2026-10-29) |
| 2. Godot 4.6.3 script parser/import | `godot --headless --editor --path . --quit` exits 0 | Not run: missing local Godot executable, cloud job has not started |
| 3. Godot Android debug export | Export command returns 0, emits APK | Blocked: GitHub Actions job failed before execution |
| 4. APK file integrity and ABI | `tools/verify_apk.py` and `unzip -t` pass, arm64 present | Not run: no APK |
| 5. APK signing | `apksigner verify --verbose` succeeds | Not run: no APK |
| 6. Quest installation | `adb install -r` succeeds on physical Quest 3 | Not run: no device access |
| 7. Quest runtime smoke test | opens without crash, centered model visible, 5 control buttons selectable | Not run: no device access |
| 8. VR/MR behavior | VR fallback working, native passthrough separately verified with vendor extension | VR untested, MR extension not bundled |
| 9. Headset rendering and comfort | comfortable scale, no flashing, 72+ fps measured by headset profiler | Not run: no device access |
| 10. Documentary integrity | Official race result cited; no synthetic positions sold as lap telemetry | Passed source review |

## What to test on Quest

1. Install signed debug APK through `adb install -r GrandPrixTimeMachine_Quest3.apk`.
2. Open from Quest App Library / Unknown Sources and verify the scene appears
   in front of the user. Do not require access to a PC after installation.
3. Press the right-controller trigger on **Play**, **Back**, **Next**,
   **Scale** and **Speed**. All five buttons must work, even while expanded.
4. Confirm Lauda's car disappears at the withdrawal chapter, and replay
   returns to chapter 1. Check controls remain near the headset.
5. Disable passthrough or use an XR runtime without it. Confirm immersive
   fallback is visible (not a black scene). For mixed reality, install a
   compatible Godot OpenXR Vendors plugin, enable Meta passthrough in the
   export preset, then separately verify true video background on Quest.
6. Confirm a full race presentation, no tracking loss, no heating or frame
   pacing issues. Do not use real-room walking until Guardian safety is tested.

## Current blocker

GitHub reported on October 22, 2026 that the private project account had
2,000 included Actions minutes were **100% exhausted**, resetting November 1.
A separate email reported all 0.5 GB of included Actions storage exhausted.
Private repo runner jobs, including a minimal independent probe, fail before
the first step and have no logs. This strongly implicates the quota, though
GitHub did not provide a job-specific root cause. The existing automated task
resumes builds after the reset. Do not spend money, enable paid runners, or
make the source repo public to bypass this.

### Installation release gate

The build must yield a real `GrandPrixTimeMachine_Quest3.apk`, pass `apksigner`
and ABI verification, and include `GrandPrixTimeMachine_Quest3_Easy_Install.zip`
containing the signed APK, SHA256SUMS.txt and Windows install scripts. Source
ZIPs and installers without an APK must not be described as installable.
The exact 3D app behavior cannot be certified until physically launched on a
Quest 3. A user-visible startup test will still be required to prove that step.

**Never publish a fake `.apk`, rename source ZIP to APK, or describe this as
headset-tested based on static tests alone.**
