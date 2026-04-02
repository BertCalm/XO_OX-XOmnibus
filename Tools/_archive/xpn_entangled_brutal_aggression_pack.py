#!/usr/bin/env python3
"""
xpn_entangled_brutal_aggression_pack.py
Generates 80 Entangled mood presets, ALL with aggression >= 0.86.

Sub-groups:
- brutal-dense  (20): aggression 0.88-1.0,  density 0.80-1.0,  brightness 0.4-0.7
- brutal-bright (20): aggression 0.86-0.97, brightness 0.70-0.92, movement 0.65-0.90
- brutal-cold   (20): aggression 0.88-0.98, brightness 0.05-0.20, warmth 0.10-0.25
- brutal-movement (20): aggression 0.87-0.97, movement 0.80-0.98, density 0.55-0.80

Writes to Presets/XOceanus/Entangled/. Skips existing files.
"""

import json
import os

PRESET_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "..", "Presets", "XOceanus", "Entangled"
)

# ---------------------------------------------------------------------------
# Preset data: (name, [engineA, engineB], dna, macros, coupling_type, coupling_amount, tags, description)
# ---------------------------------------------------------------------------

BRUTAL_DENSE = [
    (
        "Mass Collision",
        ["OUROBOROS", "OBESE"],
        dict(brightness=0.55, warmth=0.35, movement=0.72, density=0.97, space=0.08, aggression=0.98),
        dict(CHARACTER=0.95, MOVEMENT=0.72, COUPLING=0.92, SPACE=0.08),
        "CHAOS_INJECT", 0.92,
        ["entangled", "brutal", "dense", "ouroboros", "obese", "collision"],
        "Ouroboros strange attractor injecting chaos into OBESE saturation — harmonic mass at total density."
    ),
    (
        "Serpent Belly",
        ["OUROBOROS", "OBESE"],
        dict(brightness=0.48, warmth=0.42, movement=0.65, density=0.95, space=0.06, aggression=0.96),
        dict(CHARACTER=1.0, MOVEMENT=0.65, COUPLING=0.88, SPACE=0.06),
        "SATURATION_DRIVE", 0.88,
        ["entangled", "brutal", "dense", "ouroboros", "obese", "serpent"],
        "MOJO driven by Ouroboros topology loop — the serpent swallowed by its own belly."
    ),
    (
        "Topology Crush",
        ["OBLONG", "OUROBOROS"],
        dict(brightness=0.52, warmth=0.5, movement=0.68, density=0.92, space=0.07, aggression=0.94),
        dict(CHARACTER=0.9, MOVEMENT=0.68, COUPLING=0.85, SPACE=0.07),
        "CHAOS_INJECT", 0.85,
        ["entangled", "brutal", "dense", "oblong", "ouroboros", "topology"],
        "Bob's amber warmth crushed under Ouroboros strange attractor — structure consumed by chaos."
    ),
    (
        "Bob Annihilated",
        ["OBLONG", "OUROBOROS"],
        dict(brightness=0.58, warmth=0.38, movement=0.7, density=0.90, space=0.09, aggression=0.91),
        dict(CHARACTER=0.88, MOVEMENT=0.7, COUPLING=0.82, SPACE=0.09),
        "FREQUENCY_SMEAR", 0.82,
        ["entangled", "brutal", "dense", "oblong", "ouroboros", "annihilation"],
        "Frequency smear from the ouroboros loop dissolves Bob's harmonic order entirely."
    ),
    (
        "Percussion Mass",
        ["OUROBOROS", "ONSET"],
        dict(brightness=0.62, warmth=0.2, movement=0.78, density=0.96, space=0.05, aggression=0.99),
        dict(CHARACTER=0.82, MOVEMENT=0.78, COUPLING=0.95, SPACE=0.05),
        "CHAOS_INJECT", 0.95,
        ["entangled", "brutal", "dense", "ouroboros", "onset", "percussion"],
        "Ouroboros chaos injected into Onset XVC — every drum voice erupting from a fractal attractor."
    ),
    (
        "Drum Singularity",
        ["OUROBOROS", "ONSET"],
        dict(brightness=0.55, warmth=0.15, movement=0.82, density=0.93, space=0.06, aggression=0.97),
        dict(CHARACTER=0.78, MOVEMENT=0.82, COUPLING=0.91, SPACE=0.06),
        "VELOCITY_COUPLE", 0.91,
        ["entangled", "brutal", "dense", "ouroboros", "onset", "singularity"],
        "Ouroboros velocity coupling feeding Onset percussive triggers — a drum singularity event."
    ),
    (
        "Fat Serpent",
        ["OBESE", "OVERBITE"],
        dict(brightness=0.45, warmth=0.45, movement=0.6, density=0.98, space=0.05, aggression=0.97),
        dict(CHARACTER=1.0, MOVEMENT=0.6, COUPLING=0.9, SPACE=0.05),
        "SATURATION_DRIVE", 0.9,
        ["entangled", "brutal", "dense", "obese", "overbite", "saturation"],
        "MOJO and BITE fused — two saturation architectures driving toward thermal collapse."
    ),
    (
        "Venom Overdrive",
        ["OBESE", "OVERBITE"],
        dict(brightness=0.5, warmth=0.4, movement=0.58, density=0.95, space=0.07, aggression=0.95),
        dict(CHARACTER=0.95, MOVEMENT=0.58, COUPLING=0.87, SPACE=0.07),
        "TIMBRAL_WARP", 0.87,
        ["entangled", "brutal", "dense", "obese", "overbite", "venom"],
        "Overbite fang venom soaking OBESE harmonic chain — poisoned saturation at maximum density."
    ),
    (
        "Orca Mass Strike",
        ["ORCA", "OUROBOROS"],
        dict(brightness=0.42, warmth=0.18, movement=0.74, density=0.91, space=0.08, aggression=0.98),
        dict(CHARACTER=0.92, MOVEMENT=0.74, COUPLING=0.93, SPACE=0.08),
        "CHAOS_INJECT", 0.93,
        ["entangled", "brutal", "dense", "orca", "ouroboros", "predator"],
        "Orca echolocation pulse shattered by Ouroboros topology — predator meets singularity."
    ),
    (
        "Deep Predator Loop",
        ["ORCA", "OUROBOROS"],
        dict(brightness=0.38, warmth=0.22, movement=0.7, density=0.89, space=0.1, aggression=0.96),
        dict(CHARACTER=0.88, MOVEMENT=0.7, COUPLING=0.89, SPACE=0.1),
        "FREQUENCY_SMEAR", 0.89,
        ["entangled", "brutal", "dense", "orca", "ouroboros", "deep"],
        "Frequency smear locks Orca breach timing to strange attractor rhythm — looped predation."
    ),
    (
        "Fang Topology",
        ["OVERBITE", "OUROBOROS"],
        dict(brightness=0.6, warmth=0.3, movement=0.72, density=0.94, space=0.06, aggression=0.93),
        dict(CHARACTER=0.9, MOVEMENT=0.72, COUPLING=0.86, SPACE=0.06),
        "CHAOS_INJECT", 0.86,
        ["entangled", "brutal", "dense", "overbite", "ouroboros", "fang"],
        "BITE macro chaos-injected by Ouroboros — the possum's fang at fractal frequency."
    ),
    (
        "Obese Attractor",
        ["OBESE", "OUROBOROS"],
        dict(brightness=0.54, warmth=0.36, movement=0.68, density=0.96, space=0.05, aggression=1.0),
        dict(CHARACTER=1.0, MOVEMENT=0.68, COUPLING=0.94, SPACE=0.05),
        "CHAOS_INJECT", 0.94,
        ["entangled", "brutal", "dense", "obese", "ouroboros", "attractor"],
        "OBESE BELLY maxed, MOJO maxed — Ouroboros strange attractor as the engine's own heartbeat."
    ),
    (
        "Bone Grind",
        ["OBLONG", "OBESE"],
        dict(brightness=0.5, warmth=0.44, movement=0.64, density=0.93, space=0.07, aggression=0.92),
        dict(CHARACTER=0.92, MOVEMENT=0.64, COUPLING=0.84, SPACE=0.07),
        "SATURATION_DRIVE", 0.84,
        ["entangled", "brutal", "dense", "oblong", "obese", "grind"],
        "Bob's amber warmth ground through OBESE saturation — character crushed into harmonic bone dust."
    ),
    (
        "Chaos Percussion",
        ["OBESE", "ONSET"],
        dict(brightness=0.65, warmth=0.25, movement=0.76, density=0.92, space=0.06, aggression=0.94),
        dict(CHARACTER=0.85, MOVEMENT=0.76, COUPLING=0.88, SPACE=0.06),
        "TIMBRAL_WARP", 0.88,
        ["entangled", "brutal", "dense", "obese", "onset", "percussion"],
        "Onset drum voices timbral-warped by OBESE saturation — percussion with infinite harmonic weight."
    ),
    (
        "Fractal Density",
        ["OUROBOROS", "OBLONG"],
        dict(brightness=0.57, warmth=0.48, movement=0.66, density=0.88, space=0.09, aggression=0.90),
        dict(CHARACTER=0.87, MOVEMENT=0.66, COUPLING=0.83, SPACE=0.09),
        "FREQUENCY_SMEAR", 0.83,
        ["entangled", "brutal", "dense", "ouroboros", "oblong", "fractal"],
        "Ouroboros fractal frequency smearing Bob's filter cutoff — warmth buried under mathematical weight."
    ),
    (
        "Kill Density",
        ["ORCA", "ONSET"],
        dict(brightness=0.48, warmth=0.12, movement=0.8, density=0.95, space=0.05, aggression=0.99),
        dict(CHARACTER=0.88, MOVEMENT=0.8, COUPLING=0.93, SPACE=0.05),
        "CHAOS_INJECT", 0.93,
        ["entangled", "brutal", "dense", "orca", "onset", "predator"],
        "Orca kill velocity coupling into Onset XVC — every breach triggers percussion cascade."
    ),
    (
        "Subsonic Crush",
        ["ORCA", "OBESE"],
        dict(brightness=0.4, warmth=0.3, movement=0.65, density=0.97, space=0.06, aggression=0.96),
        dict(CHARACTER=0.95, MOVEMENT=0.65, COUPLING=0.9, SPACE=0.06),
        "SATURATION_DRIVE", 0.9,
        ["entangled", "brutal", "dense", "orca", "obese", "subsonic"],
        "Orca subsonic echolocation crushing OBESE saturation into pure subfrequency mass."
    ),
    (
        "Double Fang",
        ["OVERBITE", "OBESE"],
        dict(brightness=0.55, warmth=0.38, movement=0.62, density=0.90, space=0.08, aggression=0.91),
        dict(CHARACTER=0.93, MOVEMENT=0.62, COUPLING=0.85, SPACE=0.08),
        "TIMBRAL_WARP", 0.85,
        ["entangled", "brutal", "dense", "overbite", "obese", "double-fang"],
        "Both BITE and MOJO maxed and coupled — double bite architecture at total density."
    ),
    (
        "Impact Geometry",
        ["OBLONG", "ONSET"],
        dict(brightness=0.6, warmth=0.42, movement=0.75, density=0.87, space=0.07, aggression=0.89),
        dict(CHARACTER=0.84, MOVEMENT=0.75, COUPLING=0.82, SPACE=0.07),
        "VELOCITY_COUPLE", 0.82,
        ["entangled", "brutal", "dense", "oblong", "onset", "impact"],
        "Bob's filter geometry velocity-coupled to Onset drum voices — warm percussion at maximum density."
    ),
    (
        "Serpent Strike",
        ["OUROBOROS", "OVERBITE"],
        dict(brightness=0.58, warmth=0.28, movement=0.7, density=0.91, space=0.07, aggression=0.93),
        dict(CHARACTER=0.9, MOVEMENT=0.7, COUPLING=0.87, SPACE=0.07),
        "CHAOS_INJECT", 0.87,
        ["entangled", "brutal", "dense", "ouroboros", "overbite", "serpent"],
        "Ouroboros topology chaos feeding Overbite BITE macro — fractal fang."
    ),
]

