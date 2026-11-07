# Public build of Grand Prix Time Machine (Quest 3)

This standalone game source is shared publicly to enable GitHub Actions standard runners at no charge. The original `project-hq` repository remains private; no unrelated project files or history are included in this branch's tree.

This is an independently maintained branch of a pre-existing public game repository, not the default branch or a separate standalone repository. The host repository's default branch is untouched. Source is visible publicly, but no explicit open-source redistribution license is granted by this publication.

Current state: successful public GitHub Actions builds produce signed ARM64 APKs and Windows Easy Install ZIPs. Download the artifact from the latest successful `Quest 3 APK / Fuji 1976` run **on this branch**, extract its inner `GrandPrixTimeMachine_Quest3_Easy_Install.zip`, and run `INSTALL_ON_QUEST3.cmd` on Windows 11. See `install/READ_ME_FIRST.txt` for Developer Mode and USB permission details. Neither GitHub CI nor the Windows installer proves physical headset rendering, controller functionality or Meta passthrough, which remain unverified.
