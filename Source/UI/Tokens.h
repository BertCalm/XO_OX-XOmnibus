// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

// XOceanus design token surface — Track B canonical reference.
// Single source of truth for colors, typography, and animation grammar.
// Forwards to existing GalleryColors / GalleryFonts / AccentColors where appropriate.
//
// Locked decisions (Session 2A, 2026-05-02):
//   D1 — Pedal-board direction: submarine instrument console (Session 2B implements)
//          riveted depth-bar, brass-rim porthole rotaries, dot-matrix labels via Doto font.
//   D5 — Cursor identity: depth-ring custom cursor (Session 2C implements; asset TBD)
//          master plan item #17.
//
// Namespace casing notes (verified 2026-05-02):
//   GalleryColors / GalleryFonts  →  xoceanus::GalleryColors / xoceanus::GalleryFonts  (lowercase)
//   AccentColors                  →  XOceanus::AccentColors                             (mixed case)
//   GalleryFonts has: display(), heading(), body(), value() (JetBrains Mono), label()
//   There is no GalleryFonts::mono() — use value() for mono/JetBrains Mono output.

#include <juce_graphics/juce_graphics.h>
#include "GalleryColors.h"
#include "AccentColors.h"

namespace XO::Tokens {

// ── D2: 8-token semantic color palette ──────────────────────────────────────
// These tokens are the canonical replacement for raw colour literals scattered
// across the codebase. Sweeps to replace call-sites land in Session 2C+.
namespace Color {

    // Raw ARGB hex constants — use when a constexpr uint32 is required.
    constexpr juce::uint32 Primary       = 0xFFE9C46A; // XO Gold — brand-warm accent
    constexpr juce::uint32 Surface       = 0xFF1A1F2A; // Ocean::deep — default panel background
    constexpr juce::uint32 SurfaceMuted  = 0xFF101620; // Ocean::abyss — modal/drawer/scrim
    constexpr juce::uint32 Accent        = 0xFF3CB4BE; // Ocean::foam-adjacent — replaces bare Colour(60,180,170)
    constexpr juce::uint32 AccentBright  = 0xFF48CAE4; // hover/active states
    constexpr juce::uint32 Success       = 0xFF7DD3A0; // confirmation green
    constexpr juce::uint32 Warning       = 0xFFE89B4A; // warm copper (couplingPrimary territory)
    constexpr juce::uint32 Text          = 0xFFE6E8EC; // Ocean::salt-adjacent — primary text

    // D2.b — Glow alias: Accent is the canonical name for teal in the palette.
    // Glow is an alias for use at halo/glow effect sites (8+ sites that were
    // using bare Colour(60,180,170) for glowing UI elements).
    // All sites should use Glow or Accent depending on semantic intent:
    //   Glow  → halos, radial glows, selection rings, breathing pulse effects
    //   Accent → interactive teal (borders, fills, active-state indicators)
    constexpr juce::uint32 Glow = Accent; // D2.b: alias of Accent — same teal, semantic distinction only

    // D3.c — State palette expansion (3 tokens, within the ≤11 total budget).
    // PlaySurface.h had 5 scope-local constants (kAmber/kTerracotta/kTeal/kFireGreen/kPanicRed).
    // Audit result (2026-05-02):
    //   kAmber     → maps to Warning (used for LATCH badge — non-critical indicator state)
    //   kTerracotta → UNUSED — never referenced in any paint call (dead declaration)
    //   kTeal      → UNUSED — never referenced in any paint call (dead declaration)
    //   kFireGreen → UNUSED — never referenced in any paint call (dead declaration)
    //   kPanicRed  → UNUSED — never referenced in any paint call (dead declaration)
    // Decision: only kAmber needs a canonical home → Warning covers it.
    // The 4 unused constants are deleted from PlaySurface.h in the Wave 1 sweep.
    // No new tokens needed beyond what's already in this palette. Token count stays at 9 (8 + Glow alias).

    // juce::Colour helpers — use in paint() and LookAndFeel overrides.
    inline juce::Colour primary()      { return juce::Colour(Primary); }
    inline juce::Colour surface()      { return juce::Colour(Surface); }
    inline juce::Colour surfaceMuted() { return juce::Colour(SurfaceMuted); }
    inline juce::Colour accent()       { return juce::Colour(Accent); }
    inline juce::Colour accentBright() { return juce::Colour(AccentBright); }
    inline juce::Colour glow()         { return juce::Colour(Glow); }   // D2.b alias
    inline juce::Colour success()      { return juce::Colour(Success); }
    inline juce::Colour warning()      { return juce::Colour(Warning); }
    inline juce::Colour text()         { return juce::Colour(Text); }

} // namespace Color

// ── D3: Typography tokens ────────────────────────────────────────────────────
// Forwards to xoceanus::GalleryFonts which holds the embedded-font typefaces.
//
// Role mapping (verified against GalleryFonts implementation):
//   display() → Space Grotesk Bold  (D3: Display role)
//   heading() → Satoshi Bold        (D3: Heading role)
//   body()    → Satoshi Regular     (D3: Body role)
//   mono()    → JetBrains Mono      (D3: Mono role — wraps GalleryFonts::value())
//
// NOTE: GalleryFonts has no ::mono() function. The JetBrains Mono typeface is
// exposed via ::value(). This wrapper provides the canonical mono() name so
// downstream components don't depend on the historical "value" naming.
namespace Type {

