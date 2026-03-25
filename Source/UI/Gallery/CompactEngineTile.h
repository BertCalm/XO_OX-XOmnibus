#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOlokunProcessor.h"
#include "../../Core/EngineRegistry.h"
#include "../GalleryColors.h"

namespace xolokun
{

//==============================================================================
// CompactEngineTile — slim tile in the left sidebar column.
// Shows engine identity. Click to select (or load engine if empty).
class CompactEngineTile : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    std::function<void(int)> onSelect; // called with slot index when clicked

    CompactEngineTile(XOlokunProcessor& proc, int slotIndex)
        : processor(proc), slot(slotIndex)
    {
        A11y::setup (*this, "Engine Slot " + juce::String (slotIndex + 1),
                     "Click to select engine, right-click for options");
        setExplicitFocusOrder (slotIndex + 1);
        refresh();
        startTimerHz(10); // poll voice count at 10Hz (sufficient for visual feedback)
    }

    ~CompactEngineTile() override { stopTimer(); }

    void refresh()
    {
        auto* eng = processor.getEngine(slot);
        bool newHasEngine = (eng != nullptr);
        juce::String newId = newHasEngine ? eng->getEngineId() : juce::String{};

        // Only repaint when state actually changed — avoids idle repaint overhead.
        if (!isLoading && newHasEngine == hasEngine && newId == engineId)
            return;

        isLoading = false; // engine arrived — clear loading state
        hasEngine = newHasEngine;
        engineId  = newId;
        setTooltip(hasEngine ? "Click to edit parameters. Right-click for options."
                             : "Slot " + juce::String(slot + 1) + ": empty — click to load engine");
        accent    = hasEngine ? eng->getAccentColour()
                              : GalleryColors::get(GalleryColors::emptySlot());
        repaint();
    }

    void timerCallback() override
    {
        auto* eng = processor.getEngine(slot);
        int newCount = eng ? eng->getActiveVoiceCount() : 0;
        if (newCount != voiceCount)
        {
            voiceCount = newCount;
            repaint();
        }
    }

    // Ecological tile: porthole circle + accent strip + depth-zone gradient on select.
    // Voice-reactive: porthole ring brightens and strip glows when voices are playing.
    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        auto b = getLocalBounds().toFloat().reduced(3.0f, 2.0f);
        bool hovered = isMouseOver();

