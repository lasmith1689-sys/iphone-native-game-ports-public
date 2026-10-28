# Native iPhone game ports

Dedicated project for native ARM64 iPhone research on Call of Duty (2003), Batman: Arkham Asylum GOTY, Middle-earth: Shadow of Mordor (2014), and Sleeping Dogs Definitive Edition.

**No complete game app or installable game IPA is available yet.** Component builds and Simulator fixtures do not establish gameplay or physical-device readiness.

The approach is static recovery and ahead-of-time native reconstruction. PC streaming and runtime x86 interpretation are outside this project.

Current builds:

- [Encrypted CoD host SDK build](https://github.com/lasmith1689-sys/iphone-native-game-ports-public/actions/runs/40741607802): 5,188 recovered native entries; source and returned diagnostics travel as authenticated ciphertext. This run does not execute a game.
- [Graphics v6 iPhone SDK verification](https://github.com/lasmith1689-sys/iphone-native-game-ports-public/actions/runs/40741821305): native UIKit/EAGL bridge and pinned MIT GL4ES, including indexed draws and two bounded scalar capability queries.
- [Graphics v6 Simulator execution](https://github.com/lasmith1689-sys/iphone-native-game-ports-public/actions/runs/40741839590): self-authored rendering fixture, separate from game execution.

Public platform fixtures and transport tooling use isolated branches. Recovered source is encrypted. Retail assets, saves and Apple signing keys are excluded. The build key belongs only to this repository; this project is separate from GoWPad.

Next: finish actual engine startup, input/render/audio integration, then a signed app and physical iPhone gameplay verification.