BRUTAL_BRIGHT = [
    (
        "Phosphor Riot",
        ["OPTIC", "OUROBOROS"],
        dict(brightness=0.92, warmth=0.12, movement=0.88, density=0.72, space=0.18, aggression=0.94),
        dict(CHARACTER=0.78, MOVEMENT=0.88, COUPLING=0.86, SPACE=0.18),
        "CHAOS_INJECT", 0.86,
        ["entangled", "brutal", "bright", "optic", "ouroboros", "phosphor"],
        "Optic AutoPulse at maximum rate chaos-injected by Ouroboros — light as pure mathematical violence."
    ),
    (
        "Light Singularity",
        ["OPTIC", "OUROBOROS"],
        dict(brightness=0.88, warmth=0.08, movement=0.82, density=0.68, space=0.22, aggression=0.92),
        dict(CHARACTER=0.72, MOVEMENT=0.82, COUPLING=0.84, SPACE=0.22),
        "TIMBRAL_WARP", 0.84,
        ["entangled", "brutal", "bright", "optic", "ouroboros", "light"],
        "Ouroboros timbral warp modulating Optic luminance — bright singularity event."
    ),
    (
        "Prism Shatter Storm",
        ["OBLIQUE", "ONSET"],
        dict(brightness=0.85, warmth=0.15, movement=0.87, density=0.78, space=0.2, aggression=0.93),
        dict(CHARACTER=0.84, MOVEMENT=0.87, COUPLING=0.88, SPACE=0.2),
        "FREQUENCY_SMEAR", 0.88,
        ["entangled", "brutal", "bright", "oblique", "onset", "prism"],
        "Prism bounce frequency smearing Onset percussion — shattered light spectrum as percussion."
    ),
    (
        "Bounce Cascade",
        ["OBLIQUE", "ONSET"],
        dict(brightness=0.82, warmth=0.2, movement=0.84, density=0.74, space=0.24, aggression=0.90),
        dict(CHARACTER=0.82, MOVEMENT=0.84, COUPLING=0.85, SPACE=0.24),
        "VELOCITY_COUPLE", 0.85,
        ["entangled", "brutal", "bright", "oblique", "onset", "bounce"],
        "RTJ-mode prism with velocity coupling to Onset — percussive light cascade."
    ),
    (
        "Chip Fury",
        ["OVERWORLD", "OUROBOROS"],
        dict(brightness=0.90, warmth=0.08, movement=0.85, density=0.75, space=0.15, aggression=0.95),
        dict(CHARACTER=0.82, MOVEMENT=0.85, COUPLING=0.89, SPACE=0.15),
        "CHAOS_INJECT", 0.89,
        ["entangled", "brutal", "bright", "overworld", "ouroboros", "chip"],
        "NES 2A03 glitch noise at maximum chaos injection from Ouroboros — chip fury without ceiling."
    ),
    (
        "Era Collapse",
        ["OVERWORLD", "OUROBOROS"],
        dict(brightness=0.87, warmth=0.1, movement=0.80, density=0.78, space=0.17, aggression=0.92),
        dict(CHARACTER=0.79, MOVEMENT=0.80, COUPLING=0.86, SPACE=0.17),
        "FREQUENCY_SMEAR", 0.86,
        ["entangled", "brutal", "bright", "overworld", "ouroboros", "era"],
        "ERA triangle cornered in NES while Ouroboros smears frequencies — chip era collapses."
    ),
    (
        "Phosphor Topology",
        ["OBLONG", "OPTIC"],
        dict(brightness=0.86, warmth=0.28, movement=0.72, density=0.70, space=0.2, aggression=0.89),
        dict(CHARACTER=0.74, MOVEMENT=0.72, COUPLING=0.83, SPACE=0.2),
        "TIMBRAL_WARP", 0.83,
        ["entangled", "brutal", "bright", "oblong", "optic", "phosphor"],
        "Bob's warmth timbral-warped by Optic luminance modulation — amber light at brutal intensity."
    ),
    (
        "Bob Illuminated",
        ["OBLONG", "OPTIC"],
        dict(brightness=0.82, warmth=0.32, movement=0.68, density=0.68, space=0.22, aggression=0.87),
        dict(CHARACTER=0.72, MOVEMENT=0.68, COUPLING=0.80, SPACE=0.22),
        "VELOCITY_COUPLE", 0.8,
        ["entangled", "brutal", "bright", "oblong", "optic", "illuminated"],
        "Optic velocity signal driving Bob's filter — amber warmth at maximum luminous aggression."
    ),
    (
        "Brass Phosphor",
        ["OTTONI", "OUROBOROS"],
        dict(brightness=0.80, warmth=0.22, movement=0.75, density=0.76, space=0.2, aggression=0.91),
        dict(CHARACTER=0.86, MOVEMENT=0.75, COUPLING=0.87, SPACE=0.2),
        "CHAOS_INJECT", 0.87,
        ["entangled", "brutal", "bright", "ottoni", "ouroboros", "brass"],
        "Triple brass GROW macro with Ouroboros chaos injection — harmonic brass in fractal formation."
    ),
    (
        "Grow Singularity",
        ["OTTONI", "OUROBOROS"],
        dict(brightness=0.76, warmth=0.18, movement=0.78, density=0.74, space=0.18, aggression=0.88),
        dict(CHARACTER=0.84, MOVEMENT=0.78, COUPLING=0.84, SPACE=0.18),
        "TIMBRAL_WARP", 0.84,
        ["entangled", "brutal", "bright", "ottoni", "ouroboros", "grow"],
        "GROW macro timbral-warped by Ouroboros topology — brass expansion without harmonic ceiling."
    ),
    (
        "White Strike",
        ["OVERBITE", "OPTIC"],
        dict(brightness=0.91, warmth=0.1, movement=0.82, density=0.72, space=0.2, aggression=0.96),
        dict(CHARACTER=0.88, MOVEMENT=0.82, COUPLING=0.90, SPACE=0.2),
        "TIMBRAL_WARP", 0.9,
        ["entangled", "brutal", "bright", "overbite", "optic", "white-strike"],
        "Fang White meets Phosphor Green — BITE macro timbral-warped by Optic luminance."
    ),
    (
        "Prism Fang",
        ["OBLIQUE", "OVERBITE"],
        dict(brightness=0.84, warmth=0.16, movement=0.76, density=0.71, space=0.22, aggression=0.90),
        dict(CHARACTER=0.86, MOVEMENT=0.76, COUPLING=0.85, SPACE=0.22),
        "FREQUENCY_SMEAR", 0.85,
        ["entangled", "brutal", "bright", "oblique", "overbite", "prism-fang"],
        "Prism bounce frequency smearing Overbite resonance — bright venom across the spectrum."
    ),
    (
        "Glitch Barrage",
        ["OVERWORLD", "ONSET"],
        dict(brightness=0.88, warmth=0.12, movement=0.86, density=0.8, space=0.15, aggression=0.93),
        dict(CHARACTER=0.8, MOVEMENT=0.86, COUPLING=0.88, SPACE=0.15),
        "VELOCITY_COUPLE", 0.88,
        ["entangled", "brutal", "bright", "overworld", "onset", "glitch"],
        "NES glitch velocity coupling into Onset XVC — chip noise driving percussion barrage."
    ),
    (
        "Light Percussion",
        ["OPTIC", "ONSET"],
        dict(brightness=0.92, warmth=0.06, movement=0.88, density=0.75, space=0.18, aggression=0.94),
        dict(CHARACTER=0.76, MOVEMENT=0.88, COUPLING=0.90, SPACE=0.18),
        "VELOCITY_COUPLE", 0.9,
        ["entangled", "brutal", "bright", "optic", "onset", "light"],
        "Optic AutoPulse velocity triggering Onset percussive events — light as drumstick."
    ),
    (
        "Neon Rupture",
        ["OVERWORLD", "OBESE"],
        dict(brightness=0.85, warmth=0.2, movement=0.78, density=0.82, space=0.14, aggression=0.92),
        dict(CHARACTER=0.86, MOVEMENT=0.78, COUPLING=0.86, SPACE=0.14),
        "SATURATION_DRIVE", 0.86,
        ["entangled", "brutal", "bright", "overworld", "obese", "neon"],
        "Overworld neon green running through OBESE saturation — chip signal with infinite harmonic weight."
    ),
    (
        "Era Saturation",
        ["OVERWORLD", "OBESE"],
        dict(brightness=0.82, warmth=0.25, movement=0.72, density=0.85, space=0.12, aggression=0.90),
        dict(CHARACTER=0.88, MOVEMENT=0.72, COUPLING=0.84, SPACE=0.12),
        "TIMBRAL_WARP", 0.84,
        ["entangled", "brutal", "bright", "overworld", "obese", "era"],
        "ERA crossfade timbral-warped by OBESE mojo — chip eras dissolving in saturation heat."
    ),
    (
        "Optic Serpent",
        ["OPTIC", "OUROBOROS"],
        dict(brightness=0.90, warmth=0.05, movement=0.86, density=0.66, space=0.24, aggression=0.95),
        dict(CHARACTER=0.7, MOVEMENT=0.86, COUPLING=0.91, SPACE=0.24),
        "CHAOS_INJECT", 0.91,
        ["entangled", "brutal", "bright", "optic", "ouroboros", "serpent"],
        "Ouroboros topology channeled through Optic luminance — the strange attractor made of light."
    ),
    (
        "Bounce Riot",
        ["OBLIQUE", "OUROBOROS"],
        dict(brightness=0.87, warmth=0.14, movement=0.85, density=0.74, space=0.2, aggression=0.93),
        dict(CHARACTER=0.84, MOVEMENT=0.85, COUPLING=0.88, SPACE=0.2),
        "CHAOS_INJECT", 0.88,
        ["entangled", "brutal", "bright", "oblique", "ouroboros", "bounce"],
        "Prism walls chaos-injected by Ouroboros — the bounce has no predictable reflection."
    ),
    (
        "Brass Storm",
        ["OTTONI", "ONSET"],
        dict(brightness=0.78, warmth=0.2, movement=0.82, density=0.77, space=0.18, aggression=0.91),
        dict(CHARACTER=0.82, MOVEMENT=0.82, COUPLING=0.87, SPACE=0.18),
        "VELOCITY_COUPLE", 0.87,
        ["entangled", "brutal", "bright", "ottoni", "onset", "brass"],
        "Ottoni triple brass velocity coupled into Onset percussion — martial brass storm."
    ),
    (
        "Neon Perc Surge",
        ["OVERWORLD", "OBLIQUE"],
        dict(brightness=0.89, warmth=0.1, movement=0.84, density=0.7, space=0.2, aggression=0.91),
        dict(CHARACTER=0.8, MOVEMENT=0.84, COUPLING=0.85, SPACE=0.2),
        "TIMBRAL_WARP", 0.85,
        ["entangled", "brutal", "bright", "overworld", "oblique", "neon"],
        "Overworld chip timbral-warped by Oblique prism — neon spectrum through bounced geometry."
    ),
]

