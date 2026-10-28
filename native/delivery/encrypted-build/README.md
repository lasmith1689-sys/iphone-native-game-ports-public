This tooling transfers explicitly selected private build sources through a public
repository as authenticated ciphertext. It uses Node24 built-in `node:crypto`
AES-256-GCM, not a custom cipher or third-party cryptography package. No provider,
signer, account, provisioning or target execution action is performed locally.

`transport.mjs` implements the bounded envelope and deterministic bundle.
`bundle-builder.mjs` verifies an explicit source closure and creates `input.ngp`
plus a generic `public-manifest.json`. `run-build.mjs` authenticates the input,
runs the exact private Python driver, captures its output privately, and seals
selected build files and diagnostics. `decrypt.mjs` requires an independently
recorded ciphertext SHA256/size and exact expected context before extraction.

The public workflow comes from `prepare_workflow.py`; it requires the exact
public repository, branch and either an exact workflow-path push or explicit
manual bundle request, checks all
public tool digests and the checkout commit, and uploads **only** `result.ngp`
for one day. No cache or plaintext artifact is configured. The standard `xcode-27`
runner's installed Node must actually be version24. The child receives a small
environment whitelist without the transport secret or Actions credentials.

Each child stream is capped at32MiB. The child process group is killed on timeout
or excess logs, and a failed result with bounded diagnostics is sealed. Selected
output paths are explicit; synthetic fixtures and sparse files are excluded.
No output archive is created by recursively copying an entire working directory.

Format and boundaries:

- Envelope:8-byte `NGPENC01`,4-byte big-endian canonical JSON header length,
  canonical header, ciphertext,16-byte GCM tag. The full prefix/header is AAD.
  A fresh random12-byte nonce is generated per envelope. HKDF-SHA256 derives
  separate input/output keys from the caller's random32-byte root key.
- Input context: exact repository/ref/purpose/fresh UUIDv4 bundle ID/source-closure
  SHA256. Output additionally binds actual run ID, attempt, head commit and input
  ciphertext SHA256. No algorithm, role, context or tag-length downgrade exists.
- Authentication finishes **in RAM before any plaintext filesystem write**.
  Encrypted payload and expanded bundle are each bounded to512MiB. This is
  deliberately an in-memory source transport, not unbounded streaming encryption.
- Bundle: deterministic gzip of `NGPBDL01`,4-byte manifest length, canonical JSON
  file table, concatenated bytes. Each file has an exact size/SHA256. All names,
  collisions and content hashes are checked before extraction. Only regular
  files are produced; absolute/traversal/drive/ADS/device/backslash/ambiguous
  names, casefold duplicates, file/parent collisions and symlink ancestors fail.
- Destination must be fresh. A caller-owned persistent `wx` ledger prevents
  repeated consumption under that ledger. Changed context/run replay fails
  cryptographically. There is no claim of a global ledger shared across fresh
hosted runners: each authorized push/manual request identifies the reviewed
  bundle, and unchanged authorized input can be rebuilt in another run. A fresh
  bundle ID is required when preparing a new source version/authorization.
- Caller owns valid immutable source inputs and serialized destination/ledger
  lifecycle. Hostile same-user/privileged processes and a compromised runner are
  outside this interface. Compiled source is trusted reviewed input; encryption
  is not a sandbox. Source reconstruction and whole-game semantics remain
  independent of successful transport or SDK compilation.

Keys are canonical base64 supplied through stdin, or the named secret environment
variable. CLI key arguments/files are not accepted. No key is generated, printed,
installed or uploaded by these tools. CLI errors are generic; private child
tracebacks and source snippets are retained only inside the output envelope.

Run local synthetic tests with the verified Node24 executable:

```text
node --test test_transport.mjs test_builder.mjs test_runner.mjs test_decrypt.mjs
```

Prepare with explicit `--root`, `--closure`, `--recipe`, `--context`, `--output`
arguments to `bundle-builder.mjs`. Keep the private closure/recipe/context files
outside public staging. Only `input.ngp`, `public-manifest.json`, the generated
workflow and the public source whitelist are publishable. Validate a prepared
workflow with `validate_workflow.py --bundle BUNDLE_DIR --workflow WORKFLOW.yml`.
After obtaining the sealed result, derive its expected context from the reviewed
input context and independently verified actual run/head/ciphertext evidence;
pass that file and the independent size/hash to `decrypt.mjs`. Inspect/scan any
resulting executable separately before loading it.

Official action pins were inspected in their primary repositories:
[checkout](https://github.com/actions/checkout/blob/d23441a48e516b6c34aea4fa41551a30e30af803/action.yml),
[upload-artifact](https://github.com/actions/upload-artifact/blob/cf430e030ddbb5b0abf93d22962f4752f3646cd9/action.yml).
Both exact action manifests declare Node24. Push triggers are limited to the
isolated branch and the exact workflow path; staging with `[skip ci]` does not
build. After owner secret installation, a normal exact workflow-file commit can
trigger the single reviewed build without modifying the default branch.
The runner's installed Node24 listing is documented in
[runner-images](https://github.com/actions/runner-images/blob/main/images/macos/xcode-27-arm64-Readme.md).

Next: review the exact source/ciphertext/workflow projection, stage only that
whitelist, then have the authorized owner handle secret installation and dispatch.
Continue in the current thread; no new thread is required.
