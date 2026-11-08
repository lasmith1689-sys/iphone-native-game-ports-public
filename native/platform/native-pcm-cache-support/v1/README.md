# PCM/cache support iPhoneOS SDK compile

This branch candidate contains only self-authored PCM, WAV and cache-registry support code, headers, a bounded compile driver and a source-hash manifest. It contains no recovered engine translation units, retail assets, saves, app signing data, Apple credentials or device records.

The workflow compiles five support C units twice with the pinned Xcode 27.1/iPhoneOS SDK 27.1 toolchain and preserves the object files, `.su` stack reports and command receipts for one day. Matching object and stack-report bytes are required. It does not link an engine or execute the game.

The source manifest pins ten C/header files at 34,814 bytes. The private SDK input separately contains the three recovered source-observer units; those stay out of this public support-only branch.

## Evidence limits

A passed run will establish that these five self-authored support units compile to repeatable ARM64 iPhoneOS SDK objects and will report their compiler stack frames. It will not establish integration with recovered engine objects, complete call depth, cache-guard catalog admission, an app link, signing, device installation or gameplay.
