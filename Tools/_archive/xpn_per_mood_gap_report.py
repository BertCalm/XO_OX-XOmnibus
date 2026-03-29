"""
xpn_per_mood_gap_report.py
Per-mood XLOW (<=0.15) / XHIGH (>=0.85) percentage breakdown for all 6 DNA dimensions.
Flags any dimension/endpoint under 5% with a warning marker.
"""
import json
import os
import glob

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PRESETS_DIR = os.path.join(REPO_ROOT, "Presets", "XOlokun")

moods = ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"]
dims = ["brightness", "warmth", "movement", "density", "space", "aggression"]

results = {}

for mood in moods:
    files = glob.glob(os.path.join(PRESETS_DIR, mood, "*.xometa"))
    total = len(files)
    if total == 0:
        continue
    counts = {d: {"xlow": 0, "xhigh": 0} for d in dims}
    for f in files:
        try:
            with open(f) as fh:
                data = json.load(fh)
            dna = data.get("dna", {})
            for d in dims:
                v = dna.get(d, 0.5)
                if v <= 0.15:
                    counts[d]["xlow"] += 1
                elif v >= 0.85:
                    counts[d]["xhigh"] += 1
        except Exception:
            pass
    results[mood] = {"total": total, "counts": counts}
    print(f"\n{mood} ({total} presets):")
    for d in dims:
        xl = counts[d]["xlow"] / total * 100
        xh = counts[d]["xhigh"] / total * 100
        flag_l = " ⚠️" if xl < 5 else ""
        flag_h = " ⚠️" if xh < 5 else ""
        print(f"  {d:12s}: XLOW={xl:5.1f}%{flag_l}  XHIGH={xh:5.1f}%{flag_h}")

print("\n")

# Persist results as JSON for downstream use
out_path = os.path.join(REPO_ROOT, "Docs", "snapshots", "per_mood_gap_report_wave92.json")
os.makedirs(os.path.dirname(out_path), exist_ok=True)
payload = {}
for mood, data in results.items():
    total = data["total"]
    payload[mood] = {"total": total, "dims": {}}
    for d in dims:
        xl = data["counts"][d]["xlow"] / total * 100
        xh = data["counts"][d]["xhigh"] / total * 100
        payload[mood]["dims"][d] = {
            "xlow_pct": round(xl, 2),
            "xhigh_pct": round(xh, 2),
            "xlow_warn": xl < 5,
            "xhigh_warn": xh < 5,
        }
with open(out_path, "w") as fh:
    json.dump(payload, fh, indent=2)
print(f"JSON saved → {out_path}")
