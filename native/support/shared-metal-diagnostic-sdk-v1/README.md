# Owned shared Metal diagnostic shell

Fresh infrastructure for an eventual native game shell. Every source file here is newly authored. No game code, translated engine archive, recovered shader, retail asset, guest callback, account access, network operation, signing, or installation belongs to this closure.

## Behavior and authority

The app creates one retained scene/window/controller and offers one explicit **Run one synthetic Metal frame** action. It creates a real Metal device, queue, pipeline, 64x64 private texture, shared readback buffer, and CAMetalLayer drawable. The owned shader draws a green triangle over a blue clear in two render passes: the fixed offscreen texture and the admitted drawable. A blit copies only the offscreen texture to the CPU buffer. It checks BGRA bytes at the top-left corner and center, after successful GPU completion. These are two sample checks, not an image-wide oracle or a readback of the presented drawable.

Command completion and the drawable's `addPresentedHandler` are independent observations. Scheduling `presentDrawable:` does not set either outcome. Combined success needs both callbacks, successful GPU status, matching offscreen samples, and a finite positive `presentedTime`. A system presentation callback does not establish that a human saw correct pixels, or that a game rendered anything. Both callback orders are supported.

UIKit capture, policy state, encoding admission, commit, and reports use the main thread. Metal callbacks capture their result and dispatch back to that authority; they do not read UIKit state on a Metal thread. Start/commit require the same foreground-active scene and key window, root controller attachment, identity transforms, positive finite integral drawable dimensions, matching bounds/scale, known orientation, and a bounded pixel area. Scale comes from `scene.traitCollection.displayScale`. Any observed layout/scene/foreground change after starting cancels the lifetime, including after an earlier success; the report preserves the earlier event. No restart/resume exists. Before the explicit start, ordinary foreground transitions do not consume the attempt.

The 10-second observation deadline fails/cancels a still-pending attempt; it does not invent a GPU completion or cancel work already submitted. Resources and scene objects remain retained until process exit, including on refusal, backgrounding, disconnect, timeout, or late callbacks. This deliberate diagnostic retention is not a production teardown solution. There are no repeated frames, display-link loop, render worker thread, audio, file import, engine entry, idle-timer mutation, or game ABI claims.

The UI and local `Documents/metal-diagnostic.json` report each event (bounded to 32). JSON records distinguish a committed command from completion and presentation. A write error is logged; it does not alter observed GPU results. Reporting is local only. UIKit lifecycle callbacks and geometry checks cannot prove external window occlusion or human visibility.

## Exact closure and local verification

`source-manifest.json` pins the explicitly listed owned files. `verify_manifest.py` verifies them without discovery. The app compile/link closure is exactly `main.m`, `shell_policy.c`, `shell_policy.h`, `diagnostic.metal`, and `Info.plist`; support scripts and tests are separately included in the same manifest. No additional native library is linked. Framework dependencies are UIKit, Foundation, Metal, QuartzCore, and CoreGraphics, plus the Apple runtime/system libraries selected by clang.

Run the portable proof into an absent directory:

```text
python3 test_policy.py --out /absolute/fresh-policy-output --cc /path/to/native/clang
```

The optional `--cc` defaults to PATH clang/cc. `test_shell_policy.c` covers malformed and changed geometry, single-attempt consumption, both callback orders, cancellation before/after commit, late callbacks, GPU/pixel/presentation failures, duplicate observations, observation deadline failure, and exact BGRA sample/size checks. It executes only owned portable C. The retained Windows proof is `local-policy-proof-v1/receipt.json`: **214 checks passed** with clang 23.1.2, warnings treated as errors. This does not compile UIKit/Metal or execute a GPU.

## Apple SDK gate — still required

On a separately authorized Mac/Apple SDK runner, with Python 3 and Metal command-line tools installed, run:

```sh
bash build_apple.sh /absolute/fresh-apple-output 26.5
```

The second argument is the **exact expected measured iPhoneOS SDK version**; omitting it expects 27.1. It must match `xcrun --sdk iphoneos --show-sdk-version`. The deployment target stays iOS 26.0. For example, explicit `26.5` admits the previously observed Xcode runner SDK without pretending it is 27.1. The script never downloads missing tools or changes Xcode selection. Source-manifest verification happens first; output must be absent. It compiles the MSL source to AIR and then `default.metallib`; it does not compile shaders dynamically in the app.

Outputs: `SharedMetalDiagnostic.app`, object/AIR files, link map, load-command/import listings, build log, and `receipt.json`. The link disables automatic ad-hoc signing. The receipt independently checks thin ARM64 `MH_EXECUTE`, `LC_MAIN`, absence of `LC_CODE_SIGNATURE`, one actual iOS `LC_BUILD_VERSION` matching requested/measured SDK and iOS 26.0, and system-only dylib dependencies. Bundle SDK metadata comes from the measured SDK and is cross-checked against the actual executable. The receipt hashes the app and other completed artifacts; the concurrently open build log is intentionally not hashed.

The script creates an **unsigned diagnostic app**, not an Xcode archive or distributable IPA. It does not run codesign, select a signing identity, access an account, upload, install, or launch anything. Bundle identifier is a local placeholder; no entitlements or distribution claim is made. Runtime receipt fields remain false after compilation.

Next step in the **current thread**, no new thread required: root runs the exact Apple SDK gate, reviews any real compiler diagnostics and the unsigned receipt, and only then considers a separately authorized diagnostic-device delivery. Physical foreground/geometry behavior, shader pixels, actual presentation, deadline behavior, and retained GPU lifetime remain unverified until real execution. Arkham/Mordor/Sleeping Dogs integrations remain separate work; this shell does not bypass any stopped constructor proof.
