from pathlib import Path
import argparse
import hashlib
import json
import os
import platform
import re
import subprocess
import time

HERE = Path(__file__).resolve().parent
BASE = ["-std=gnu11", "-O0", "-g0", "-fno-inline", "-fno-lto", "-fno-optimize-sibling-calls",
        "-fstack-usage", "-Werror=implicit-function-declaration", "-Wno-unused-label",
        "-fvisibility=hidden", "-DRT_TEST", "-DRT_CALLTRACE"]
FILES = ["cache_registry.c", "wave_info_pcm.c", "ngp_wave_pcm_v1.c", "size_process_pcm.c", "process_pcm.c",
         "cache_registry.h", "wave_info_pcm.h", "ngp_wave_pcm_v1.h", "size_process_pcm.h", "process_pcm.h"]

def sha(raw):
    return hashlib.sha256(raw).hexdigest()

def identity(path):
    raw = path.read_bytes()
    return {"bytes": len(raw), "sha256": sha(raw)}

def run(label, argv, cwd, output, commands):
    start = time.monotonic()
    try:
        p = subprocess.run(list(map(str, argv)), cwd=cwd, capture_output=True, timeout=90, check=False)
        row = {"label": label, "argv": list(map(str, argv)), "exitCode": p.returncode,
               "stdout": p.stdout.decode(errors="replace")[:65536],
               "stderr": p.stderr.decode(errors="replace")[:65536],
               "elapsedMs": round((time.monotonic() - start) * 1000)}
    except subprocess.TimeoutExpired as exc:
        row = {"label": label, "argv": list(map(str, argv)), "exitCode": None, "timedOut": True,
               "stdout": (exc.stdout or b"").decode(errors="replace")[:65536],
               "stderr": (exc.stderr or b"").decode(errors="replace")[:65536],
               "elapsedMs": round((time.monotonic() - start) * 1000)}
    commands.append(row)
    (output / "commands.json").write_text(json.dumps(commands, indent=2) + "\n", encoding="utf-8")
    if row.get("exitCode") != 0:
        raise RuntimeError(f"bounded command failed: {label}")
    return row

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--output", required=True)
    args = ap.parse_args()
    out = Path(args.output).resolve()
    repo = Path(os.environ.get("GITHUB_WORKSPACE", "")).resolve()
    assert repo.is_dir() and HERE.is_relative_to(repo)
    assert not out.exists() and not out.is_relative_to(repo)
    out.mkdir(mode=0o700)
    objects = out / "objects"
    objects.mkdir(mode=0o700)
    source_manifest_path = HERE / "support-source-manifest.json"
    manifest_raw = source_manifest_path.read_bytes()
    manifest = json.loads(manifest_raw)
    assert manifest.get("schema") == "native-pcm-cache-support-source-manifest-v1"
    assert list(manifest["files"]) == sorted(FILES)
    for name in FILES:
        p = HERE / name
        assert p.is_file() and not p.is_symlink()
        assert identity(p) == manifest["files"][name], name
    source_manifest_sha = sha(manifest_raw)
    commands = []
    metadata = {"status": "failed", "actualAppleSdk": False, "linked": False,
                "signed": False, "nativeExecuted": False, "phone": False}
    result = {"status": "failed", "sourceManifestSha256": source_manifest_sha,
              "sourceFiles": manifest["files"], "compileObjects": {}, "commands": 0,
              "actualAppleSdk": False, "linked": False, "signed": False,
              "nativeExecuted": False, "phone": False}
    try:
        row = run("host", ["/usr/bin/uname", "-m"], repo, out, commands)
        assert row["stdout"].strip() == "arm64"
        row = run("developer", ["/usr/bin/xcode-select", "-p"], repo, out, commands)
        developer = Path(row["stdout"].strip()).resolve()
        assert developer == Path("/Applications/Xcode_27.1.app/Contents/Developer")
        row = run("sdk-path", ["/usr/bin/xcrun", "--sdk", "iphoneos", "--show-sdk-path"], repo, out, commands)
        sdk = Path(row["stdout"].strip()).resolve()
        assert sdk.is_dir() and sdk.is_relative_to(developer)
        row = run("sdk-version", ["/usr/bin/xcrun", "--sdk", "iphoneos", "--show-sdk-version"], repo, out, commands)
        assert row["stdout"].strip() == "27.1"
        row = run("compiler", ["/usr/bin/xcrun", "--sdk", "iphoneos", "--find", "clang"], repo, out, commands)
        clang = Path(row["stdout"].strip()).resolve()
        assert clang.is_relative_to(developer)
        row = run("compiler-version", [str(clang), "--version"], repo, out, commands)
        assert row["stdout"].startswith("Apple clang version ")
        result["toolchain"] = {"developer": str(developer), "sdk": str(sdk), "sdkVersion": "27.1",
                               "clang": str(clang), "clangVersion": row["stdout"].splitlines()[0],
                               "target": "arm64-apple-ios26.0"}

        for source in FILES[:5]:
            stem = Path(source).stem
            objects_for_unit = {}
            stack_files = {}
            depfiles = {}
            for kind in ("primary", "repeat"):
                suffix = "" if kind == "primary" else "-repeat"
                obj = objects / f"{stem}{suffix}.o"
                dep = objects / f"{stem}{suffix}.d"
                args = [clang, "-target", "arm64-apple-ios26.0", "-isysroot", sdk, *BASE,
                        "-I", HERE, "-MMD", "-MF", dep, "-c", HERE / source, "-o", obj]
                label = f"{stem}-{kind}"
                run(label, args, repo, out, commands)
                assert obj.is_file() and 0 < obj.stat().st_size <= 8 * 1024 * 1024
                stack = objects / f"{stem}{suffix}.su"
                assert stack.is_file() and stack.stat().st_size <= 262144
                assert dep.is_file() and dep.stat().st_size <= 262144
                objects_for_unit[kind] = obj.read_bytes()
                stack_files[kind] = stack.read_bytes()
                depfiles[kind] = identity(dep)
            assert objects_for_unit["primary"] == objects_for_unit["repeat"], source
            assert stack_files["primary"] == stack_files["repeat"], source
            if source == "cache_registry.c":
                assert any(line.endswith("\tstatic") for line in stack_files["primary"].decode().splitlines())
            result["compileObjects"][stem] = {
                "source": source,
                "sourceSha256": manifest["files"][source]["sha256"],
                "primary": identity(objects / f"{stem}.o"),
                "repeat": identity(objects / f"{stem}-repeat.o"),
                "byteIdentical": True,
                "stackUsageSha256": sha(stack_files["primary"]),
                "stackUsageRows": len(stack_files["primary"].decode().splitlines()),
                "depfiles": depfiles,
            }
        for name in FILES:
            assert identity(HERE / name) == manifest["files"][name], name
        result.update(status="collected-not-certified", commands=len(commands), actualAppleSdk=True,
                      sdkFilesRetained=False, sourceClosureStable=True,
                      note="Five self-authored support C units compiled twice for the iPhoneOS SDK. Objects and stack reports are byte-identical per unit. No link or engine execution.")
        metadata.update(status="passed", actualAppleSdk=True, sdkVersion="27.1", linked=False)
    except Exception as exc:
        result.update(failure={"type": type(exc).__name__, "message": str(exc)[:512]}, commands=len(commands))
    (out / "result.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    (out / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": result["status"], "commands": result["commands"],
                      "actualAppleSdk": result["actualAppleSdk"], "linked": result["linked"],
                      "sourceManifestSha256": source_manifest_sha}))
    if result["status"] != "collected-not-certified":
        raise SystemExit(1)

if __name__ == "__main__":
    main()
