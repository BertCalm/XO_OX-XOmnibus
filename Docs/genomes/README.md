# XOlokun Engine Genomes

**Status:** 5 of 70 genomes complete
**Schema:** v0.1 — see `Docs/specs/xogenome-schema-v0.1.md`
**Project:** VQ 009 — Engine Genome Project
**Updated:** 2026-03-22

---

## What Is a Genome?

A `.xogenome` file is a behavioral fingerprint for one XOlokun engine. It captures
how the engine *behaves* — oscillator architecture, filter topology, modulation depth,
velocity response, macro personality, sonic character, coupling affinities, and mythology.

It is not DSP code. It is the engine's identity as heard by a musician.

Genomes enable:
- **Discovery** — find engines matching a target sonic profile
- **Pairing** — predict productive coupling partners
- **Curriculum** — Guru Bin retreat writers use the genome as a starting point
- **Tooling** — future CLI/Aquarium interface for genome-based search

---

## Completed Genomes (5)

| File | Engine | Category | Seance | Presets |
|------|--------|----------|--------|---------|
| `oware.xogenome` | OWARE | tuned_percussion | 9.2 | 175 |
| `obrix.xogenome` | OBRIX | modular | 8.6 (est. post-Wave 4) | 466 |
| `opera.xogenome` | OPERA | vocal | 8.85 | 156 |
| `onset.xogenome` | ONSET | drums | unscored | 636 |
| `origami.xogenome` | ORIGAMI | spectral | 8.0 | 598 |

---

## Remaining Engines (65)

The following engines need genomes generated. Source headers are in
`Source/Engines/{Name}/{Name}Engine.h`. Read each header for real param counts,
macro names, voice counts, and architecture before writing the genome.

For each engine: count `ParameterID` declarations for `param_count`,
run `grep -rl '"<EngineId>"' Presets/ | wc -l` for `preset_count`,
and check `Docs/fleet-seance-scores-2026-03-20.md` for `seance_score`.

| Engine ID | Param Prefix | Category (est.) |
|-----------|-------------|-----------------|
| OddfeliX | `snap_` | lead / pluck |
| OddOscar | `morph_` | pad / morphing |
| Overdub | `dub_` | pad / tape |
| Odyssey | `drift_` | lead / wavetable |
| Oblong | `bob_` | bass / analog |
| Obese | `fat_` | lead / saturation |
| Overworld | `ow_` | lead / chip |
| Opal | `opal_` | pad / granular |
| Orbital | `orb_` | pad / spectral |
| Organon | `organon_` | generative / metabolism |
| Ouroboros | `ouro_` | lead / chaotic |
| Obsidian | `obsidian_` | pad / crystal |
| Overbite | `poss_` | bass / physical |
| Oracle | `oracle_` | lead / stochastic |
| Obscura | `obscura_` | pad / physical |
| Oceanic | `ocean_` | pad / chromatophore |
| Ocelot | `ocelot_` | lead / biome |
| Optic | `optic_` | fx / visual |
| Oblique | `oblq_` | lead / prismatic |
| Osprey | `osprey_` | pad / coastal |
| Osteria | `osteria_` | bass / wine |
| Owlfish | `owl_` | lead / trautonium |
| Ohm | `ohm_` | pad / analog |
| Orphica | `orph_` | pluck / siren |
| Obbligato | `obbl_` | pad / choir |
| Ottoni | `otto_` | brass / pipe |
| Ole | `ole_` | lead / flamenco |
| Overlap | `olap_` | pad / knot |
| Outwit | `owit_` | lead / chromatophore |
| Ombre | `ombre_` | pad / dual-narrative |
| Orca | `orca_` | lead / apex |
| Octopus | `octo_` | lead / decentralized |
| Ostinato | `osti_` | drum-machine / membrane |
| OpenSky | `sky_` | pad / shimmer |
| OceanDeep | `deep_` | bass / abyssal |
| Ouie | `ouie_` | lead / duophonic |
| Orbweave | `weave_` | pad / topological |
| Overtone | `over_` | spectral / continued-fraction |
| Organism | `org_` | generative / cellular-automata |
| Oxbow | `oxb_` | pad / entangled-reverb |
| Offering | `ofr_` | drum-machine / psychology |
| Osmosis | `osmo_` | fx / membrane |
| Kitchen Collection — XOto | (Kitchen) | organ |
| Kitchen Collection — XOctave | (Kitchen) | organ |
| Kitchen Collection — XOleg | (Kitchen) | organ |
| Kitchen Collection — XOtis | (Kitchen) | organ |
| Kitchen Collection — XOven | (Kitchen) | piano |
| Kitchen Collection — XOchre | (Kitchen) | piano |
| Kitchen Collection — XObelisk | (Kitchen) | piano |
| Kitchen Collection — XOpaline | (Kitchen) | piano |
| Kitchen Collection — XOgre | (Kitchen) | bass |
| Kitchen Collection — XOlate | (Kitchen) | bass |
| Kitchen Collection — XOaken | (Kitchen) | bass |
| Kitchen Collection — XOmega | (Kitchen) | bass |
| Kitchen Collection — XOrchard | (Kitchen) | strings |
| Kitchen Collection — XOvergrow | (Kitchen) | strings |
| Kitchen Collection — XOsier | (Kitchen) | strings |
| Kitchen Collection — XOxalis | (Kitchen) | strings |
| Kitchen Collection — XOverwash | (Kitchen) | pad |
| Kitchen Collection — XOverworn | (Kitchen) | pad |
| Kitchen Collection — XOverflow | (Kitchen) | pad |
| Kitchen Collection — XOvercast | (Kitchen) | pad |
| Kitchen Collection — XOasis | (Kitchen) | ep |
| Kitchen Collection — XOddfellow | (Kitchen) | ep |
| Kitchen Collection — XOnkolo | (Kitchen) | ep |
| Kitchen Collection — XOpcode | (Kitchen) | ep |

