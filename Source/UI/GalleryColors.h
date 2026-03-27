#pragma once
// GalleryColors.h — Gallery Model color, font, and accessibility utilities.
//
// Extracted from XOlokunEditor.h so that UI sub-components (CouplingVisualizer,
// ExportDialog, PresetBrowser …) can include this header directly rather than
// the full editor header, which would create circular include chains.
//
// Provides: GalleryColors, GalleryFonts, A11y namespaces.
// Requires juce_gui_basics + BinaryData (embedded fonts) + EngineVocabulary.

#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"              // FontData:: namespace (embedded fonts via juce_add_binary_data)
#include "../Core/PresetManager.h"   // frozenPrefixForEngine()

#if JUCE_MAC
#include <CoreFoundation/CoreFoundation.h>  // CFPreferencesGetAppBooleanValue for A11y::prefersReducedMotion()
#endif

#if JUCE_IOS
// Forward declaration of the Objective-C++ bridge for UIAccessibility.isReduceMotionEnabled.
// Definition lives in HapticEngine_iOS.mm, compiled only on iOS.
namespace xolokun::a11y_platform {
    bool isReduceMotionEnabled();
}
#endif

namespace xolokun {
namespace GalleryColors {

    // Theme state — light by default (brand rule: light mode is primary)
    inline bool& darkMode()
    {
        static bool dark = false;
        return dark;
    }

    // Light palette
    namespace Light {
        constexpr uint32_t shellWhite = 0xFFF8F6F3;
        constexpr uint32_t textDark   = 0xFF1A1A1A;
        constexpr uint32_t textMid    = 0xFF777570;
        constexpr uint32_t borderGray = 0xFFDDDAD5;
        constexpr uint32_t slotBg     = 0xFFFCFBF9;
        constexpr uint32_t emptySlot  = 0xFFEAE8E4;
        constexpr uint32_t xoGoldText = 0xFF9E7C2E;   // WCAG AA on shellWhite
    }

    // Dark palette (matched to xolokun-v04-polished.html CSS variables exactly)
    namespace Dark {
        // Core backgrounds
        static constexpr uint32_t bg          = 0xFF0E0E10;  // --bg (plugin shell)
        static constexpr uint32_t surface     = 0xFF1A1A1C;  // --surface (header, status bar)
        static constexpr uint32_t elevated    = 0xFF242426;  // --elevated (cards, pills)
        static constexpr uint32_t raised      = 0xFF2C2C2F;  // --raised (tooltips)

        // Text hierarchy (4-level tonal scale)
        static constexpr uint32_t t1          = 0xFFF0EDE8;  // primary text (headings, active)
        static constexpr uint32_t t2          = 0xFF9E9B97;  // secondary (labels)
        static constexpr uint32_t t3          = 0xFF5E5C5A;  // tertiary (disabled, muted) — matches prototype --t3
        static constexpr uint32_t t4          = 0xFF3A3938;  // quaternary (very subtle)

        // Legacy accessor-mapped values
        static constexpr uint32_t shellWhite  = bg;           // shell background
        static constexpr uint32_t textDark    = t1;           // primary text
        static constexpr uint32_t textMid     = t2;           // secondary text
        static constexpr uint32_t borderGray  = t4;           // borders (use border() for alpha version)
        static constexpr uint32_t slotBg      = elevated;     // card / slot backgrounds
        static constexpr uint32_t emptySlot   = t4;           // empty slot indicator
        static constexpr uint32_t xoGoldText  = 0xFFE9C46A;
    }

    // Brand constants — unchanged between modes
    constexpr uint32_t xoGold     = 0xFFE9C46A;

    // Theme-aware accessors
    inline uint32_t shellWhite() { return darkMode() ? Dark::shellWhite : Light::shellWhite; }
    inline uint32_t textDark()   { return darkMode() ? Dark::textDark   : Light::textDark; }
    inline uint32_t textMid()    { return darkMode() ? Dark::textMid    : Light::textMid; }
    inline uint32_t borderGray() { return darkMode() ? Dark::borderGray : Light::borderGray; }
    inline uint32_t slotBg()     { return darkMode() ? Dark::slotBg     : Light::slotBg; }
    inline uint32_t emptySlot()  { return darkMode() ? Dark::emptySlot  : Light::emptySlot; }
    inline uint32_t xoGoldText() { return darkMode() ? Dark::xoGoldText : Light::xoGoldText; }

