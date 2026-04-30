// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// EngineRoster.h — Single source of truth for the XOceanus engine metadata table.
//
// Both EnginePickerDrawer (Ocean view) and EnginePickerPopup (Gallery view) source
// their engine lists from this header.  Previously each maintained an independent
// copy of the metadata table — a parallel-list smell surfaced by the fab-five F2
// audit (#1354) which also revealed 17 implemented engines were invisible in both
// pickers.
//
// ADDING A NEW ENGINE
//   1. Add a row to kEngineRoster[] below (keep sections and alphabetical order
//      within each section).
//   2. Compile.  Both pickers pick it up automatically.
//   3. Update Docs/engines.json (canonical fleet record) and CLAUDE.md tables.
//
// DATA SHAPE
//   id         — canonical engine ID string (matches EngineRegistry + preset "engines" keys)
//   category   — "Synth" | "Percussion" | "Bass" | "Pad" | "String" | "Organ" | "Vocal" | "FX" | "Utility"
//   archetype  — one-line description shown in both pickers (search-indexed)
//   accentARGB — 0xAARRGGBB color from engine-color-table.md
//   depthZone  — 0=Sunlit 1=Twilight 2=Midnight  (used by EnginePickerPopup zone grouping)
//
// THREAD SAFETY
//   engineRosterTable() returns a pointer to a function-local static array.
//   Safe to call from any thread after first call; never modified after init.

#include <cstdint>
#include <cstddef>