        // ── Tile background ───────────────────────────────────────────────
        if (isSelected && hasEngine)
        {
            // Depth-zone gradient: engine accent (sunlit) → midnight violet
            juce::ColourGradient grad(accent.withAlpha(0.10f), b.getX(), b.getCentreY(),
                                      juce::Colour(0xFF7B2FBE).withAlpha(0.04f),
                                      b.getRight(), b.getCentreY(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(b, 8.0f);
        }
        else
        {
            g.setColour(hovered ? get(slotBg()).brighter(0.025f) : get(slotBg()));
            g.fillRoundedRectangle(b, 8.0f);
        }

        // Border — accent when selected, subtle otherwise
        g.setColour(isSelected ? accent : (hovered ? accent.withAlpha(0.35f) : get(borderGray())));
        g.drawRoundedRectangle(b, 8.0f, isSelected ? 2.0f : 1.0f);

        if (isLoading)
        {
            g.setColour(get(xoGold).withAlpha(0.5f));
            g.setFont(GalleryFonts::body(9.0f));
            g.drawText("LOADING...", b.toNearestInt(), juce::Justification::centred);
            if (hasKeyboardFocus(true)) A11y::drawFocusRing(g, b, 8.0f);
            return;
        }

        if (hasEngine)
        {
            // Voice density: smooth sqrt ramp 0 (silent) → 1 (full polyphony)
            const float kMaxVoices = 8.0f;
            float voiceDensity = (voiceCount > 0)
                ? juce::jmin(1.0f, std::sqrt((float)voiceCount / kMaxVoices))
                : 0.0f;

            // Derived alphas / stroke — replace binary ternaries with smooth curves
            float fillAlpha  = 0.09f + voiceDensity * 0.19f;  // 0.09 (silent) → 0.28 (full)
            float ringAlpha  = 0.38f + voiceDensity * 0.52f;  // 0.38 → 0.90
            float ringStroke = 1.0f  + voiceDensity * 1.0f;   // 1.0  → 2.0
            float stripAlpha = 0.38f + voiceDensity * 0.50f;  // 0.38 → 0.88

            // ── Left accent strip — voice activity indicator ───────────────
            float stripX = b.getX() + 1.5f;
            float stripH = b.getHeight() * 0.55f;
            float stripY = b.getCentreY() - stripH * 0.5f;
            g.setColour(accent.withAlpha(stripAlpha));
            g.fillRoundedRectangle(stripX, stripY, 3.0f, stripH, 1.5f);

            // ── Porthole circle ────────────────────────────────────────────
            const float porW = 30.0f;
            float porCx = b.getX() + 20.0f + porW * 0.5f;
            float porCy = b.getCentreY();
            float porR  = porW * 0.5f;

            // Inner fill — brightest when voices active
            g.setColour(accent.withAlpha(fillAlpha));
            g.fillEllipse(porCx - porR, porCy - porR, porW, porW);

            // Porthole ring
            g.setColour(accent.withAlpha(ringAlpha));
            g.drawEllipse(porCx - porR, porCy - porR, porW, porW, ringStroke);

            // Glass highlight arc — top-left arc (porthole glass illusion)
            {
                float hR = porR - 2.0f;
                juce::Path hl;
                hl.addCentredArc(porCx, porCy, hR, hR, 0,
                                  -juce::MathConstants<float>::pi * 1.15f,
                                  -juce::MathConstants<float>::pi * 0.45f, true);
                g.setColour(juce::Colours::white.withAlpha(0.20f));
                g.strokePath(hl, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
            }

            // Slot number inside porthole
            g.setFont(GalleryFonts::value(9.0f));
            g.setColour(accent.withAlpha(0.38f + voiceDensity * 0.37f));  // 0.38 → 0.75
            g.drawText(juce::String(slot + 1),
                       (int)(porCx - porR), (int)(porCy - porR),
                       (int)porW, (int)porW, juce::Justification::centred);

            // ── Engine name ────────────────────────────────────────────────
            float nameX = porCx + porR + 7.0f;
            float nameW = b.getRight() - nameX - 18.0f;
            g.setFont(GalleryFonts::heading(11.0f));
            g.setColour(isSelected ? accent : get(textDark()));
            g.drawText(engineId.toUpperCase(),
                       (int)nameX, (int)b.getY(), (int)nameW, (int)b.getHeight(),
                       juce::Justification::centredLeft);

            // ── Voice activity dots — right edge ───────────────────────────
            if (voiceCount > 0)
            {
                const float dotR = 2.5f, dotSpacing = 5.5f;
                float dotX = b.getRight() - 7.0f;
                float dotY = b.getCentreY() - dotR;
                int maxDots = std::min(voiceCount, 4);
                for (int d = 0; d < maxDots; ++d)
                {
                    float alpha = d == 0 ? 0.88f : juce::jmax(0.30f, 0.7f - d * 0.2f);
                    g.setColour(accent.withAlpha(alpha));
                    g.fillEllipse(dotX - static_cast<float>(d) * dotSpacing,
                                  dotY, dotR * 2.0f, dotR * 2.0f);
                }
            }
        }
        else
        {
            // Empty slot — soft "+" affordance (warmer invite than a slot number)
            float cx = b.getCentreX(), cy = b.getCentreY();
            float armLen = 7.0f, armW = 1.5f;
            juce::Colour plusCol = get(textMid()).withAlpha(0.28f);
            g.setColour(plusCol);
            g.fillRoundedRectangle(cx - armLen, cy - armW * 0.5f, armLen * 2.0f, armW, armW * 0.5f);
            g.fillRoundedRectangle(cx - armW * 0.5f, cy - armLen, armW, armLen * 2.0f, armW * 0.5f);
        }

        // Focus ring (WCAG 2.4.7)
        if (hasKeyboardFocus(true))
            A11y::drawFocusRing(g, b, 8.0f);
    }

    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&)  override { repaint(); }
    void focusGained (juce::Component::FocusChangeType) override { repaint(); }
    void focusLost   (juce::Component::FocusChangeType) override { repaint(); }

    // Keyboard activation (WCAG 2.1.1 — all interactive elements operable via keyboard)
    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey)
        {
            if (hasEngine)
            {
                if (onSelect) onSelect (slot);
            }
            else
                showLoadMenu();
            return true;
        }
        return false;
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (!e.mods.isPopupMenu() || !hasEngine)
            return;