BRUTAL_COLD = [
    (
        "Obsidian Rupture",
        ["OBSIDIAN", "OUROBOROS"],
        dict(brightness=0.08, warmth=0.12, movement=0.65, density=0.9, space=0.12, aggression=0.97),
        dict(CHARACTER=0.92, MOVEMENT=0.65, COUPLING=0.93, SPACE=0.12),
        "CHAOS_INJECT", 0.93,
        ["entangled", "brutal", "cold", "obsidian", "ouroboros", "rupture"],
        "Crystal surface shattered by Ouroboros strange attractor — cold brutality as topology event."
    ),
    (
        "Crystal Topology",
        ["OBSIDIAN", "OUROBOROS"],
        dict(brightness=0.12, warmth=0.10, movement=0.60, density=0.88, space=0.10, aggression=0.95),
        dict(CHARACTER=0.90, MOVEMENT=0.60, COUPLING=0.90, SPACE=0.10),
        "FREQUENCY_SMEAR", 0.9,
        ["entangled", "brutal", "cold", "obsidian", "ouroboros", "crystal"],
        "Frequency smear from the topology loop corrupting Obsidian's crystal silence."
    ),
    (
        "Grey Percussion",
        ["OMBRE", "ONSET"],
        dict(brightness=0.15, warmth=0.20, movement=0.70, density=0.85, space=0.14, aggression=0.94),
        dict(CHARACTER=0.86, MOVEMENT=0.70, COUPLING=0.88, SPACE=0.14),
        "VELOCITY_COUPLE", 0.88,
        ["entangled", "brutal", "cold", "ombre", "onset", "grey"],
        "Ombre shadow mauve velocity coupling into Onset — grey percussion with zero warmth."
    ),
    (
        "Memory Barrage",
        ["OMBRE", "ONSET"],
        dict(brightness=0.18, warmth=0.16, movement=0.75, density=0.88, space=0.12, aggression=0.92),
        dict(CHARACTER=0.84, MOVEMENT=0.75, COUPLING=0.86, SPACE=0.12),
        "CHAOS_INJECT", 0.86,
        ["entangled", "brutal", "cold", "ombre", "onset", "memory"],
        "Ombre memory layer chaos-injected into Onset percussion — grief as machine barrage."
    ),
    (
        "Phantom Architecture",
        ["OBSCURA", "OUROBOROS"],
        dict(brightness=0.14, warmth=0.18, movement=0.62, density=0.86, space=0.16, aggression=0.96),
        dict(CHARACTER=0.88, MOVEMENT=0.62, COUPLING=0.92, SPACE=0.16),
        "CHAOS_INJECT", 0.92,
        ["entangled", "brutal", "cold", "obscura", "ouroboros", "phantom"],
        "Daguerreotype silver structure chaos-injected by Ouroboros — the photograph as topological weapon."
    ),
    (
        "Silver Topology",
        ["OBSCURA", "OUROBOROS"],
        dict(brightness=0.10, warmth=0.14, movement=0.58, density=0.84, space=0.14, aggression=0.93),
        dict(CHARACTER=0.86, MOVEMENT=0.58, COUPLING=0.89, SPACE=0.14),
        "TIMBRAL_WARP", 0.89,
        ["entangled", "brutal", "cold", "obscura", "ouroboros", "silver"],
        "Ouroboros timbral warp corrupting Obscura plate stiffness — silver daguerreotype at fractal resolution."
    ),
    (
        "Axolotl Freeze",
        ["ODDOSCAR", "OUROBOROS"],
        dict(brightness=0.16, warmth=0.22, movement=0.64, density=0.87, space=0.12, aggression=0.95),
        dict(CHARACTER=0.90, MOVEMENT=0.64, COUPLING=0.91, SPACE=0.12),
        "CHAOS_INJECT", 0.91,
        ["entangled", "brutal", "cold", "oddoscar", "ouroboros", "axolotl"],
        "Axolotl morph parameter chaos-injected by Ouroboros topology — gill pink frozen at fractal cold."
    ),
    (
        "Morph Singularity",
        ["ODDOSCAR", "OUROBOROS"],
        dict(brightness=0.12, warmth=0.18, movement=0.60, density=0.91, space=0.10, aggression=0.98),
        dict(CHARACTER=0.92, MOVEMENT=0.60, COUPLING=0.94, SPACE=0.10),
        "FREQUENCY_SMEAR", 0.94,
        ["entangled", "brutal", "cold", "oddoscar", "ouroboros", "morph"],
        "Morph parameter frequency-smeared by Ouroboros — axolotl regeneration as brutal topology."
    ),
    (
        "Orca Frost",
        ["OBSIDIAN", "ORCA"],
        dict(brightness=0.08, warmth=0.10, movement=0.68, density=0.92, space=0.08, aggression=0.97),
        dict(CHARACTER=0.93, MOVEMENT=0.68, COUPLING=0.93, SPACE=0.08),
        "VELOCITY_COUPLE", 0.93,
        ["entangled", "brutal", "cold", "obsidian", "orca", "frost"],
        "Orca echolocation pulse beneath Obsidian crystal density — deep cold with predator velocity."
    ),
    (
        "Abyss Predator",
        ["OBSIDIAN", "ORCA"],
        dict(brightness=0.06, warmth=0.08, movement=0.72, density=0.93, space=0.09, aggression=0.99),
        dict(CHARACTER=0.94, MOVEMENT=0.72, COUPLING=0.95, SPACE=0.09),
        "CHAOS_INJECT", 0.95,
        ["entangled", "brutal", "cold", "obsidian", "orca", "abyss"],
        "Orca chaos injected into Obsidian crystal — the predator in the abyss, zero warmth, maximum kill."
    ),
    (
        "Silver Serpent",
        ["OBSCURA", "ORCA"],
        dict(brightness=0.12, warmth=0.15, movement=0.70, density=0.88, space=0.13, aggression=0.94),
        dict(CHARACTER=0.88, MOVEMENT=0.70, COUPLING=0.90, SPACE=0.13),
        "TIMBRAL_WARP", 0.9,
        ["entangled", "brutal", "cold", "obscura", "orca", "silver"],
        "Orca breach timbral-warping Obscura stiffness model — cold silver with hunter's velocity."
    ),
    (
        "Forgetting Strike",
        ["OMBRE", "OUROBOROS"],
        dict(brightness=0.14, warmth=0.20, movement=0.65, density=0.89, space=0.12, aggression=0.96),
        dict(CHARACTER=0.88, MOVEMENT=0.65, COUPLING=0.92, SPACE=0.12),
        "CHAOS_INJECT", 0.92,
        ["entangled", "brutal", "cold", "ombre", "ouroboros", "forgetting"],
        "Forgetting layer chaos-injected by Ouroboros topology — the narrative erased by mathematics."
    ),
    (
        "Shadow Topology",
        ["OMBRE", "OUROBOROS"],
        dict(brightness=0.10, warmth=0.15, movement=0.68, density=0.86, space=0.15, aggression=0.93),
        dict(CHARACTER=0.85, MOVEMENT=0.68, COUPLING=0.88, SPACE=0.15),
        "FREQUENCY_SMEAR", 0.88,
        ["entangled", "brutal", "cold", "ombre", "ouroboros", "shadow"],
        "Shadow mauve frequency-smeared by Ouroboros — the memory's topology consumed by chaos."
    ),
    (
        "Daguerreotype Violence",
        ["OBSCURA", "ONSET"],
        dict(brightness=0.16, warmth=0.18, movement=0.72, density=0.84, space=0.14, aggression=0.91),
        dict(CHARACTER=0.82, MOVEMENT=0.72, COUPLING=0.86, SPACE=0.14),
        "VELOCITY_COUPLE", 0.86,
        ["entangled", "brutal", "cold", "obscura", "onset", "daguerreotype"],
        "Obscura plate stiffness velocity coupling into Onset percussion — frozen image as drumbeat."
    ),
    (
        "Cold Machine",
        ["OBSIDIAN", "ONSET"],
        dict(brightness=0.09, warmth=0.11, movement=0.76, density=0.90, space=0.10, aggression=0.98),
        dict(CHARACTER=0.90, MOVEMENT=0.76, COUPLING=0.94, SPACE=0.10),
        "VELOCITY_COUPLE", 0.94,
        ["entangled", "brutal", "cold", "obsidian", "onset", "machine"],
        "Obsidian crystal density driving Onset percussion velocity — cold machine with perfect silence between hits."
    ),
    (
        "Frost Venom",
        ["OBSIDIAN", "OVERBITE"],
        dict(brightness=0.11, warmth=0.13, movement=0.60, density=0.91, space=0.10, aggression=0.95),
        dict(CHARACTER=0.92, MOVEMENT=0.60, COUPLING=0.91, SPACE=0.10),
        "TIMBRAL_WARP", 0.91,
        ["entangled", "brutal", "cold", "obsidian", "overbite", "frost"],
        "Overbite BITE timbral-warping Obsidian crystal — fang at absolute zero."
    ),
    (
        "Ombre Machine",
        ["OMBRE", "ORCA"],
        dict(brightness=0.13, warmth=0.16, movement=0.72, density=0.87, space=0.12, aggression=0.94),
        dict(CHARACTER=0.86, MOVEMENT=0.72, COUPLING=0.90, SPACE=0.12),
        "CHAOS_INJECT", 0.9,
        ["entangled", "brutal", "cold", "ombre", "orca", "machine"],
        "Orca predator chaos injected into Ombre dual narrative — the hunt as cold memory."
    ),
    (
        "Axolotl Strike",
        ["ODDOSCAR", "ORCA"],
        dict(brightness=0.15, warmth=0.20, movement=0.74, density=0.88, space=0.11, aggression=0.93),
        dict(CHARACTER=0.88, MOVEMENT=0.74, COUPLING=0.89, SPACE=0.11),
        "VELOCITY_COUPLE", 0.89,
        ["entangled", "brutal", "cold", "oddoscar", "orca", "axolotl"],
        "Orca strike velocity coupled into Oddoscar morph — gill pink meets apex predator cold."
    ),
    (
        "Silver Percussion",
        ["OBSCURA", "ORCA"],
        dict(brightness=0.08, warmth=0.12, movement=0.76, density=0.86, space=0.13, aggression=0.95),
        dict(CHARACTER=0.87, MOVEMENT=0.76, COUPLING=0.91, SPACE=0.13),
        "VELOCITY_COUPLE", 0.91,
        ["entangled", "brutal", "cold", "obscura", "orca", "silver"],
        "Orca velocity driving Obscura stiffness — silver daguerreotype percussion at predator tempo."
    ),
    (
        "Crystal Morph",
        ["OBSIDIAN", "ODDOSCAR"],
        dict(brightness=0.07, warmth=0.14, movement=0.58, density=0.93, space=0.09, aggression=0.97),
        dict(CHARACTER=0.93, MOVEMENT=0.58, COUPLING=0.92, SPACE=0.09),
        "TIMBRAL_WARP", 0.92,
        ["entangled", "brutal", "cold", "obsidian", "oddoscar", "crystal"],
        "Axolotl morph timbral-warped by Obsidian crystal density — regeneration under absolute cold."
    ),
]