---

## Generating a New Genome

1. Read the engine header: `Source/Engines/{Name}/{Name}Engine.h`
2. Count params: `grep -c 'ParameterID' Source/Engines/{Name}/{Name}Engine.h`
3. Count presets: `grep -rl '"EngineName"' Presets/ | wc -l` (use preset-cased name, e.g., `"Oware"`)
4. Find seance score in `Docs/fleet-seance-scores-2026-03-20.md`
5. Check for Guru Bin retreat in `Docs/guru-bin-retreats/`
6. Write `Docs/genomes/{engine_lowercase}.xogenome` following the schema in `Docs/specs/xogenome-schema-v0.1.md`
7. Validate JSON: `python3 -m json.tool Docs/genomes/{name}.xogenome > /dev/null && echo VALID`
8. Add a row to the Completed Genomes table above

### Priority order for remaining engines

| Priority | Engine | Seance Score | Notes |
|----------|--------|-------------|-------|
| 1 | OVERBITE | 9.2 | Highest score in fleet, Five-Macro system |
| 2 | OBSCURA | 9.1 | Physical string model |
| 3 | OUROBOROS | 9.0 | Flagship chaos/strange attractor engine |
| 4 | OXBOW | 9.0 | Entangled reverb, Chiasmus FDN |
| 5 | OSTINATO | 8.7 | 96 world rhythm patterns, B017-B020 |
| 6 | OUIE | 8.5 | HAMMER/LOVE axis, duophonic |
| 7 | OPENSKY | 8.1 | Shepard shimmer, B023/B024 |
| 8 | OCEANDEEP | 7.8 | Darkness Filter ceiling, B029-B031 |
| 9–24 | Kitchen Collection (24 engines) | varies | V2 paid expansion engines |

---

## Quick jq Queries

```bash
# All genomes sorted by seance score
jq -rs 'sort_by(-.metadata.seance_score) | .[] | "\(.engine_id): \(.metadata.seance_score)"' Docs/genomes/*.xogenome

# All engines with movement > 0.7 (good for evolving pads)
jq -rs 'map(select(.sonic_character.movement > 0.7)) | .[] | .engine_id' Docs/genomes/*.xogenome

# Engines that accept EnvToMorph coupling
jq -rs 'map(select(.coupling_affinity.best_as_target | contains(["EnvToMorph"]))) | .[] | .engine_id' Docs/genomes/*.xogenome

# All feliX-leaning engines (polarity < 0.3)
jq -rs 'map(select(.mythology.felix_oscar_polarity < 0.3)) | .[] | "\(.engine_id): \(.mythology.felix_oscar_polarity)"' Docs/genomes/*.xogenome

# Engines by primary category
jq -rs 'group_by(.sonic_character.primary_category) | .[] | {category: .[0].sonic_character.primary_category, engines: [.[].engine_id]}' Docs/genomes/*.xogenome
```