        juce::PopupMenu menu;
        menu.addSectionHeader("SLOT " + juce::String(slot + 1) + ": " + engineId.toUpperCase());
        menu.addSeparator();
        menu.addItem(100, "Change Engine...");
        menu.addItem(101, "Remove Engine");
        menu.addSeparator();

        juce::PopupMenu moveMenu;
        for (int i = 0; i < 4; ++i)
        {
            if (i != slot)
                moveMenu.addItem(200 + i, "Move to Slot " + juce::String(i + 1));
        }
        menu.addSubMenu("Move to Slot", moveMenu);

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
            [this](int result)
            {
                if (result == 100)
                    showLoadMenu();
                else if (result == 101)
                {
                    processor.unloadEngine(slot);
                }
                else if (result >= 200 && result < 204)
                {
                    int targetSlot = result - 200;
                    // Get current engine ID before unloading
                    auto* eng = processor.getEngine(slot);
                    if (eng != nullptr)
                    {
                        auto currentId = eng->getEngineId().toStdString();
                        processor.loadEngine(targetSlot, currentId);
                        processor.unloadEngine(slot);
                    }
                }
            });
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (e.mouseWasDraggedSinceMouseDown())
            return;

        if (e.mods.isPopupMenu())
            return;  // right-click handled by mouseDown

        if (hasEngine)
        {
            if (onSelect) onSelect(slot);
        }
        else
        {
            showLoadMenu();
        }
    }

    void setSelected(bool sel) { isSelected = sel; repaint(); }

private:
    void showLoadMenu()
    {
        // Dynamically query the registry — no hardcoded ID list to keep in sync.
        auto registeredIds = EngineRegistry::instance().getRegisteredIds();

        juce::PopupMenu menu;
        menu.addSectionHeader("LOAD INTO SLOT " + juce::String(slot + 1));
        menu.addSeparator();

        for (int i = 0; i < (int)registeredIds.size(); ++i)
        {
            juce::String id(registeredIds[static_cast<size_t>(i)].c_str());
            auto colour = GalleryColors::accentForEngine(id);
            menu.addColouredItem(i + 1, id, colour, true, false);
        }

        if (hasEngine)
        {
            menu.addSeparator();
            menu.addItem(9999, "Remove Engine from Slot " + juce::String(slot + 1));
        }

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
            [this, registeredIds](int result)
            {
                if (result == 9999)
                {
                    processor.unloadEngine(slot);
                    return;
                }

                if (result >= 1 && result <= (int)registeredIds.size())
                {
                    isLoading = true;
                    repaint(); // show "LOADING..." immediately
                    processor.loadEngine(slot, registeredIds[static_cast<size_t>(result - 1)]);
                    // Refresh is driven by onEngineChanged (event-driven, via callAsync).
                    // Navigate to detail panel on the next message loop tick.
                    juce::Timer::callAfterDelay(0, [this]
                    {
                        if (onSelect) onSelect(slot);
                    });
                }
            });
    }

    XOlokunProcessor& processor;
    int slot;
    juce::String engineId;
    juce::Colour accent;
    bool hasEngine  = false;
    bool isSelected = false;
    bool isLoading  = false; // true between loadEngine() call and onEngineChanged callback
    int  voiceCount = 0;     // updated by timerCallback at 20Hz

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompactEngineTile)
};

} // namespace xolokun