BRUTAL_MOVEMENT = [
    (
        "Arm Chaos Surge",
        ["OUTWIT", "OUROBOROS"],
        dict(brightness=0.58, warmth=0.25, movement=0.96, density=0.72, space=0.16, aggression=0.97),
        dict(CHARACTER=0.84, MOVEMENT=0.96, COUPLING=0.93, SPACE=0.16),
        "CHAOS_INJECT", 0.93,
        ["entangled", "brutal", "movement", "outwit", "ouroboros", "kinetic"],
        "Outwit 8-arm Wolfram CA chaos-injected by Ouroboros topology — cellular automation in fractal storm."
    ),
    (
        "Wolfram Attractor",
        ["OUTWIT", "OUROBOROS"],
        dict(brightness=0.52, warmth=0.20, movement=0.92, density=0.68, space=0.18, aggression=0.95),
        dict(CHARACTER=0.80, MOVEMENT=0.92, COUPLING=0.90, SPACE=0.18),
        "TIMBRAL_WARP", 0.9,
        ["entangled", "brutal", "movement", "outwit", "ouroboros", "wolfram"],
        "Ouroboros timbral warp modulating Outwit arm evolution rate — strange attractor as cellular rule."
    ),
    (
        "Ink Percussion",
        ["OCTOPUS", "ONSET"],
        dict(brightness=0.62, warmth=0.22, movement=0.94, density=0.75, space=0.15, aggression=0.96),
        dict(CHARACTER=0.82, MOVEMENT=0.94, COUPLING=0.91, SPACE=0.15),
        "VELOCITY_COUPLE", 0.91,
        ["entangled", "brutal", "movement", "octopus", "onset", "ink"],
        "Octopus eight arms velocity coupled into Onset XVC — ink cloud as percussion weapon."
    ),
    (
        "Arm Drum Surge",
        ["OCTOPUS", "ONSET"],
        dict(brightness=0.58, warmth=0.18, movement=0.91, density=0.72, space=0.17, aggression=0.93),
        dict(CHARACTER=0.78, MOVEMENT=0.91, COUPLING=0.88, SPACE=0.17),
        "CHAOS_INJECT", 0.88,
        ["entangled", "brutal", "movement", "octopus", "onset", "arm"],
        "Octopus CA rule 30 chaos injected into Onset drum voices — alien intelligence driving drums."
    ),
    (
        "World Percussion",
        ["OVERWORLD", "ONSET"],
        dict(brightness=0.82, warmth=0.15, movement=0.95, density=0.70, space=0.14, aggression=0.97),
        dict(CHARACTER=0.78, MOVEMENT=0.95, COUPLING=0.92, SPACE=0.14),
        "VELOCITY_COUPLE", 0.92,
        ["entangled", "brutal", "movement", "overworld", "onset", "world"],
        "Overworld chip oscillator velocity coupling into Onset — NES era collapse as kinetic percussion."
    ),
    (
        "Glitch Rhythm",
        ["OVERWORLD", "ONSET"],
        dict(brightness=0.85, warmth=0.12, movement=0.90, density=0.68, space=0.16, aggression=0.94),
        dict(CHARACTER=0.75, MOVEMENT=0.90, COUPLING=0.89, SPACE=0.16),
        "CHAOS_INJECT", 0.89,
        ["entangled", "brutal", "movement", "overworld", "onset", "glitch"],
        "NES glitch chaos injected into Onset XVC — chip noise as rhythm engine at maximum velocity."
    ),
    (
        "Photon Cascade",
        ["OPTIC", "ONSET"],
        dict(brightness=0.90, warmth=0.08, movement=0.96, density=0.65, space=0.18, aggression=0.95),
        dict(CHARACTER=0.72, MOVEMENT=0.96, COUPLING=0.92, SPACE=0.18),
        "VELOCITY_COUPLE", 0.92,
        ["entangled", "brutal", "movement", "optic", "onset", "photon"],
        "Optic AutoPulse velocity triggering Onset percussion at maximum kinetic rate — light percussion."
    ),
    (
        "Light Rhythm",
        ["OPTIC", "ONSET"],
        dict(brightness=0.88, warmth=0.06, movement=0.93, density=0.62, space=0.20, aggression=0.92),
        dict(CHARACTER=0.70, MOVEMENT=0.93, COUPLING=0.89, SPACE=0.20),
        "TIMBRAL_WARP", 0.89,
        ["entangled", "brutal", "movement", "optic", "onset", "light"],
        "Optic timbral-warping Onset drum timbre in real time — light shaping percussion."
    ),
    (
        "Origami Storm",
        ["ORIGAMI", "OUROBOROS"],
        dict(brightness=0.68, warmth=0.28, movement=0.88, density=0.74, space=0.18, aggression=0.94),
        dict(CHARACTER=0.86, MOVEMENT=0.88, COUPLING=0.90, SPACE=0.18),
        "CHAOS_INJECT", 0.9,
        ["entangled", "brutal", "movement", "origami", "ouroboros", "storm"],
        "Origami fold point chaos-injected by Ouroboros — paper crane in topological storm."
    ),
    (
        "Vermillion Attractor",
        ["ORIGAMI", "OUROBOROS"],
        dict(brightness=0.72, warmth=0.24, movement=0.84, density=0.70, space=0.20, aggression=0.91),
        dict(CHARACTER=0.84, MOVEMENT=0.84, COUPLING=0.87, SPACE=0.20),
        "TIMBRAL_WARP", 0.87,
        ["entangled", "brutal", "movement", "origami", "ouroboros", "vermillion"],
        "Ouroboros timbral warping Origami fold timbre — vermillion crease fractal."
    ),
    (
        "Arm Topology",
        ["OUTWIT", "ONSET"],
        dict(brightness=0.60, warmth=0.20, movement=0.98, density=0.68, space=0.14, aggression=0.97),
        dict(CHARACTER=0.80, MOVEMENT=0.98, COUPLING=0.93, SPACE=0.14),
        "VELOCITY_COUPLE", 0.93,
        ["entangled", "brutal", "movement", "outwit", "onset", "arm"],
        "Outwit 8-arm velocity coupled into Onset XVC — Wolfram CA feeding drum velocity at maximum kinetics."
    ),
    (
        "Cellular Drum",
        ["OUTWIT", "ONSET"],
        dict(brightness=0.55, warmth=0.18, movement=0.95, density=0.72, space=0.16, aggression=0.95),
        dict(CHARACTER=0.78, MOVEMENT=0.95, COUPLING=0.90, SPACE=0.16),
        "CHAOS_INJECT", 0.9,
        ["entangled", "brutal", "movement", "outwit", "onset", "cellular"],
        "Outwit CA chaos injected into Onset percussive triggers — alien cellular intelligence as drum computer."
    ),
    (
        "Prism Surge",
        ["OPTIC", "OUROBOROS"],
        dict(brightness=0.89, warmth=0.10, movement=0.91, density=0.66, space=0.20, aggression=0.95),
        dict(CHARACTER=0.74, MOVEMENT=0.91, COUPLING=0.91, SPACE=0.20),
        "CHAOS_INJECT", 0.91,
        ["entangled", "brutal", "movement", "optic", "ouroboros", "prism"],
        "Optic luminance driven by Ouroboros topology — fractal light at kinetic velocity."
    ),
    (
        "Chromatophore Chase",
        ["OCTOPUS", "OUROBOROS"],
        dict(brightness=0.65, warmth=0.22, movement=0.94, density=0.70, space=0.18, aggression=0.94),
        dict(CHARACTER=0.80, MOVEMENT=0.94, COUPLING=0.91, SPACE=0.18),
        "CHAOS_INJECT", 0.91,
        ["entangled", "brutal", "movement", "octopus", "ouroboros", "chromatophore"],
        "Octopus chromatophore modulators chaos-injected by Ouroboros — color change at topological velocity."
    ),
    (
        "Ink Storm",
        ["OCTOPUS", "OUROBOROS"],
        dict(brightness=0.60, warmth=0.20, movement=0.90, density=0.73, space=0.17, aggression=0.91),
        dict(CHARACTER=0.78, MOVEMENT=0.90, COUPLING=0.88, SPACE=0.17),
        "TIMBRAL_WARP", 0.88,
        ["entangled", "brutal", "movement", "octopus", "ouroboros", "ink"],
        "Ouroboros timbral warping Octopus ink cloud mix — chromatophore explosion in topological space."
    ),
    (
        "Fold Velocity",
        ["ORIGAMI", "ONSET"],
        dict(brightness=0.70, warmth=0.26, movement=0.90, density=0.69, space=0.18, aggression=0.91),
        dict(CHARACTER=0.83, MOVEMENT=0.90, COUPLING=0.87, SPACE=0.18),
        "VELOCITY_COUPLE", 0.87,
        ["entangled", "brutal", "movement", "origami", "onset", "fold"],
        "Origami fold velocity coupled into Onset percussion — paper crane at drumstick speed."
    ),
    (
        "Shuriken Barrage",
        ["ORIGAMI", "ONSET"],
        dict(brightness=0.74, warmth=0.22, movement=0.93, density=0.67, space=0.16, aggression=0.93),
        dict(CHARACTER=0.84, MOVEMENT=0.93, COUPLING=0.89, SPACE=0.16),
        "CHAOS_INJECT", 0.89,
        ["entangled", "brutal", "movement", "origami", "onset", "shuriken"],
        "Origami crane chaos-injected into Onset trigger rate — folded paper as weaponized percussion."
    ),
    (
        "Eight Arm Riot",
        ["OCTOPUS", "OBESE"],
        dict(brightness=0.64, warmth=0.30, movement=0.88, density=0.76, space=0.16, aggression=0.93),
        dict(CHARACTER=0.86, MOVEMENT=0.88, COUPLING=0.88, SPACE=0.16),
        "SATURATION_DRIVE", 0.88,
        ["entangled", "brutal", "movement", "octopus", "obese", "eight-arm"],
        "Octopus eight arms saturated by OBESE MOJO — decentralized alien intelligence at harmonic maximum."
    ),
    (
        "Kinetic Saturation",
        ["OCTOPUS", "OBESE"],
        dict(brightness=0.62, warmth=0.28, movement=0.85, density=0.79, space=0.15, aggression=0.91),
        dict(CHARACTER=0.88, MOVEMENT=0.85, COUPLING=0.86, SPACE=0.15),
        "TIMBRAL_WARP", 0.86,
        ["entangled", "brutal", "movement", "octopus", "obese", "kinetic"],
        "Octopus arm timbre warped by OBESE saturation — kinetic intelligence at full harmonic load."
    ),
    (
        "Cellular Predator",
        ["OUTWIT", "ORCA"],
        dict(brightness=0.50, warmth=0.15, movement=0.96, density=0.65, space=0.16, aggression=0.97),
        dict(CHARACTER=0.82, MOVEMENT=0.96, COUPLING=0.93, SPACE=0.16),
        "VELOCITY_COUPLE", 0.93,
        ["entangled", "brutal", "movement", "outwit", "orca", "cellular"],
        "Orca hunt velocity coupling into Outwit arm evolution — apex predator driving cellular automata."
    ),
]