    // Font constructors — forward to xoceanus::GalleryFonts.
    inline juce::Font display(float h) { return xoceanus::GalleryFonts::display(h); }
    inline juce::Font heading(float h) { return xoceanus::GalleryFonts::heading(h); }
    inline juce::Font body   (float h) { return xoceanus::GalleryFonts::body(h); }
    inline juce::Font mono   (float h) { return xoceanus::GalleryFonts::value(h); } // JetBrains Mono

    // Canonical size constants (D3).
    constexpr float DisplayLarge  = 16.0f;
    constexpr float DisplaySmall  = 14.0f;
    constexpr float HeadingLarge  = 13.0f;
    constexpr float HeadingSmall  = 11.0f;
    constexpr float BodyLarge     = 12.0f;
    constexpr float BodyDefault   = 10.0f;
    constexpr float BodySmall     =  9.0f;
    constexpr float MonoDefault   = 11.0f;
    constexpr float MonoSmall     = 10.0f;
    constexpr float MonoTiny      =  9.0f;

} // namespace Type

// ── D4: Animation grammar ────────────────────────────────────────────────────
// Durations in milliseconds. EaseOutStep30Hz is a per-frame fractional blend
// coefficient calibrated for 30 Hz timer callbacks (matches ocean depth-ring
// animation feel). Use easeOutStep() for any lerped UI animation.
namespace Motion {

    constexpr int   DefaultDurationMs = 200;   // tab cross-fade (#18)
    constexpr int   HoverFadeMs       = 150;   // hover state fade-in (#20)
    constexpr int   RevealMs          = 320;   // modal opens, drawer slides
    constexpr int   ClickDepthMs      =  80;   // click-depth spring-back (#19)
    constexpr float EaseOutStep30Hz   = 0.18f; // lerp fraction per 33 ms tick at 30 Hz

    // Per-tick lerp factors derived from durations at 30 Hz:
    //   factor = 1 - (1 - EaseOutStep30Hz)^N  where N = durationMs / (1000/30)
    // Tab cross-fade: ~6 ticks at 200ms → per-tick step ≈ 0.40 for quick snap
    // Click depth spring-back: ~2.4 ticks at 80ms → step ≈ 0.70 (snappy)
    // Hover fade: ~4.5 ticks at 150ms → step ≈ 0.28 (smooth)
    constexpr float TabFadeStep30Hz    = 0.40f; // lerp fraction for 200ms cross-fade (#18)
    constexpr float ClickDepthStep30Hz = 0.70f; // lerp fraction for 80ms spring-back (#19)
    constexpr float HoverFadeStep30Hz  = 0.28f; // lerp fraction for 150ms hover fade (#20)

    /// Fractional-step ease-out for per-frame lerp animations.
    /// Call once per timer tick (30 Hz): value = easeOutStep(value, target);
    inline float easeOutStep(float current, float target) noexcept
    {
        return current + (target - current) * EaseOutStep30Hz;
    }

    /// Tab cross-fade variant — faster convergence for 200ms mode transitions (#18).
    inline float tabFadeStep(float current, float target) noexcept
    {
        return current + (target - current) * TabFadeStep30Hz;
    }

    /// Click-depth spring-back — fast 80ms bounce-back (#19).
    inline float clickDepthStep(float current, float target) noexcept
    {
        return current + (target - current) * ClickDepthStep30Hz;
    }

    /// Hover fade-in — smooth 150ms highlight reveal (#20).
    inline float hoverFadeStep(float current, float target) noexcept
    {
        return current + (target - current) * HoverFadeStep30Hz;
    }

    /// A11y-aware variant: skips to target immediately when OS/in-app Reduce Motion is on.
    /// Requires GalleryColors.h (included above) for A11y::prefersReducedMotion().
    inline float easeOutStepA11y(float current, float target) noexcept
    {
        if (xoceanus::A11y::prefersReducedMotion())
            return target;
        return current + (target - current) * EaseOutStep30Hz;
    }

} // namespace Motion

} // namespace XO::Tokens
