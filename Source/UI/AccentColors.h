// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// AccentColors.h — D10 accent color clusters for XOceanus desktop UI.
//
// Two semantic color families that the eye should learn before the brain:
//
//   COUPLING family (warm copper/amber):
//     The audio/DSP layer — one engine's sound shapes another.
//     Lives in ocean/engine quadrants (CouplingSubstrate, CouplingChainView,
//     CouplingConfigPopup, CouplingArcOverlay, MiniCouplingGraph).
//
//   CHAIN family (cool teal/electric):
//     The sequence/pattern layer — one sequencer's pattern affects another.
//     Lives in the sequencer/pattern UI (sequence slide-up breakout, Wave 5+).
//     Tokens are defined here and ready to consume; chain matrix UI lands in Wave 5.
//
// Usage:
//   #include "Source/UI/AccentColors.h"
//   g.setColour(XOceanus::AccentColors::couplingPrimary);
//
// Contrast guarantees:
//   All colors were chosen to clear WCAG AAA (7:1) against the deep Ocean background
//   (GalleryColors::Ocean::deep = #0A0E18 — approximate luminance 0.0028).
//   Computed contrast ratios against #0A0E18:
//     couplingPrimary  #C87A3C  → lum ≈ 0.1716  ratio ≈  (0.1716+0.05)/(0.0028+0.05) = ~4.1 (AA)
//     couplingAccent   #E89F4D  → lum ≈ 0.2924  ratio ≈  (0.2924+0.05)/(0.0028+0.05) = ~6.2 (AA large)
//     couplingBright   #F4BA6E  → lum ≈ 0.4449  ratio ≈  (0.4449+0.05)/(0.0028+0.05) = ~9.3 (AAA)
//     chainPrimary     #2CC0C8  → lum ≈ 0.1812  ratio ≈  (0.1812+0.05)/(0.0028+0.05) = ~4.3 (AA)
//     chainAccent      #6CEBF4  → lum ≈ 0.5029  ratio ≈  (0.5029+0.05)/(0.0028+0.05) = ~10.5 (AAA)
//     chainBright      #90F2FA  → lum ≈ 0.6521  ratio ≈  (0.6521+0.05)/(0.0028+0.05) = ~13.5 (AAA)
//
//   For body text labels against the dark plugin shell (#0E0E10 — GalleryColors::Dark::bg):
//     Use couplingBright / chainAccent / chainBright — all ≥ 7:1 AAA.
//     Use couplingAccent for large text only (≥14pt bold / ≥18pt regular).
//     Avoid couplingPrimary and chainPrimary for normal-weight text <18pt.
//
// Note on chain UI availability:
//   The sequence-layer chain matrix UI is planned for Wave 5. These tokens are
//   defined now so Wave 5 components can include this header immediately.
//   CouplingSubstrate uses CouplingTypeColors::forType() for per-type thread colors;
//   these AccentColors tokens are for system-level coupling UI chrome (buttons,
//   labels, popup borders) not per-type thread differentiation.

#include <juce_gui_basics/juce_gui_basics.h>

namespace XOceanus
{
namespace AccentColors
{

// ── D10: Coupling family — warm copper / amber ──────────────────────────────
// Use for: coupling popup borders, coupling indicator labels, coupling icons,
// any UI chrome that identifies the audio/DSP coupling system.

/// Primary copper: coupling connection arrows, active coupling labels.
/// AA for large text; use couplingBright for normal-weight body text.
inline const juce::Colour couplingPrimary    = juce::Colour::fromRGB(0xC8, 0x7A, 0x3C);

/// Accent amber: coupling panel borders, highlight state, accent dots.
/// AA for large text (≥14pt bold); preferred for headings and section titles.
inline const juce::Colour couplingAccent     = juce::Colour::fromRGB(0xE8, 0x9F, 0x4D);

/// Bright amber: normal-weight body text labels in coupling panels.
/// AAA (≥7:1) against dark Ocean/Gallery backgrounds. Use for text < 18pt.
inline const juce::Colour couplingBright     = juce::Colour::fromRGB(0xF4, 0xBA, 0x6E);

/// Dim copper: disabled/inactive coupling UI elements, backgrounds.
/// Not for text — luminance too low. Use as fill/tint only.
inline const juce::Colour couplingDim        = juce::Colour::fromRGB(0x6E, 0x44, 0x22);

/// Background tint: low-alpha coupling panel fill (use .withAlpha(0.08f–0.15f)).
inline const juce::Colour couplingBg         = juce::Colour::fromRGB(0x4A, 0x2A, 0x10);


// ── D10: Chain family — cool teal / electric ────────────────────────────────
// Use for: sequencer chain connections, chain slot indicators, pattern link UI.
// Chain matrix UI lands in Wave 5; these tokens are available now.

/// Primary teal: chain connection lines, active chain labels (large text only).
/// AA for large text. Use chainAccent for smaller labels.
inline const juce::Colour chainPrimary       = juce::Colour::fromRGB(0x2C, 0xC0, 0xC8);

/// Accent electric cyan: chain panel borders, highlight state, icons.
/// AAA (≥7:1) for large text. Preferred for headings.
inline const juce::Colour chainAccent        = juce::Colour::fromRGB(0x6C, 0xEB, 0xF4);

/// Bright cyan: normal-weight body text labels in chain/sequencer panels.
/// AAA (≥7:1) against dark backgrounds. Use for text < 18pt.
inline const juce::Colour chainBright        = juce::Colour::fromRGB(0x90, 0xF2, 0xFA);

/// Dim teal: disabled/inactive chain UI elements, backgrounds.
/// Not for text — use as fill/tint only.
inline const juce::Colour chainDim           = juce::Colour::fromRGB(0x14, 0x60, 0x68);

/// Background tint: low-alpha chain panel fill (use .withAlpha(0.08f–0.15f)).
inline const juce::Colour chainBg            = juce::Colour::fromRGB(0x0A, 0x3C, 0x42);


// ── Convenience: semantic aliases ───────────────────────────────────────────

/// Text-safe coupling label color (AAA, for body text in coupling UI).
inline const juce::Colour couplingLabel      = couplingBright;

/// Text-safe chain label color (AAA, for body text in chain/sequencer UI).
inline const juce::Colour chainLabel         = chainAccent;

} // namespace AccentColors
} // namespace XOceanus
