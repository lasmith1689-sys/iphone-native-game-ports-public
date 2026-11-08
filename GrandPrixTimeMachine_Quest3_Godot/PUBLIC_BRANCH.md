# Public build of Grand Prix Time Machine (Quest 3)

This standalone Godot 4.6.3 game source is maintained on the isolated public
branch `quest3/grand-prix-time-machine` of this repository. The current
installable milestone is v0.15. Standard public GitHub Actions runners build
the Android ARM64 APK and Windows Easy Install package at no charge.

The original private `project-hq` repository is separate and remains private.
The host repository's default branch is not a development target and must not
be modified or merged into.

Current product status: VR-first standalone OpenXR APK. True Meta passthrough,
room anchoring, and physical Quest 3 controller/rendering acceptance remain
unverified. See `docs/QUEST_ACCEPTANCE.md` and `docs/MR_CI_BLOCKER.md`.

Only the signed APK inside the Easy Install artifact is installable. Do not
install source ZIPs. Windows users extract the Easy Install ZIP and run
`INSTALL_ON_QUEST3.cmd`; Developer Mode and USB debugging authorization are
required. See `install/READ_ME_FIRST.txt`.