namespace xoceanus
{

//==============================================================================
struct EngineRosterEntry
{
    const char* id;
    const char* category;   // "Synth"|"Percussion"|"Bass"|"Pad"|"String"|"Organ"|"Vocal"|"FX"|"Utility"
    const char* archetype;  // short description displayed under the engine name
    uint32_t    accentARGB; // 0xAARRGGBB
    int         depthZone;  // 0=Sunlit 1=Twilight 2=Midnight
};

/// Returns a pointer to the null-terminated engine roster table.
/// The sentinel entry has id == nullptr — use that to detect end-of-table.
inline const EngineRosterEntry* engineRosterTable() noexcept
{
    // clang-format off
    static const EngineRosterEntry kTable[] =
    {
        // ── Kitchen Collection — Organs (Chef Quad) ──────────────────────────
        { "Oto",          "Organ",      "tonewheel drawbar organ",                          0xFFF5F0E8, 0 },
        { "Octave",       "Organ",      "Hammond tonewheel simulation",                     0xFF8B6914, 0 },
        { "Oleg",         "Organ",      "theatre pipe organ",                               0xFFC0392B, 0 },
        { "Otis",         "Organ",      "gospel soul organ drive",                          0xFFD4A017, 0 },
        // ── Kitchen Collection — Pianos (Kitchen Quad) ───────────────────────
        { "Oven",         "String",     "Steinway concert grand piano",                     0xFF1C1C1C, 1 },
        { "Ochre",        "String",     "wooden resonator piano",                           0xFFCC7722, 1 },
        { "Obelisk",      "String",     "grand piano sympathetic resonance",                0xFFFFFFE0, 0 },
        { "Opaline",      "String",     "prepared piano rust and objects",                  0xFFB7410E, 1 },
        // ── Kitchen Collection — Bass (Cellar Quad) ──────────────────────────
        { "Ogre",         "Bass",       "sub bass synthesizer",                             0xFF0D0D0D, 2 },
        { "Olate",        "Bass",       "fretless bass guitar",                             0xFF5C3317, 1 },
        { "Oaken",        "Bass",       "upright double bass",                              0xFF9C6B30, 1 },
        { "Omega",        "Bass",       "analog synth bass",                                0xFF003366, 2 },
        // ── Kitchen Collection — Strings (Garden Quad) ───────────────────────
        { "Orchard",      "String",     "orchestral strings bow pressure",                  0xFFFFB7C5, 0 },
        { "Overgrow",     "String",     "overgrown string textures",                        0xFF228B22, 1 },
        { "Osier",        "String",     "willow wind strings",                              0xFFC0C8C8, 0 },
        { "Oxalis",       "String",     "wood sorrel lilac strings",                        0xFF9B59B6, 1 },
        // ── Kitchen Collection — Pads (Broth Quad) ───────────────────────────
        { "Overwash",     "Pad",        "tide foam diffusion pad",                          0xFFF0F8FF, 0 },
        { "Overworn",     "Pad",        "worn felt texture pad",                            0xFF808080, 1 },
        { "Overflow",     "Pad",        "deep current flowing pad",                         0xFF1A3A5C, 2 },
        { "Overcast",     "Pad",        "cloud diffusion pad",                              0xFF778899, 1 },
        // ── Kitchen Collection — EPs (Fusion Quad) ───────────────────────────
        { "Oasis",        "Synth",      "desert spring electric piano",                     0xFF00827F, 0 },
        { "Oddfellow",    "Synth",      "spectral fingerprint cache EP",                    0xFFB87333, 1 },
        { "Onkolo",       "Synth",      "spectral amber resonant EP",                       0xFFFFBF00, 1 },
        { "Opcode",       "Synth",      "dark turquoise code-driven EP",                    0xFF5F9EA0, 1 },
        // ── Flagship + core synths ────────────────────────────────────────────
        { "Obrix",        "Synth",      "modular brick reef synthesizer",                   0xFF1E8B7E, 2 },
        { "Oxytocin",     "Synth",      "circuit love triangle synthesizer",                0xFF9B5DE5, 2 },
        { "Overbite",     "Synth",      "apex predator modal synthesizer",                  0xFFF0EDE8, 2 },
        { "Overworld",    "Synth",      "ERA triangle timbral crossfade",                   0xFF39FF14, 0 },
        { "Ouroboros",    "Synth",      "strange attractor chaotic synthesizer",            0xFFFF2D2D, 2 },
        { "Oracle",       "Synth",      "GENDY stochastic maqam synthesis",                 0xFF4B0082, 2 },
        { "Orbital",      "Synth",      "group envelope synthesizer",                       0xFFFF6B6B, 1 },
        { "Opal",         "Synth",      "granular cloud synthesizer",                       0xFFA78BFA, 1 },
        { "Obsidian",     "Synth",      "crystal resonant synthesizer",                     0xFFE8E0D8, 2 },
        { "Origami",      "Synth",      "fold-point waveshaping synthesizer",               0xFFE63946, 1 },
        { "Obscura",      "Synth",      "daguerreotype physical modeling",                  0xFF8A9BA8, 1 },
        { "Oblique",      "Synth",      "prismatic bounce synth",                           0xFFBF40FF, 1 },
        { "Organism",     "Synth",      "cellular automata generative synth",               0xFFC6E377, 1 },
        { "Orbweave",     "Synth",      "topological knot coupling engine",                 0xFF8E4585, 2 },
        { "Overtone",     "Synth",      "continued fraction spectral synth",                0xFFA8D8EA, 1 },
        { "Oxbow",        "Synth",      "entangled reverb synthesizer",                     0xFF1A6B5A, 2 },
        { "Outlook",      "Synth",      "panoramic dual wavetable synth",                   0xFF4169E1, 1 },
        { "Overlap",      "Synth",      "knot matrix FDN synthesizer",                      0xFF00FFB4, 2 },
        { "Orca",         "Synth",      "apex predator wavetable echolocation",             0xFF1B2838, 2 },
        { "Octopus",      "Synth",      "decentralized alien intelligence synth",           0xFFE040FB, 2 },
        { "Ombre",        "Synth",      "dual narrative memory synthesizer",                0xFF7B6B8A, 1 },
        { "OpenSky",      "Synth",      "euphoric shimmer supersaw synth",                  0xFFFF8C00, 0 },
        // ── Percussion ────────────────────────────────────────────────────────
        { "Onset",        "Percussion", "cross-voice coupling percussion",                  0xFF0066FF, 1 },
        { "Offering",     "Percussion", "psychology-driven boom bap drums",                 0xFFE5B80B, 1 },
        { "Oware",        "Percussion", "Akan tuned mallet percussion",                     0xFFB5883E, 1 },
        { "Ostinato",     "Percussion", "modal membrane world rhythm engine",               0xFFE8701A, 1 },
        // ── Vocal ─────────────────────────────────────────────────────────────
        { "Opera",        "Vocal",      "additive-vocal Kuramoto synchrony",                0xFFD4AF37, 1 },
        { "Obbligato",    "Vocal",      "breath articulation vocal synth",                  0xFFFF8A7A, 1 },
        // ── Bass synths ───────────────────────────────────────────────────────
        { "Oblong",       "Bass",       "resonant bass synthesizer",                        0xFFE9A84A, 1 },
        { "Obese",        "Bass",       "fat saturation bass synth",                        0xFFFF1493, 1 },
        // ── Organ & wind ──────────────────────────────────────────────────────
        { "Organon",      "Organ",      "variational metabolism organ synth",               0xFF00CED1, 1 },
        { "Ohm",          "Organ",      "sage analog organ synthesizer",                    0xFF87AE73, 0 },
        { "Ottoni",       "Organ",      "patina brass organ synthesizer",                   0xFF5B8A72, 1 },
        { "Ole",          "Organ",      "hibiscus flamenco organ synth",                    0xFFC9377A, 1 },
        // ── String / physical modeling ────────────────────────────────────────
        { "Orphica",      "String",     "siren seafoam plucked string",                     0xFF7FDBCA, 1 },
        { "Osprey",       "String",     "shore coastline cultural synthesis",               0xFF1B4F8A, 1 },
        { "Osteria",      "String",     "porto wine shore string synth",                    0xFF722F37, 1 },
        { "Owlfish",      "String",     "Mixtur-Trautonium string modeling",                0xFFB8860B, 2 },
        // ── Character synths ──────────────────────────────────────────────────
        { "OddfeliX",     "Synth",      "neon tetra character synth",                       0xFF00A6D6, 0 },
        { "OddOscar",     "Synth",      "axolotl character synth",                          0xFFE8839B, 0 },
        { "Odyssey",      "Synth",      "drift analog poly synthesizer",                    0xFF7B2D8B, 1 },
        { "Overdub",      "Synth",      "spring reverb dub synthesizer",                    0xFF6B7B3A, 1 },
        { "Oceanic",      "Synth",      "chromatophore phosphorescent synth",               0xFF00B4A0, 1 },
        { "Ocelot",       "Synth",      "biome crossfade ocelot synth",                     0xFFC5832B, 1 },
        { "Osmosis",      "Synth",      "external audio membrane synth",                    0xFFC0C0C0, 1 },
        // ── Utility / FX ──────────────────────────────────────────────────────
        { "Optic",        "Utility",    "visual modulation zero-audio engine",              0xFF00FF41, 0 },
        { "Outwit",       "FX",         "chromatophore amber effect engine",                0xFFCC6600, 1 },
        // ── Additional engines ────────────────────────────────────────────────
        { "OceanDeep",    "Synth",      "hydrostatic deep ocean synthesizer",               0xFF2D0A4E, 2 },
        { "Ouie",         "Synth",      "duophonic hammerhead synthesizer",                 0xFF708090, 2 },
        { "Obiont",       "Synth",      "cellular automata oscillator",                     0xFFE8A030, 2 },
        { "Okeanos",      "String",     "Spice Route Rhodes electric piano",                0xFFC49B3F, 1 },
        { "Outflow",      "Synth",      "predictive spatial fluid-dynamics engine",         0xFF1A1A40, 2 },
        // ── Previously missing — 17 engines added in #1354 ───────────────────
        { "Observandum",  "Synth",      "polychromatic phase synthesizer",                  0xFFB0C4DE, 2 },
        { "Octant",       "Synth",      "tensor-organized additive synthesis",              0xFF8B6F47, 2 },
        { "Ogive",        "Synth",      "scanned glass synthesis",                          0xFF9B1B30, 1 },
        { "Ollotron",     "String",     "tape-chamber Mellotron-spirit keyboard",           0xFFB07050, 1 },
        { "Olvido",       "Synth",      "spectral erosion synthesis",                       0xFF3B6E8F, 2 },
        { "Onda",         "Synth",      "soliton wave propagation synthesizer",             0xFFB8A0FF, 2 },
        { "Ondine",       "Vocal",      "Klatt formant vocal synthesizer",                  0xFF2E8B8B, 1 },
        { "Oobleck",      "Synth",      "reaction-diffusion wavetable synthesis",           0xFFB4FF39, 2 },
        { "Oort",         "Synth",      "boids polyphonic waveform synthesis",              0xFFA9A9A9, 2 },
        { "Ooze",         "Synth",      "fluid dynamics synthesis",                         0xFF2D5F5D, 2 },
        { "Opsin",        "Synth",      "neural feedback network synthesis",                0xFF00FFFF, 2 },
        { "Orrery",       "Synth",      "4-source vector synthesis",                        0xFF4682B4, 1 },
        { "Ortolan",      "Vocal",      "VOSIM formant song synthesizer",                   0xFFD4A574, 1 },
        { "Ostracon",     "Synth",      "corpus-buffer shared tape synthesis",              0xFFC0785A, 2 },
        { "Outcrop",      "Synth",      "geometric terrain synthesis",                      0xFF5B6F57, 1 },
        { "Overtide",     "Synth",      "wavelet multi-scale synthesis",                    0xFF1E4D6B, 1 },
        { "Oxidize",      "Synth",      "degradation as synthesis",                         0xFF4A9E8E, 1 },
        // Sentinel — must remain last
        { nullptr, nullptr, nullptr, 0, 0 },
    };
    // clang-format on
    return kTable;
}

/// Returns the number of engines in the roster (excluding the null sentinel).
inline constexpr std::size_t engineRosterSentinelIndex() noexcept
{
    // Count at compile time is not easily done with a runtime static array.
    // Callers iterate until entry.id == nullptr. This helper exists for documentation.
    return static_cast<std::size_t>(-1); // unused — iterate to sentinel
}

} // namespace xoceanus