    // New tonal accessors (prototype v04)
    inline uint32_t surface()  { return darkMode() ? Dark::surface  : Light::shellWhite; }
    inline uint32_t elevated() { return darkMode() ? Dark::elevated : 0xFFEEEBE6; }
    inline uint32_t raised()   { return darkMode() ? Dark::raised   : 0xFFE4E1DC; }
    inline uint32_t t1()       { return darkMode() ? Dark::t1       : Light::textDark; }
    inline uint32_t t2()       { return darkMode() ? Dark::t2       : Light::textMid; }
    inline uint32_t t3()       { return darkMode() ? Dark::t3       : 0xFF888580; }
    inline uint32_t t4()       { return darkMode() ? Dark::t4       : Light::borderGray; }

    // Prototype-spec border helpers (alpha-over-background blends)
    inline juce::Colour border()   { return darkMode()
        ? juce::Colour(0xFFFFFFFF).withAlpha(0.07f)
        : juce::Colour(0xFF000000).withAlpha(0.12f); }
    inline juce::Colour borderMd() { return darkMode()
        ? juce::Colour(0xFFFFFFFF).withAlpha(0.11f)
        : juce::Colour(0xFF000000).withAlpha(0.20f); }

    // Backward compatibility constants
    constexpr uint32_t shellWhite_v = 0xFFF8F6F3;
    constexpr uint32_t textDark_v   = 0xFF1A1A1A;
    constexpr uint32_t textMid_v    = 0xFF777570;
    constexpr uint32_t borderGray_v = 0xFFDDDAD5;
    constexpr uint32_t slotBg_v     = 0xFFFCFBF9;
    constexpr uint32_t emptySlot_v  = 0xFFEAE8E4;

    inline juce::Colour get(uint32_t hex) { return juce::Colour(hex); }

    inline juce::Colour goldDim()  { return juce::Colour(get(xoGold)).withAlpha(0.14f); }
    inline juce::Colour goldGlow() { return juce::Colour(get(xoGold)).withAlpha(0.28f); }

