"""XPN Provenance — cryptographic hash chain for build integrity.

Tracks the transformation pipeline:
  source .xometa → rendered WAV → processed WAV → XPM program → .xpn archive

Each stage records a sha256 hash. The chain is embedded in the .xpn archive
as `provenance.json`. The `oxport.py verify` command validates the chain.
"""

import hashlib
import json
from pathlib import Path


def hash_file(path: Path) -> str:
    """Compute sha256 hex digest of a file."""
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            h.update(chunk)
    return h.hexdigest()


def hash_directory(dir_path: Path, glob_pattern: str = "*") -> str:
    """Compute a deterministic sha256 over all files matching glob in a directory.
    Files are sorted by name for determinism."""
    h = hashlib.sha256()
    for f in sorted(dir_path.rglob(glob_pattern)):
        if f.is_file():
            h.update(f.name.encode())
            h.update(hash_file(f).encode())
    return h.hexdigest()


class ProvenanceChain:
    """Accumulates hashes at each pipeline stage."""

    def __init__(self, engine: str, pack_name: str, version: str):
        self.engine = engine
        self.pack_name = pack_name
        self.version = version
        self.chain: list[dict] = []

    def record(self, stage: str, artifact_path: Path, description: str = ""):
        """Record a hash for a pipeline artifact."""
        if artifact_path.is_dir():
            digest = hash_directory(artifact_path)
        else:
            digest = hash_file(artifact_path)
        self.chain.append({
            "stage": stage,
            "artifact": str(artifact_path.name),
            "sha256": digest,
            "description": description,
        })

    def to_json(self) -> str:
        """Serialize the chain to JSON."""
        return json.dumps({
            "provenance_version": "1.0",
            "engine": self.engine,
            "pack_name": self.pack_name,
            "version": self.version,
            "chain": self.chain,
            "chain_hash": self._compute_chain_hash(),
        }, indent=2)

    def _compute_chain_hash(self) -> str:
        """Compute a hash over the entire chain for tamper detection."""
        h = hashlib.sha256()
        for entry in self.chain:
            h.update(entry["sha256"].encode())
        return h.hexdigest()


def verify_provenance(provenance_json: str, base_dir: Path) -> dict:
    """Verify a provenance chain against actual files.

    Returns {"valid": bool, "errors": list[str], "verified": int, "total": int}
    """
    data = json.loads(provenance_json)
    chain = data.get("chain", [])
    errors = []
    verified = 0

    # Verify chain_hash
    h = hashlib.sha256()
    for entry in chain:
        h.update(entry["sha256"].encode())
    expected_chain_hash = h.hexdigest()
    if data.get("chain_hash") != expected_chain_hash:
        errors.append(
            f"Chain hash mismatch: expected {expected_chain_hash}, "
            f"got {data.get('chain_hash')}"
        )

    # Verify individual artifacts (if they exist in base_dir)
    for entry in chain:
        artifact = base_dir / entry["artifact"]
        if not artifact.exists():
            # Artifact may be inside the archive — skip file-level check
            continue
        if artifact.is_dir():
            actual = hash_directory(artifact)
        else:
            actual = hash_file(artifact)
        if actual == entry["sha256"]:
            verified += 1
        else:
            errors.append(
                f"{entry['artifact']}: hash mismatch (stage={entry['stage']})"
            )

    return {
        "valid": len(errors) == 0,
        "errors": errors,
        "verified": verified,
        "total": len(chain),
    }
