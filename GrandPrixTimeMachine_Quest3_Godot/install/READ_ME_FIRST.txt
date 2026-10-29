GRAND PRIX TIME MACHINE | QUEST 3 EASY INSTALL
=========================================

THIS FOLDER IS A SOURCE/INSTALLER PREPARATION KIT UNTIL IT ALSO CONTAINS:
  GrandPrixTimeMachine_Quest3.apk
  SHA256SUMS.txt
A source ZIP by itself does NOT install on the headset.

ONCE YOU HAVE THE COMPLETED EASY INSTALL ZIP:

FIRST TIME ONLY
  1. Enable Developer Mode for your Quest 3. You need a Meta Horizon
     developer account linked to the same Meta account as your headset.
     On your phone, open the Meta Horizon app, select your headset in
     Devices and turn on Developer Mode. Meta may require verification.
  2. Use any USB-C cable that carries DATA (not charge-only).

INSTALL (Windows 11)
  1. Unzip the entire Easy Install ZIP to a normal folder on your PC.
  2. Turn on your Quest and connect it by USB-C to your PC.
  3. Double-click INSTALL_ON_QUEST3.cmd.
  4. If prompted, put on the headset and allow USB debugging.
  5. The installer downloads Google's Android USB tool on its first run,
     checks the APK SHA-256, installs the app and confirms its presence.
  6. Disconnect USB. In Quest's App Library, open Unknown Sources and
     launch Grand Prix Time Machine (the exact filter location can vary).

NO WINDOWS SCRIPT WANTED?
  Install Meta Quest Developer Hub from Meta (Windows/macOS), pair the
  headset, then use its app install/deploy APK control. The Developer
  Mode and USB authorization requirements are the same.
  https://developers.meta.com/vr/documentation/spatial-sdk/meta-quest-developer-hub/

OR USE SIDEQUEST
  Install SideQuest Advanced Installer from https://sidequestvr.com/ .
  Plug in the Quest, approve debugging, click 'Install APK from folder'
  and select GrandPrixTimeMachine_Quest3.apk. Find it under Unknown Sources.

TROUBLESHOOTING
  * Quest not detected: power it on, use a data cable and another USB port,
    approve the USB debugging prompt inside the headset and verify Developer Mode.
  * Windows USB driver issue: install Meta Quest Developer Hub / Quest Link
    and reconnect the headset.
  * INSTALL_FAILED_UPDATE_INCOMPATIBLE: a previous build has a different
    signature. Uninstall the earlier app in the headset before installing;
    doing so removes that app's saved data.
  * No Unknown Sources filter: check the Apps Library filter/dropdown; Meta
    may move the option in newer Horizon OS versions.
  * Blank or crashing game: installation alone does not prove the game's
    runtime correctness. Capture the error for the developer. Do not walk
    around relying on the virtual circuit as a real-world obstacle map.

No Quest Link, PCVR, Unity or Godot is required after the APK is built.
The Windows installer contacts only Google's official download host when
platform-tools are missing. It does not require administrator privileges.