    inline juce::Colour accentForEngine(const juce::String& id)
    {
        if (id == "OddfeliX")  return juce::Colour(0xFF00A6D6);
        if (id == "OddOscar")  return juce::Colour(0xFFE8839B);
        if (id == "Overdub")   return juce::Colour(0xFF6B7B3A);
        if (id == "Odyssey")   return juce::Colour(0xFF7B2D8B);
        if (id == "Oblong")    return juce::Colour(0xFFE9A84A);
        if (id == "Obese")     return juce::Colour(0xFFFF1493);
        if (id == "Onset")     return juce::Colour(0xFF0066FF);
        if (id == "Overworld") return juce::Colour(0xFF39FF14);
        if (id == "Opal")      return juce::Colour(0xFFA78BFA);
        if (id == "Orbital")   return juce::Colour(0xFFFF6B6B);
        if (id == "Organon")   return juce::Colour(0xFF00CED1);
        if (id == "Ouroboros") return juce::Colour(0xFFFF2D2D);
        if (id == "Obsidian")  return juce::Colour(0xFFE8E0D8);
        if (id == "Overbite")  return juce::Colour(0xFFC9B8A8);  // AgedBone
        if (id == "Origami")   return juce::Colour(0xFFE63946);
        if (id == "Oracle")    return juce::Colour(0xFF4B0082);
        if (id == "Obscura")   return juce::Colour(0xFF8A9BA8);
        if (id == "Oceanic")   return juce::Colour(0xFF00B4A0);
        if (id == "Optic")     return juce::Colour(0xFF00FF41);
        if (id == "Oblique")   return juce::Colour(0xFFBF40FF);
        if (id == "Ocelot")    return juce::Colour(0xFFC5832B);
        if (id == "Osprey")    return juce::Colour(0xFF1B4F8A);
        if (id == "Osteria")   return juce::Colour(0xFF722F37);
        if (id == "Owlfish")   return juce::Colour(0xFFB8860B);
        if (id == "Ohm")       return juce::Colour(0xFF87AE73);
        if (id == "Orphica")   return juce::Colour(0xFF7FDBCA);
        if (id == "Obbligato") return juce::Colour(0xFFFF8A7A);
        if (id == "Ottoni")    return juce::Colour(0xFF5B8A72);
        if (id == "Ole")       return juce::Colour(0xFFC9377A);
        if (id == "Overlap")   return juce::Colour(0xFF00FFB4);
        if (id == "Outwit")    return juce::Colour(0xFFCC6600);
        if (id == "OpenSky")   return juce::Colour(0xFFFF8C00);
        if (id == "Ostinato")  return juce::Colour(0xFFE8701A);
        if (id == "OceanDeep") return juce::Colour(0xFF2D0A4E);
        if (id == "Ouie")      return juce::Colour(0xFF708090);
        if (id == "Obrix")     return juce::Colour(0xFF1E8B7E);
        if (id == "Orbweave")  return juce::Colour(0xFF8E4585);
        if (id == "Overtone")  return juce::Colour(0xFFA8D8EA);
        if (id == "Organism")  return juce::Colour(0xFFC6E377);
        if (id == "Oaken")     return juce::Colour(0xFF9C6B30);
        if (id == "Oasis")     return juce::Colour(0xFF00827F);
        if (id == "Obelisk")   return juce::Colour(0xFFFFFFF0);
        if (id == "Ochre")     return juce::Colour(0xFFCC7722);
        if (id == "Octave")    return juce::Colour(0xFF8B6914);
        if (id == "Octopus")   return juce::Colour(0xFFE040FB);
        if (id == "Oddfellow") return juce::Colour(0xFFB87333);
        if (id == "Offering")  return juce::Colour(0xFFE5B80B);
        if (id == "Ogre")      return juce::Colour(0xFF0D0D0D);
        if (id == "Olate")     return juce::Colour(0xFF5C3317);
        if (id == "Oleg")      return juce::Colour(0xFFC0392B);
        if (id == "Ombre")     return juce::Colour(0xFF7B6B8A);
        if (id == "Omega")     return juce::Colour(0xFF003366);
        if (id == "Onkolo")    return juce::Colour(0xFFFFBF00);
        if (id == "Opaline")   return juce::Colour(0xFFB7410E);
        if (id == "Opcode")    return juce::Colour(0xFF5F9EA0); // CadetBlue
        if (id == "Opera")     return juce::Colour(0xFFD4AF37);
        if (id == "Orca")      return juce::Colour(0xFF1B2838);
        if (id == "Orchard")   return juce::Colour(0xFFFFB7C5);
        if (id == "Osier")     return juce::Colour(0xFFC0C8C8);
        if (id == "Osmosis")   return juce::Colour(0xFFC0C0C0);
        if (id == "Otis")      return juce::Colour(0xFFD4A017);
        if (id == "Oto")       return juce::Colour(0xFFF5F0E8);
        if (id == "Outlook")   return juce::Colour(0xFF4169E1);
        if (id == "Oven")      return juce::Colour(0xFF1C1C1C);
        if (id == "Overcast")  return juce::Colour(0xFF778899);
        if (id == "Overflow")  return juce::Colour(0xFF1A3A5C);
        if (id == "Overgrow")  return juce::Colour(0xFF228B22);
        if (id == "Overwash")  return juce::Colour(0xFFF0F8FF);
        if (id == "Overworn")  return juce::Colour(0xFF808080);
        if (id == "Oware")     return juce::Colour(0xFFB5883E);
        if (id == "Oxalis")    return juce::Colour(0xFF9B59B6);
        if (id == "Oxbow")     return juce::Colour(0xFF1A6B5A);
        if (id == "Oxytocin")  return juce::Colour(0xFF9B5DE5);
        return get(borderGray());
    }

    inline juce::String prefixForEngine(const juce::String& id)
    {
        return frozenPrefixForEngine(id);
    }

} // namespace GalleryColors

//==============================================================================
// GalleryFonts — centralized typography (Space Grotesk, Inter, JetBrains Mono)
namespace GalleryFonts {

    inline juce::Typeface::Ptr loadTypeface(const char* data, int size)
    {
        return juce::Typeface::createSystemTypefaceFor(data, static_cast<size_t>(size));
    }

