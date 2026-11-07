"""Verify the explicit, owned source closure; do not discover or upload anything."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
manifest = json.loads((ROOT / "source-manifest.json").read_text(encoding="utf-8"))
for name, expected in manifest["files"].items():
    path = ROOT / name
    if path.parent != ROOT or path.is_symlink() or not path.is_file():
        raise SystemExit(f"invalid closure path: {name}")
    actual = hashlib.sha256(path.read_bytes()).hexdigest()
    if actual != expected:
        raise SystemExit(f"source closure mismatch: {name}")
print(f"PASS exact {len(manifest['files'])}-file owned source closure")
