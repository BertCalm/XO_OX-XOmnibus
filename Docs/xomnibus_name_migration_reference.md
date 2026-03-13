# XOmnibus — Engine Name Migration Reference
*Quick-reference for the legacy → canonical name transition.*
*Last updated: March 2026*

---

## Why the Rename

All XO_OX instruments follow the **XO + O-word** naming convention. The original gallery engines
were developed under working names (Snap, Morph, Dub, etc.) that predated this brand rule.
The rename aligns every engine with the convention while keeping parameter prefixes frozen
for preset compatibility.

---

## Engine Name Mapping

| Legacy Gallery Code | Canonical Gallery Code | Legacy Engine ID | Canonical Engine ID | Source Instrument | Parameter Prefix (FROZEN) |
|--------------------|-----------------------|-----------------|--------------------|--------------------|--------------------------|
| SNAP | ODDFELIX | Snap | OddfeliX | OddfeliX (feliX the neon tetra) | `snap_` |
| MORPH | ODDOSCAR | Morph | OddOscar | OddOscar (Oscar the axolotl) | `morph_` |
| DUB | OVERDUB | Dub | Overdub | XOverdub | `dub_` |
| DRIFT | ODYSSEY | Drift | Odyssey | XOdyssey | `drift_` |
| BOB | OBLONG | Bob | Oblong | XOblong | `bob_` |
| FAT | OBESE | Fat | Obese | XObese | `fat_` |
| BITE | OVERBITE | Bite | Overbite | XOverbite (was XOpossum) | `poss_` |
| — | ONSET | Onset | Onset | XOnset | `perc_` |
| — | OVERWORLD | Overworld | Overworld | XOverworld | `ow_` |

### Engines that did NOT change
These were already O-prefix from birth:
- ONSET, OVERWORLD, OPAL, ORGANON, OUROBOROS, ORBITAL
- OBSIDIAN, ORIGAMI, ORACLE, OBSCURA, OCEANIC
- OPTIC, OBLIQUE, ORCA, OCTOPUS

---

## XOpossum → XOverbite

The standalone bass synth originally named **XOpossum** has been renamed to **XOverbite**
to follow the XO + O-word convention. All references (docs, source comments, roadmaps)
have been updated. The parameter prefix `poss_` remains frozen.

| Old | New |
|-----|-----|
| XOpossum | XOverbite |
| XOppossum (typo variant) | XOverbite |
| `opossum_` (prefix in some early docs) | `poss_` (actual frozen prefix) |

---

## Legacy Alias Resolution

`resolveEngineAlias()` in `Source/Core/PresetManager.h` maps old names to canonical IDs
at preset load time. This ensures backward compatibility with any .xometa files or
saved states that reference old engine names.

```cpp
{ "Snap",        "OddfeliX"  }
{ "Morph",       "OddOscar"  }
{ "Dub",         "Overdub"   }
{ "Drift",       "Odyssey"   }
{ "Bob",         "Oblong"    }
{ "Fat",         "Obese"     }
{ "Bite",        "Overbite"  }
{ "XOddCouple",  "OddfeliX" }
{ "XOverdub",    "Overdub"   }
{ "XOdyssey",    "Odyssey"   }
{ "XOblong",     "Oblong"    }
{ "XOblongBob",  "Oblong"    }
{ "XObese",      "Obese"     }
{ "XOnset",      "Onset"     }
{ "XOrbital",    "Orbital"   }
{ "XOrganon",    "Organon"   }
{ "XOuroboros",  "Ouroboros" }
{ "XOpal",       "Opal"      }
{ "XOpossum",    "Overbite"  }
{ "XOverbite",   "Overbite"  }
{ "XObsidian",   "Obsidian"  }
{ "XOrigami",    "Origami"   }
{ "XOracle",     "Oracle"    }
{ "XObscura",    "Obscura"   }
{ "XOceanic",    "Oceanic"   }
{ "XOptic",      "Optic"     }
{ "XOblique",    "Oblique"   }
{ "XOverworld",  "Overworld" }
{ "XOrca",       "Orca"      }
{ "XOctopus",    "Octopus"   }
```

---

## What to Do When You Find an Old Name

1. **In documentation:** Replace with the canonical name from the table above.
2. **In source code comments:** Replace with canonical name.
3. **In engine registration/factory code:** Use canonical engine ID.
4. **In parameter IDs:** **DO NOT CHANGE.** Prefixes are frozen forever.
5. **In preset .xometa files:** Use canonical engine IDs. Old names load via alias resolution.
6. **In Python tools:** Update engine name strings but keep parameter prefix strings.
7. **In test data:** Legacy names in test JSON are intentional (testing backward compat). Update test labels/assertions to use canonical names.

---

## Common Gotchas

- `dub_` parameters belong to **Overdub** (not "Dub")
- `bob_` parameters belong to **Oblong** (not "Bob")
- `snap_` parameters belong to **OddfeliX** (not "Snap")
- `fat_` parameters belong to **Obese** (not "Fat")
- `poss_` parameters belong to **Overbite** (not "Bite" or "XOpossum")
- `drift_` parameters belong to **Odyssey** (not `odyssey_` — the prefix predates the rename)
- `perc_` parameters belong to **Onset** (not `onset_`)
- `ow_` parameters belong to **Overworld** (not `overworld_`)
- The word "dub" as a music genre (dub echo, dub space) is NOT an engine reference — don't change it
- `bite_` as a parameter prefix in some early docs was a mistake — the actual frozen prefix is `poss_`