    inline juce::Typeface::Ptr spaceGroteskBold()
    {
        static auto tf = loadTypeface(FontData::SpaceGroteskBold_otf,
                                       FontData::SpaceGroteskBold_otfSize);
        return tf;
    }
    inline juce::Typeface::Ptr spaceGroteskMedium()
    {
        static auto tf = loadTypeface(FontData::SpaceGroteskMedium_otf,
                                       FontData::SpaceGroteskMedium_otfSize);
        return tf;
    }
    inline juce::Typeface::Ptr interRegular()
    {
        static auto tf = loadTypeface(FontData::InterRegular_ttf,
                                       FontData::InterRegular_ttfSize);
        return tf;
    }
    inline juce::Typeface::Ptr interMedium()
    {
        static auto tf = loadTypeface(FontData::InterMedium_ttf,
                                       FontData::InterMedium_ttfSize);
        return tf;
    }
    inline juce::Typeface::Ptr interBold()
    {
        static auto tf = loadTypeface(FontData::InterBold_ttf,
                                       FontData::InterBold_ttfSize);
        return tf;
    }
    inline juce::Typeface::Ptr jetBrainsMono()
    {
        static auto tf = loadTypeface(FontData::JetBrainsMonoRegular_ttf,
                                       FontData::JetBrainsMonoRegular_ttfSize);
        return tf;
    }
    inline juce::Typeface::Ptr overbitRegular()
    {
        static auto tf = loadTypeface(FontData::OverbitRegular_otf,
                                       FontData::OverbitRegular_otfSize);
        return tf;
    }

    inline juce::Font display    (float size) { return juce::Font(juce::FontOptions{}.withTypeface(spaceGroteskBold()).withHeight(size)); }
    inline juce::Font heading    (float size) { return juce::Font(juce::FontOptions{}.withTypeface(interBold()).withHeight(size)); }
    inline juce::Font body       (float size) { return juce::Font(juce::FontOptions{}.withTypeface(interRegular()).withHeight(size)); }
    inline juce::Font label      (float size) { return juce::Font(juce::FontOptions{}.withTypeface(interMedium()).withHeight(size)); }
    inline juce::Font value      (float size) { return juce::Font(juce::FontOptions{}.withTypeface(jetBrainsMono()).withHeight(size)); }
    // Overbit — engine nameplate display font (accent-colored, Column B hero text)
    inline juce::Font engineName (float size) { return juce::Font(juce::FontOptions{}.withTypeface(overbitRegular()).withHeight(size)); }

} // namespace GalleryFonts

//==============================================================================
// A11y — WCAG 2.1 AA accessibility utilities
namespace A11y {

    inline juce::Colour focusRingColour()
    {
        return GalleryColors::darkMode()
            ? juce::Colour(0xFF58A6FF)
            : juce::Colour(0xFF0066CC);
    }

    inline void drawFocusRing(juce::Graphics& g, juce::Rectangle<float> bounds,
                              float cornerRadius = 4.0f)
    {
        g.setColour(focusRingColour());
        g.drawRoundedRectangle(bounds.reduced(1.0f), cornerRadius, 2.0f);
    }

    inline void drawCircularFocusRing(juce::Graphics& g, float cx, float cy, float radius)
    {
        g.setColour(focusRingColour());
        g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 2.0f);
    }

    inline void setup(juce::Component& comp, const juce::String& title,
                      const juce::String& description = {},
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

    inline bool prefersReducedMotion()
    {
#if JUCE_MAC
        // Read the "Reduce Motion" accessibility preference via CoreFoundation.
        // CFPreferencesGetAppBooleanValue is a pure-C API callable from C++ without ObjC.
        // Key:    "reduceMotion"
        // Domain: "com.apple.universalaccess"
        // This is the same preference that NSWorkspace.accessibilityDisplayShouldReduceMotion
        // reads, but accessible without an ObjC runtime call.
        // CoreFoundation.h is included at the top of this file under #if JUCE_MAC.
        Boolean keyExists = false;
        Boolean val = CFPreferencesGetAppBooleanValue(
            CFSTR("reduceMotion"),
            CFSTR("com.apple.universalaccess"),
            &keyExists);
        return keyExists && static_cast<bool>(val);
#elif JUCE_IOS
        // UIAccessibility.isReduceMotionEnabled — implemented in HapticEngine_iOS.mm
        // via xolokun::a11y_platform::isReduceMotionEnabled() bridge function.
        // The definition is in that .mm file compiled only on iOS.
        return xolokun::a11y_platform::isReduceMotionEnabled();
#else
        return false;
#endif
    }

} // namespace A11y

} // namespace xolokun
