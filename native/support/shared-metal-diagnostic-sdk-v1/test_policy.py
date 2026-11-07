"""Compile/run only the owned portable policy; retain exact local proof."""
import argparse
import hashlib
import json
import pathlib
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cc", default=shutil.which("clang") or shutil.which("cc"))
    parser.add_argument("--out", type=pathlib.Path, required=True)
    args = parser.parse_args()
    if not args.cc:
        parser.error("supply --cc with a native C compiler")
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    exe = out / ("test_shell_policy.exe" if sys.platform == "win32" else "test_shell_policy")
    command = [str(args.cc), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-pedantic",
               str(ROOT / "shell_policy.c"), str(ROOT / "test_shell_policy.c"), "-o", str(exe)]
    if sys.platform != "win32":
        command.append("-lm")
    version = subprocess.run([str(args.cc), "--version"], check=True, text=True, capture_output=True)
    compile_result = subprocess.run(command, check=True, text=True, capture_output=True)
    run = subprocess.run([str(exe)], check=True, text=True, capture_output=True)
    receipt = {"schema": "owned-metal-policy-proof-v1", "scope": "native portable policy only",
               "uikit_compiled": False, "metal_compiled": False, "gpu_executed": False,
               "compiler_version": version.stdout, "compile_command": command,
               "compile_stdout": compile_result.stdout, "compile_stderr": compile_result.stderr,
               "run_stdout": run.stdout, "run_stderr": run.stderr, "run_exit": run.returncode,
               "sources": {name: sha(ROOT / name) for name in ("shell_policy.h", "shell_policy.c", "test_shell_policy.c", "test_policy.py")},
               "executable_sha256": sha(exe)}
    (out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(run.stdout, end="")
    print(out / "receipt.json")

if __name__ == "__main__":
    main()
