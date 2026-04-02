# ONSET Drum Essentials — Pack Specification

> Status: READY TO RENDER | Price: $9.99 | Engine: ONSET
> 20 kits, 640 WAVs (8 voices x 4 velocity layers x 20 presets)

## Pack Contents

20 drum programs spanning the full aggression spectrum:

| Kit | Aggression | Character |
|-----|-----------|-----------|
| Crystal Gentle | 0.02 | Delicate, ambient percussion |
| Glacial Machine | 0.08 | Slow, cold, mechanical |
| Silt Floor | 0.15 | Earthy, organic, muted |
| Machine Ghost | 0.20 | Haunted electronics |
| Deep House | 0.25 | Classic deep house kit |
| Dub Pressure | 0.30 | Reggae/dub sub-heavy |
| Distant Impacts | 0.35 | Cinematic, far-field |
| 606 Toy Box | 0.35 | Playful, retro drum machine |
| Bitcrushed Fur | 0.40 | Lo-fi, textured |
| Neural Beats | 0.45 | AI-adjacent, algorithmic |
| CZ Crystal | 0.45 | Phase distortion character |
| Velocity Feedback | 0.48 | Dynamic, responsive |
| Boom Bap OG | 0.50 | Classic hip-hop |
| Drum Kit Core | 0.55 | All-purpose workhorse |
| Punch Mode | 0.65 | Aggressive, forward |
| Concrete Kick | 0.70 | Industrial, hard |
| Techno Industrial | 0.80 | Dark, relentless |
| Onset Typhoon | 0.92 | Maximum aggression |

## Q-Link Assignments (all programs)

- Q1: CHARACTER (FilterCutoff) — timbral sweep
- Q2: MOVEMENT (LFO Rate) — motion/modulation
- Q3: COUPLING (Send2) — cross-feed/parallel
- Q4: SPACE (Send1) — reverb/spatial depth

## Render Instructions

```bash
# 1. Install dependencies
pip install mido python-rtmidi sounddevice

# 2. Start XOceanus (standalone or in DAW)
# 3. Install BlackHole (macOS loopback): brew install blackhole-2ch
# 4. Route XOceanus audio output to BlackHole

# 5. List available ports
python3 Tools/oxport_render.py --list-ports

# 6. Render all 20 kits
python3 Tools/oxport_render.py \
  --spec Tools/onset_render_spec.json \
  --output-dir ./wavs/onset/ \
  --midi-port "XOceanus" \
  --audio-device "BlackHole"

# 7. Run the full Oxport pipeline
python3 Tools/oxport.py run \
  --engine Onset \
  --wavs-dir ./wavs/onset/ \
  --output-dir ./dist/onset_drum_essentials/ \
  --kit-mode smart

# 8. Validate
python3 Tools/oxport.py validate \
  --output-dir ./dist/onset_drum_essentials/ \
  --presets
```

## Estimated Render Time

- 640 WAVs x (2s note + 0.5s release + 0.2s gap) = ~29 minutes
- Plus preset loading: 20 presets x 0.2s = 4 seconds
- Total: ~30 minutes unattended

## Competitive Positioning

"Every other drum pack sells you recordings of someone else's drum machine.
ONSET synthesizes every hit from scratch. 20 kits spanning ambient whispers
to industrial mayhem — each with DNA-adaptive velocity response that makes
your pads feel different depending on the kit's character."
