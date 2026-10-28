# Native iPhone game ports

Dedicated project for native ARM64 iPhone research on Call of Duty (2003), Batman: Arkham Asylum GOTY, Middle-earth: Shadow of Mordor (2014), and Sleeping Dogs Definitive Edition.

**No complete game app or installable game IPA is available yet.** Component builds and Simulator fixtures do not establish gameplay or physical-device readiness.

The approach is static recovery and ahead-of-time native reconstruction. PC streaming and runtime x86 interpretation are outside this project.

Current builds:

- [Encrypted CoD host SDK build](https://github.com/lasmith1689-sys/iphone-native-game-ports-public/actions/runs/40741607802): 5,188 recovered native entries; source and returned diagnostics travel as authenticated ciphertext. This run does not execute a game.
- [Graphics v6 iPhone SDK verification](https://github.com/lasmith1689-sys/iphone-native-game-ports-public/actions/runs/40741821305): native UIKit/EAGL bridge and pinned MIT GL4ES, including indexed draws and two bounded scalar capability queries.
- [Graphics v6 Simulator execution](https://github.com/lasmith1689-sys/iphone-native-game-ports-public/actions/runs/40744937809): 86 calls, eight expected rejections, both scalar queries matched the actual backend, and all 16,384 pixels matched across four self-authored frames. The earlier run 40741839590 timed out; the revised bounded report-file transport completed normally. This is component execution on Apple Software Renderer.
- [External payload ownership SDK and Mac execution](https://github.com/lasmith1689-sys/iphone-native-game-ports-public/actions/runs/40749274279): six actual pthread/mmap cases, including four owners, OS-exit cleanup, external mapping lifetime and allocation/cleanup/retirement failure retention. The unsigned iPhone library is 67,112 bytes. Source engine FLS integration remains incomplete.
- [Graphics v7 iPhone SDK verification](https://github.com/lasmith1689-sys/iphone-native-game-ports-public/actions/runs/40749329080): adds a bounded binary64 depth-clear binding. [Simulator attempt 2](https://github.com/lasmith1689-sys/iphone-native-game-ports-public/actions/runs/40749357470/attempts/2) passed all 24,576 pixels across six frames and all three actual depth references. The first attempt timed out during test-app installation before app execution; that failure is preserved.

Public platform fixtures and transport tooling use isolated branches. Recovered source is encrypted. Retail assets, saves and Apple signing keys are excluded. The build key belongs only to this repository; this project is separate from GoWPad.

Next: finish actual engine startup, input/render/audio integration, then a signed app and physical iPhone gameplay verification.
