// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// GalleryColors.h — Gallery Model color, font, and accessibility utilities.
//
// Extracted from XOceanusEditor.h so that UI sub-components (CouplingVisualizer,
// ExportDialog, PresetBrowser …) can include this header directly rather than
// the full editor header, which would create circular include chains.
//
// Provides: GalleryColors, GalleryFonts, A11y namespaces.
// Requires juce_gui_basics + BinaryData (embedded fonts) + EngineVocabulary.

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include <unordered_map>           // per-instance dark mode registry (fix #329)
#include "BinaryData.h"            // FontData:: namespace (embedded fonts via juce_add_binary_data)
#include "../Core/PresetManager.h" // frozenPrefixForEngine()

#if JUCE_MAC
#include <CoreFoundation/CoreFoundation.h> // CFPreferencesGetAppBooleanValue for A11y::prefersReducedMotion()
#endif

#if JUCE_IOS
// Forward declaration of the Objective-C++ bridge for UIAccessibility.isReduceMotionEnabled.
// Definition lives in HapticEngine_iOS.mm, compiled only on iOS.
namespace xoceanus::a11y_platform
{
bool isReduceMotionEnabled();
}
#endif

namespace xoceanus
{
namespace GalleryColors
{

// Theme state — dark by default (dark mode is the primary presentation).
//
// Fix #329: the previous implementation used a function-local static `bool`
// which was shared across all plugin instances in the same DAW process.
// This has been replaced with a per-instance registry:
//
//   - XOceanusEditor registers itself with setInstanceDarkMode(editorPtr, val)
//     in its constructor (reads PropertiesFile for the saved preference) and
//     calls unregisterInstance(editorPtr) in its destructor.
//   - Before painting, each editor calls setActiveDarkModeContext(editorPtr)
//     so that darkMode() returns the value for the currently-rendering instance.
//   - Components that need to write the preference (SettingsPanel) call
//     setInstanceDarkMode(getTopLevelComponent(), newVal).
//
// Because all JUCE paint calls run on the message thread, and a given thread
// can only be painting one editor at a time, the active-context pointer is
// message-thread-local in practice.
//
// The free function darkMode() is preserved for source compatibility.

// Per-instance registry: editor pointer → dark mode bool
inline std::unordered_map<void*, bool>& instanceRegistry()
{
    static std::unordered_map<void*, bool> reg;
    return reg;
}

// The editor pointer that is currently active on the message thread (paint context).
inline void*& activeEditorContext()
{
    static void* ptr = nullptr;
    return ptr;
}

// Called by XOceanusEditor constructor / setDarkMode() and SettingsPanel.
inline void setInstanceDarkMode(void* editorPtr, bool value)
{
    instanceRegistry()[editorPtr] = value;
    // If this editor is currently active, propagate immediately to darkMode()
    if (activeEditorContext() == editorPtr || activeEditorContext() == nullptr)
        activeEditorContext() = editorPtr;
}

// Called by XOceanusEditor destructor.
inline void unregisterInstance(void* editorPtr)
{
    instanceRegistry().erase(editorPtr);
    if (activeEditorContext() == editorPtr)
        activeEditorContext() = nullptr;
}

// Called by XOceanusEditor::paint() (or resized/visibilityChanged) to set
// the per-instance context before any child component queries darkMode().
inline void setActiveDarkModeContext(void* editorPtr)
{
    activeEditorContext() = editorPtr;
}

// The single free-function entry point used by all UI components.
// Returns the dark mode value for the currently-active editor instance,
// falling back to true (dark is primary) when no context is registered.
inline bool& darkMode()
{
    void* ctx = activeEditorContext();
    if (ctx != nullptr)
    {
        auto& reg = instanceRegistry();
        auto it = reg.find(ctx);
        if (it != reg.end())
            return it->second;
    }
    // Fallback: return a stable reference to a default-true static.
    // This path is taken during editor construction before the first
    // setActiveDarkModeContext() call, and in non-editor contexts
    // (e.g. export pipeline preview). Dark mode is the primary presentation.
    static bool fallbackDark = true;
    return fallbackDark;
}

// Light palette
namespace Light
{
constexpr uint32_t shellWhite = 0xFFF8F6F3;
constexpr uint32_t textDark = 0xFF1A1A1A;
constexpr uint32_t textMid = 0xFF777570;
constexpr uint32_t borderGray = 0xFFDDDAD5;
constexpr uint32_t slotBg = 0xFFFCFBF9;
constexpr uint32_t emptySlot = 0xFFEAE8E4;
constexpr uint32_t xoGoldText = 0xFF9E7C2E; // WCAG AA on shellWhite
} // namespace Light

// Dark palette (matched to xoceanus-v05-accurate.html CSS variables exactly)
namespace Dark
{
// Core backgrounds
static constexpr uint32_t bg = 0xFF0E0E10;       // --bg (plugin shell)
static constexpr uint32_t surface = 0xFF1A1A1C;  // --surface (header, status bar)
static constexpr uint32_t elevated = 0xFF242426; // --elevated (cards, pills)
static constexpr uint32_t raised = 0xFF2C2C2F;   // --raised (tooltips)

// Text hierarchy (4-level tonal scale)
static constexpr uint32_t t1 = 0xFFF0EDE8; // primary text (headings, active)
static constexpr uint32_t t2 = 0xFF9E9B97; // secondary (labels)
static constexpr uint32_t t3 = 0xFF5E5C5A; // tertiary (disabled, muted) — matches prototype --t3
static constexpr uint32_t t4 = 0xFF3A3938; // quaternary (very subtle)

// Legacy accessor-mapped values
static constexpr uint32_t shellWhite = bg;   // shell background
static constexpr uint32_t textDark = t1;     // primary text
static constexpr uint32_t textMid = t2;      // secondary text
static constexpr uint32_t borderGray = t4;   // borders (use border() for alpha version)
static constexpr uint32_t slotBg = elevated; // card / slot backgrounds
static constexpr uint32_t emptySlot = t4;    // empty slot indicator
static constexpr uint32_t xoGoldText = 0xFFE9C46A;
} // namespace Dark

// Ocean palette — radial depth view (Ocean View redesign, 2026-04-06)
// These tokens replace the Gallery dark palette when the Ocean View is active.
// The 8 depth-graduated backgrounds move from neutral gray to blue-tinted depths.
namespace Ocean
{
// Depth backgrounds — from deepest edge to brightest center
static constexpr uint32_t abyss    = 0xFF04040A; // deepest background, plugin edges
static constexpr uint32_t deep     = 0xFF0A0E18; // radial view outer edge
static constexpr uint32_t twilight = 0xFF0E1428; // mid-depth surfaces
static constexpr uint32_t shallow  = 0xFF142040; // near-center surfaces, overlays
static constexpr uint32_t surface  = 0xFF1A2848; // cards, panels, elevated content
static constexpr uint32_t foam     = 0xFFE8E4DF; // primary text (warm white)
static constexpr uint32_t salt     = 0xFF9E9B97; // secondary text
static constexpr uint32_t plankton = 0xFF5E6878; // tertiary text, disabled

// Depth zone tint colours — applied at low alpha over the radial gradient
static constexpr uint32_t sunlitTint   = 0xFF48CAE4; // warm cyan (Sunlit zone, 4% alpha)
static constexpr uint32_t twilightTint = 0xFF0096C7; // blue (Twilight zone, 3% alpha)
static constexpr uint32_t midnightTint = 0xFF7B2FBE; // violet (Midnight zone, 4% alpha)

// Ambient edge glow defaults
static constexpr uint32_t edgeCyan   = 0xFF48CAE4;
static constexpr uint32_t edgeViolet = 0xFF7B2FBE;
} // namespace Ocean

// Brand constants — unchanged between modes
constexpr uint32_t xoGold = 0xFFE9C46A;

// Theme-aware accessors
inline uint32_t shellWhite()
{
    return darkMode() ? Dark::shellWhite : Light::shellWhite;
}
inline uint32_t textDark()
{
    return darkMode() ? Dark::textDark : Light::textDark;
}
inline uint32_t textMid()
{
    return darkMode() ? Dark::textMid : Light::textMid;
}
inline uint32_t borderGray()
{
    return darkMode() ? Dark::borderGray : Light::borderGray;
}
inline uint32_t slotBg()
{
    return darkMode() ? Dark::slotBg : Light::slotBg;
}
inline uint32_t emptySlot()
{
    return darkMode() ? Dark::emptySlot : Light::emptySlot;
}
inline uint32_t xoGoldText()
{
    return darkMode() ? Dark::xoGoldText : Light::xoGoldText;
}

// New tonal accessors (prototype v05)
inline uint32_t surface()
{
    return darkMode() ? Dark::surface : Light::shellWhite;
}
inline uint32_t elevated()
{
    return darkMode() ? Dark::elevated : 0xFFEEEBE6;
}
inline uint32_t raised()
{
    return darkMode() ? Dark::raised : 0xFFE4E1DC;
}
inline uint32_t t1()
{
    return darkMode() ? Dark::t1 : Light::textDark;
}
inline uint32_t t2()
{
    return darkMode() ? Dark::t2 : Light::textMid;
}
inline uint32_t t3()
{
    return darkMode() ? Dark::t3 : 0xFF888580;
}
inline uint32_t t4()
{
    return darkMode() ? Dark::t4 : Light::borderGray;
}

// Prototype-spec border helpers (alpha-over-background blends)
inline juce::Colour border()
{
    return darkMode() ? juce::Colour(0xFFFFFFFF).withAlpha(0.07f) : juce::Colour(0xFF000000).withAlpha(0.12f);
}
inline juce::Colour borderMd()
{
    return darkMode() ? juce::Colour(0xFFFFFFFF).withAlpha(0.11f) : juce::Colour(0xFF000000).withAlpha(0.20f);
}

// Backward compatibility constants
constexpr uint32_t shellWhite_v = 0xFFF8F6F3;
constexpr uint32_t textDark_v = 0xFF1A1A1A;
constexpr uint32_t textMid_v = 0xFF777570;
constexpr uint32_t borderGray_v = 0xFFDDDAD5;
constexpr uint32_t slotBg_v = 0xFFFCFBF9;
constexpr uint32_t emptySlot_v = 0xFFEAE8E4;

inline juce::Colour get(uint32_t hex)
{
    return juce::Colour(hex);
}

inline juce::Colour goldDim()
{
    return juce::Colour(get(xoGold)).withAlpha(0.14f);
}
inline juce::Colour goldGlow()
{
    return juce::Colour(get(xoGold)).withAlpha(0.28f);
}

/** Ensures minimum WCAG AA contrast on dark backgrounds.
        Preserves hue/saturation but lifts luminance if contrast < 4.5:1 on Dark::bg.
        4.5:1 meets WCAG AA for normal text (under 18pt / 14pt bold) — #211.
        Pass minRatio = 3.0f explicitly for large-text-only contexts. */
static inline juce::Colour ensureMinContrast(juce::Colour c, float minRatio = 4.5f) noexcept
{
    // Relative luminance of Dark::bg (#0E0E10) ≈ 0.0046
    constexpr float bgLum = 0.0046f;
    float lum = 0.2126f * std::pow(c.getFloatRed(), 2.2f) + 0.7152f * std::pow(c.getFloatGreen(), 2.2f) +
                0.0722f * std::pow(c.getFloatBlue(), 2.2f);
    float ratio = (std::max(lum, bgLum) + 0.05f) / (std::min(lum, bgLum) + 0.05f);
    if (ratio >= minRatio)
        return c;
    // Target luminance for minRatio:1 against bgLum
    float targetLum = minRatio * (bgLum + 0.05f) - 0.05f;
    // Lift via HSB brightness
    float h, s, b;
    c.getHSB(h, s, b);
    // Binary search for brightness that achieves target luminance
    float lo = b, hi = 1.0f;
    for (int i = 0; i < 12; ++i)
    {
        float mid = (lo + hi) * 0.5f;
        auto test = juce::Colour::fromHSV(h, s, mid, 1.0f);
        float tl = 0.2126f * std::pow(test.getFloatRed(), 2.2f) + 0.7152f * std::pow(test.getFloatGreen(), 2.2f) +
                   0.0722f * std::pow(test.getFloatBlue(), 2.2f);
        if (tl < targetLum)
            lo = mid;
        else
            hi = mid;
    }
    return juce::Colour::fromHSV(h, s, (lo + hi) * 0.5f, c.getFloatAlpha());
}

inline juce::Colour accentForEngine(const juce::String& id)
{
    juce::Colour result = get(t2());
    if (id == "OddfeliX")
        result = juce::Colour(0xFF00A6D6);
    else if (id == "OddOscar")
        result = juce::Colour(0xFFE8839B);
    else if (id == "Overdub")
        result = juce::Colour(0xFF6B7B3A);
    else if (id == "Odyssey")
        result = juce::Colour(0xFF7B2D8B);
    else if (id == "Oblong")
        result = juce::Colour(0xFFE9A84A);
    else if (id == "Obese")
        result = juce::Colour(0xFFFF1493);
    else if (id == "Obiont")
        result = juce::Colour(0xFFE8A030);
    else if (id == "Obelisk")
        result = juce::Colour(0xFFFFFFF0);
    else if (id == "Obrix")
        result = juce::Colour(0xFF1E8B7E);
    else if (id == "Obscura")
        result = juce::Colour(0xFF8A9BA8);
    else if (id == "Obsidian")
        result = juce::Colour(0xFFE8E0D8);
    else if (id == "Obbligato")
        result = juce::Colour(0xFFFF8A7A);
    else if (id == "OceanDeep")
        result = juce::Colour(0xFF2D0A4E);
    else if (id == "Oceanic")
        result = juce::Colour(0xFF00B4A0);
    else if (id == "Ocelot")
        result = juce::Colour(0xFFC5832B);
    else if (id == "Ochre")
        result = juce::Colour(0xFFCC7722);
    else if (id == "Octave")
        result = juce::Colour(0xFF8B6914);
    else if (id == "Octopus")
        result = juce::Colour(0xFFE040FB);
    else if (id == "Oddfellow")
        result = juce::Colour(0xFFB87333);
    else if (id == "Offering")
        result = juce::Colour(0xFFE5B80B);
    // Near-black engines: return pre-chosen accessible replacements directly.
    // ensureMinContrast() would lift these to indistinguishable medium gray
    // because their saturation is near zero.  The replacements share the
    // original engine concept (Steinway warmth / sub-bass depth) while
    // providing ≥4.5:1 contrast and visually distinct identities (#185).
    else if (id == "Ogre")
        return juce::Colour(0xFF7878A0); // Sub Bass Slate — cool blue-violet, ≥4.5:1 on Dark::bg
    else if (id == "Okeanos")
        result = juce::Colour(0xFFC49B3F);
    else if (id == "Ohm")
        result = juce::Colour(0xFF87AE73);
    else if (id == "Oaken")
        result = juce::Colour(0xFF9C6B30);
    else if (id == "Oasis")
        result = juce::Colour(0xFF00827F);
    else if (id == "Oblique")
        result = juce::Colour(0xFFBF40FF);
    else if (id == "Olate")
        result = juce::Colour(0xFF5C3317);
    else if (id == "Ole")
        result = juce::Colour(0xFFC9377A);
    else if (id == "Oleg")
        result = juce::Colour(0xFFC0392B);
    else if (id == "Ombre")
        result = juce::Colour(0xFF7B6B8A);
    else if (id == "Omega")
        result = juce::Colour(0xFF003366);
    else if (id == "Onkolo")
        result = juce::Colour(0xFFFFBF00);
    else if (id == "Onset")
        result = juce::Colour(0xFF0066FF);
    else if (id == "Opal")
        result = juce::Colour(0xFFA78BFA);
    else if (id == "Opaline")
        result = juce::Colour(0xFFB7410E);
    else if (id == "Opcode")
        result = juce::Colour(0xFF5F9EA0); // CadetBlue
    else if (id == "OpenSky")
        result = juce::Colour(0xFFFF8C00);
    else if (id == "Opera")
        result = juce::Colour(0xFFD4AF37);
    else if (id == "Optic")
        result = juce::Colour(0xFF00FF41);
    else if (id == "Oracle")
        result = juce::Colour(0xFF4B0082);
    else if (id == "Orbweave")
        result = juce::Colour(0xFF8E4585);
    else if (id == "Orbital")
        result = juce::Colour(0xFFFF6B6B);
    else if (id == "Orca")
        result = juce::Colour(0xFF1B2838);
    else if (id == "Orchard")
        result = juce::Colour(0xFFFFB7C5);
    else if (id == "Organon")
        result = juce::Colour(0xFF00CED1);
    else if (id == "Organism")
        result = juce::Colour(0xFFC6E377);
    else if (id == "Origami")
        result = juce::Colour(0xFFE63946);
    else if (id == "Orphica")
        result = juce::Colour(0xFF7FDBCA);
    else if (id == "Osier")
        result = juce::Colour(0xFFC0C8C8);
    else if (id == "Osmosis")
        result = juce::Colour(0xFFC0C0C0);
    else if (id == "Osprey")
        result = juce::Colour(0xFF1B4F8A);
    else if (id == "Osteria")
        result = juce::Colour(0xFF722F37);
    else if (id == "Ostinato")
        result = juce::Colour(0xFFE8701A);
    else if (id == "Otis")
        result = juce::Colour(0xFFD4A017);
    else if (id == "Ottoni")
        result = juce::Colour(0xFF5B8A72);
    else if (id == "Oto")
        result = juce::Colour(0xFFF5F0E8);
    else if (id == "Outflow")
        result = juce::Colour(0xFF1A1A40);
    else if (id == "Ouie")
        result = juce::Colour(0xFF708090);
    else if (id == "Ouroboros")
        result = juce::Colour(0xFFFF2D2D);
    else if (id == "Outwit")
        result = juce::Colour(0xFFCC6600);
    else if (id == "Outlook")
        result = juce::Colour(0xFF4169E1);
    else if (id == "Oven")
        return juce::Colour(0xFF908070); // Steinway Ebony Warm — warm ivory charcoal, ≥4.5:1 on Dark::bg
    else if (id == "Overbite")
        result = juce::Colour(0xFFF0EDE8); // Fang White
    else if (id == "Overcast")
        result = juce::Colour(0xFF778899);
    else if (id == "Overflow")
        result = juce::Colour(0xFF1A3A5C);
    else if (id == "Overgrow")
        result = juce::Colour(0xFF228B22);
    else if (id == "Overlap")
        result = juce::Colour(0xFF00FFB4);
    else if (id == "Overtone")
        result = juce::Colour(0xFFA8D8EA);
    else if (id == "Overworld")
        result = juce::Colour(0xFF39FF14);
    else if (id == "Overwash")
        result = juce::Colour(0xFFF0F8FF);
    else if (id == "Overworn")
        result = juce::Colour(0xFF808080);
    else if (id == "Oware")
        result = juce::Colour(0xFFB5883E);
    else if (id == "Owlfish")
        result = juce::Colour(0xFFB8860B);
    else if (id == "Oxalis")
        result = juce::Colour(0xFF9B59B6);
    else if (id == "Oxbow")
        result = juce::Colour(0xFF1A6B5A);
    else if (id == "Oxytocin")
        result = juce::Colour(0xFF9B5DE5);
    return ensureMinContrast(result);
}

inline juce::String prefixForEngine(const juce::String& id)
{
    return frozenPrefixForEngine(id);
}

} // namespace GalleryColors

//==============================================================================
// GalleryFonts — centralized typography (Space Grotesk, Inter, JetBrains Mono)
namespace GalleryFonts
{

inline juce::Typeface::Ptr loadTypeface(const char* data, int size)
{
    return juce::Typeface::createSystemTypefaceFor(data, static_cast<size_t>(size));
}

inline juce::Typeface::Ptr spaceGroteskBold()
{
    static auto tf = loadTypeface(FontData::SpaceGroteskBold_otf, FontData::SpaceGroteskBold_otfSize);
    return tf;
}
inline juce::Typeface::Ptr spaceGroteskMedium()
{
    static auto tf = loadTypeface(FontData::SpaceGroteskMedium_otf, FontData::SpaceGroteskMedium_otfSize);
    return tf;
}
inline juce::Typeface::Ptr interRegular()
{
    static auto tf = loadTypeface(FontData::InterRegular_ttf, FontData::InterRegular_ttfSize);
    return tf;
}
inline juce::Typeface::Ptr interMedium()
{
    static auto tf = loadTypeface(FontData::InterMedium_ttf, FontData::InterMedium_ttfSize);
    return tf;
}
inline juce::Typeface::Ptr interBold()
{
    static auto tf = loadTypeface(FontData::InterBold_ttf, FontData::InterBold_ttfSize);
    return tf;
}
inline juce::Typeface::Ptr jetBrainsMono()
{
    static auto tf = loadTypeface(FontData::JetBrainsMonoRegular_ttf, FontData::JetBrainsMonoRegular_ttfSize);
    return tf;
}
inline juce::Typeface::Ptr overbitRegular()
{
    static auto tf = loadTypeface(FontData::OverbitRegular_otf, FontData::OverbitRegular_otfSize);
    return tf;
}
inline juce::Typeface::Ptr dotoRegular()
{
    static auto tf = loadTypeface(FontData::DotoRegular_ttf, FontData::DotoRegular_ttfSize);
    return tf;
}
inline juce::Typeface::Ptr dotoBold()
{
    static auto tf = loadTypeface(FontData::DotoBold_ttf, FontData::DotoBold_ttfSize);
    return tf;
}

inline juce::Font display(float size)
{
    return juce::Font(juce::FontOptions{}.withTypeface(spaceGroteskBold()).withHeight(size));
}
inline juce::Font heading(float size)
{
    return juce::Font(juce::FontOptions{}.withTypeface(interBold()).withHeight(size));
}
inline juce::Font body(float size)
{
    return juce::Font(juce::FontOptions{}.withTypeface(interRegular()).withHeight(size));
}
inline juce::Font label(float size)
{
    return juce::Font(juce::FontOptions{}.withTypeface(interMedium()).withHeight(size));
}
inline juce::Font value(float size)
{
    return juce::Font(juce::FontOptions{}.withTypeface(jetBrainsMono()).withHeight(size));
}
// Doto — dot-matrix display font (submarine instrumentation aesthetic).
// Used for: preset display, visualizer labels, BPM readout, transport values.
inline juce::Font dotMatrix(float size)
{
    return juce::Font(juce::FontOptions{}.withTypeface(dotoBold()).withHeight(size));
}
inline juce::Font dotMatrixLight(float size)
{
    return juce::Font(juce::FontOptions{}.withTypeface(dotoRegular()).withHeight(size));
}
// ── Mood fonts — unique typeface per preset mood category ──────────────────
// Each mood gets its own display font for preset names and mood labels.
// When a mood font is not yet embedded, falls back to Inter Bold.
enum class MoodType
{
    Foundation, Atmosphere, Entangled, Prism, Flux, Aether,
    Family, Submerged, Coupling, Crystalline, Deep, Ethereal,
    Kinetic, Luminous, Organic, Shadow
};

inline juce::Typeface::Ptr moodTypeface(MoodType mood)
{
    switch (mood)
    {
        case MoodType::Foundation:  { static auto tf = loadTypeface(FontData::FjordRegular_otf,       FontData::FjordRegular_otfSize);       return tf; }
        case MoodType::Deep:        { static auto tf = loadTypeface(FontData::BionixFat_otf,          FontData::BionixFat_otfSize);          return tf; }
        case MoodType::Crystalline: { static auto tf = loadTypeface(FontData::Norqen_otf,             FontData::Norqen_otfSize);             return tf; }
        case MoodType::Luminous:    { static auto tf = loadTypeface(FontData::MaleahRegular_otf,      FontData::MaleahRegular_otfSize);      return tf; }
        case MoodType::Atmosphere:  { static auto tf = loadTypeface(FontData::SaesonBold_ttf,         FontData::SaesonBold_ttfSize);         return tf; }
        case MoodType::Kinetic:     { static auto tf = loadTypeface(FontData::NebulicaRegular_ttf,    FontData::NebulicaRegular_ttfSize);    return tf; }
        case MoodType::Entangled:   { static auto tf = loadTypeface(FontData::RoyalOcean_otf,         FontData::RoyalOcean_otfSize);         return tf; }
        case MoodType::Shadow:      { static auto tf = loadTypeface(FontData::ForgeSans_otf,          FontData::ForgeSans_otfSize);          return tf; }
        case MoodType::Organic:     { static auto tf = loadTypeface(FontData::OtfitsGroteskBold_otf,  FontData::OtfitsGroteskBold_otfSize);  return tf; }
        case MoodType::Prism:       { static auto tf = loadTypeface(FontData::LokanovaProBold_otf,    FontData::LokanovaProBold_otfSize);    return tf; }
        case MoodType::Flux:        { static auto tf = loadTypeface(FontData::NeoformDisplay_otf,     FontData::NeoformDisplay_otfSize);     return tf; }
        case MoodType::Aether:      { static auto tf = loadTypeface(FontData::Apestron_otf,           FontData::Apestron_otfSize);           return tf; }
        case MoodType::Submerged:   { static auto tf = loadTypeface(FontData::CostalineRegular_otf,   FontData::CostalineRegular_otfSize);   return tf; }
        case MoodType::Coupling:    { static auto tf = loadTypeface(FontData::NeuticalRegular_otf,    FontData::NeuticalRegular_otfSize);    return tf; }
        case MoodType::Family:      { static auto tf = loadTypeface(FontData::ZTOtezRegular_otf,      FontData::ZTOtezRegular_otfSize);      return tf; }
        case MoodType::Ethereal:    { static auto tf = loadTypeface(FontData::Elijah_otf,             FontData::Elijah_otfSize);             return tf; }
        default:                    return interBold();
    }
}

inline juce::Font moodFont(MoodType mood, float size)
{
    return juce::Font(juce::FontOptions{}.withTypeface(moodTypeface(mood)).withHeight(size));
}

/// Resolve a mood name string (from .xometa) to a MoodType enum.
inline MoodType moodFromString(const juce::String& name)
{
    if (name == "Foundation")  return MoodType::Foundation;
    if (name == "Atmosphere")  return MoodType::Atmosphere;
    if (name == "Entangled")   return MoodType::Entangled;
    if (name == "Prism")       return MoodType::Prism;
    if (name == "Flux")        return MoodType::Flux;
    if (name == "Aether")      return MoodType::Aether;
    if (name == "Family")      return MoodType::Family;
    if (name == "Submerged")   return MoodType::Submerged;
    if (name == "Coupling")    return MoodType::Coupling;
    if (name == "Crystalline") return MoodType::Crystalline;
    if (name == "Deep")        return MoodType::Deep;
    if (name == "Ethereal")    return MoodType::Ethereal;
    if (name == "Kinetic")     return MoodType::Kinetic;
    if (name == "Luminous")    return MoodType::Luminous;
    if (name == "Organic")     return MoodType::Organic;
    if (name == "Shadow")      return MoodType::Shadow;
    return MoodType::Foundation; // fallback
}

// Overbit — engine nameplate display font (accent-colored, Column B hero text).
// Decision D2: Overbit at ≥12px; Space Grotesk Bold fallback below 12px.
//
// NOTE (Windows ClearType): Overbit at 12–14px requires ClearType verification
// before shipping on Windows — hinting may differ from macOS CoreText rendering.
// Verify on a ClearType-enabled Windows display before V1 release.
inline juce::Font engineName(float size)
{
    if (size >= 12.0f)
        return juce::Font(juce::FontOptions{}.withTypeface(overbitRegular()).withHeight(size));
    return display(size); // Space Grotesk Bold fallback below 12px threshold
}

} // namespace GalleryFonts

//==============================================================================
// A11y — WCAG 2.1 AA accessibility utilities
namespace A11y
{

inline juce::Colour focusRingColour()
{
    return GalleryColors::darkMode() ? juce::Colour(0xFF58A6FF) : juce::Colour(0xFF0066CC);
}

inline void drawFocusRing(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius = 4.0f)
{
    g.setColour(focusRingColour());
    g.drawRoundedRectangle(bounds.reduced(1.0f), cornerRadius, 2.0f);
}

inline void drawCircularFocusRing(juce::Graphics& g, float cx, float cy, float radius)
{
    g.setColour(focusRingColour());
    g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 2.0f);
}

inline void setup(juce::Component& comp, const juce::String& title, const juce::String& description = {},
                  bool wantsKeyFocus = true)
{
    comp.setTitle(title);
    if (description.isNotEmpty())
        comp.setDescription(description);
    comp.setWantsKeyboardFocus(wantsKeyFocus);
}

inline bool meetsMinTargetSize(const juce::Component& comp, bool mobile = false)
{
    int minSize = mobile ? 44 : 24;
    return comp.getWidth() >= minSize && comp.getHeight() >= minSize;
}

// In-app Reduce Motion override — set by SettingsPanel toggle at runtime.
// When true, prefersReducedMotion() returns true regardless of the OS setting.
// Stored as a static atomic so it is safe to read from any thread (paint callbacks,
// timer callbacks, etc.) without locking. Default false — OS preference governs.
inline std::atomic<bool>& inAppReducedMotion()
{
    static std::atomic<bool> flag{false};
    return flag;
}

// setReducedMotion() — call from the SettingsPanel toggle callback on the message thread.
inline void setReducedMotion(bool v)
{
    inAppReducedMotion().store(v, std::memory_order_relaxed);
}

inline bool prefersReducedMotion()
{
    // In-app override takes priority — toggled at runtime by SettingsPanel.
    if (inAppReducedMotion().load(std::memory_order_relaxed))
        return true;

#if JUCE_MAC
    // Read the "Reduce Motion" accessibility preference via CoreFoundation.
    // CFPreferencesGetAppBooleanValue is a pure-C API callable from C++ without ObjC.
    // Key:    "reduceMotion"
    // Domain: "com.apple.universalaccess"
    // This is the same preference that NSWorkspace.accessibilityDisplayShouldReduceMotion
    // reads, but accessible without an ObjC runtime call.
    // CoreFoundation.h is included at the top of this file under #if JUCE_MAC.
    Boolean keyExists = false;
    Boolean val =
        CFPreferencesGetAppBooleanValue(CFSTR("reduceMotion"), CFSTR("com.apple.universalaccess"), &keyExists);
    return keyExists && static_cast<bool>(val);
#elif JUCE_IOS
    // UIAccessibility.isReduceMotionEnabled — implemented in HapticEngine_iOS.mm
    // via xoceanus::a11y_platform::isReduceMotionEnabled() bridge function.
    // The definition is in that .mm file compiled only on iOS.
    return xoceanus::a11y_platform::isReduceMotionEnabled();
#else
    return false;
#endif
}

} // namespace A11y

//==============================================================================
// GalleryUtils — shared text / layout helpers
namespace GalleryUtils
{

/** Truncate text with a Unicode ellipsis ("…") so it fits within maxWidth at
    the given font.  Returns the original string unchanged when it already fits,
    or the longest prefix + "…" that fits, or just "…" when nothing else does.

    Usage:
        auto display = GalleryUtils::ellipsizeText(g.getCurrentFont(), longText, (float)(w - 60));
        g.drawText(display, x, y, w - 60, h, juce::Justification::centredLeft, false);
*/
static inline juce::String ellipsizeText(const juce::Font& font, const juce::String& text, float maxWidth)
{
    if (font.getStringWidthFloat(text) <= maxWidth)
        return text;

    const juce::String ellipsis = juce::String::charToString(0x2026); // "…"

    for (int len = text.length() - 1; len > 0; --len)
    {
        juce::String candidate = text.substring(0, len) + ellipsis;
        if (font.getStringWidthFloat(candidate) <= maxWidth)
            return candidate;
    }
    return ellipsis; // nothing fits except the ellipsis itself
}

} // namespace GalleryUtils

} // namespace xoceanus