# ---------------------------------------------------------------------------

def make_preset(name, engines, dna, macros, coupling_type, coupling_amount, tags, description):
    coupling_intensity = "High" if coupling_amount >= 0.8 else "Moderate"
    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": engines,
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "macros": macros,
        "couplingIntensity": coupling_intensity,
        "dna": dna,
        "sonic_dna": dna,
        "parameters": {},
        "coupling": {
            "pairs": [
                {
                    "engineA": engines[0],
                    "engineB": engines[1],
                    "type": coupling_type,
                    "amount": coupling_amount
                }
            ]
        }
    }


def filename_from_name(name):
    safe = "".join(c if c.isalnum() or c in " -'" else "" for c in name)
    safe = safe.strip().replace(" ", "_")
    return safe + ".xometa"


def main():
    os.makedirs(PRESET_DIR, exist_ok=True)

    all_groups = [
        ("brutal-dense", BRUTAL_DENSE),
        ("brutal-bright", BRUTAL_BRIGHT),
        ("brutal-cold", BRUTAL_COLD),
        ("brutal-movement", BRUTAL_MOVEMENT),
    ]

    written = 0
    skipped = 0

    for group_name, presets in all_groups:
        print(f"\n--- {group_name} ({len(presets)} presets) ---")
        for entry in presets:
            name, engines, dna, macros, coupling_type, coupling_amount, tags, description = entry

            # Validate aggression
            agg = dna["aggression"]
            if agg < 0.86:
                print(f"  WARN  {name} — aggression {agg} below 0.86, skipping")
                continue

            fname = filename_from_name(name)
            fpath = os.path.join(PRESET_DIR, fname)

            if os.path.exists(fpath):
                print(f"  SKIP  {fname}")
                skipped += 1
                continue

            preset = make_preset(name, engines, dna, macros, coupling_type, coupling_amount, tags, description)
            with open(fpath, "w", encoding="utf-8") as f:
                json.dump(preset, f, indent=2)
                f.write("\n")

            print(f"  WRITE {fname}  [agg={agg}]")
            written += 1

    print(f"\nDone. Written: {written}, Skipped: {skipped}")
    print(f"Total presets defined: {sum(len(g[1]) for g in all_groups)}")


if __name__ == "__main__":
    main()
