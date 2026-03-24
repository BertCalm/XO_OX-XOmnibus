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

namespace xolokun {
namespace GalleryColors {

    // Theme state — light by default (brand rule)
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

    // Dark palette (from master spec section 6.2–6.4)
    namespace Dark {
        constexpr uint32_t shellWhite = 0xFF1A1A1A;
        constexpr uint32_t textDark   = 0xFFEEEEEE;
        constexpr uint32_t textMid    = 0xFFC8C8C8;
        constexpr uint32_t borderGray = 0xFF4A4A4A;
        constexpr uint32_t slotBg     = 0xFF2D2D2D;
        constexpr uint32_t emptySlot  = 0xFF363636;
        constexpr uint32_t xoGoldText = 0xFFE9C46A;
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

    // Backward compatibility constants
    constexpr uint32_t shellWhite_v = 0xFFF8F6F3;
    constexpr uint32_t textDark_v   = 0xFF1A1A1A;
    constexpr uint32_t textMid_v    = 0xFF777570;
    constexpr uint32_t borderGray_v = 0xFFDDDAD5;
    constexpr uint32_t slotBg_v     = 0xFFFCFBF9;
    constexpr uint32_t emptySlot_v  = 0xFFEAE8E4;

    inline juce::Colour get(uint32_t hex) { return juce::Colour(hex); }

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
        if (id == "Overbite")  return juce::Colour(0xFFF0EDE8);
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
        if (id == "XOverlap")  return juce::Colour(0xFF00FFB4);
        if (id == "XOutwit")   return juce::Colour(0xFFCC6600);
        if (id == "OpenSky")   return juce::Colour(0xFFFF8C00);
        if (id == "Ostinato")  return juce::Colour(0xFFE8701A);
        if (id == "Oceandeep") return juce::Colour(0xFF2D0A4E);
        if (id == "Ouie")      return juce::Colour(0xFF708090);
        if (id == "Obrix")     return juce::Colour(0xFF1E8B7E);
        if (id == "Orbweave")  return juce::Colour(0xFF8E4585);
        if (id == "Overtone")  return juce::Colour(0xFFA8D8EA);
        if (id == "Organism")  return juce::Colour(0xFFC6E377);
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

    inline juce::Font display (float size) { return juce::Font(spaceGroteskBold()).withHeight(size); }
    inline juce::Font heading (float size) { return juce::Font(interBold()).withHeight(size); }
    inline juce::Font body    (float size) { return juce::Font(interRegular()).withHeight(size); }
    inline juce::Font label   (float size) { return juce::Font(interMedium()).withHeight(size); }
    inline juce::Font value   (float size) { return juce::Font(jetBrainsMono()).withHeight(size); }

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
       #if JUCE_MAC || JUCE_IOS
        return juce::Desktop::getInstance().isScreenSaverEnabled() == false;
       #else
        return false;
       #endif
    }

} // namespace A11y

} // namespace xolokun
