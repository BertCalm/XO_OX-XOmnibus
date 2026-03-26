#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOlokunProcessor.h"
#include "../../Core/EngineRegistry.h"
#include "../GalleryColors.h"
#include "WaveformDisplay.h"
#include "EnginePickerPopup.h"

namespace xolokun
{

//==============================================================================
// CompactEngineTile — slim tile in the left Column A (260pt wide).
//
// Layout at 260pt width:
//   [4pt margin][16pt toggle][8pt gap][porthole 30pt][8pt gap][name+bars+dots][auto][24pt voice dots][4pt margin]
//
//   Left section  (24pt): On/Off mute toggle (16×16pt, top-left, 4pt margin)
//   Center section: porthole + engine name (top), mini waveform (32×16pt),
//                   macro indicator bars (4×40×3pt), coupling dots (4×4pt)
//   Right section (24pt): Voice activity dots (right edge)
//
// Per-tile enhancements added 2026-03-25:
//   • Mute toggle — visual-only (16×16pt, top-left). No APVTS param yet; calls
//     nothing on the processor. Active = accent-filled circle, Muted = gray
//     circle with diagonal line, 40% opacity. Stored in isMuted member.
//   • Macro indicator bars — 4 × (40×3pt) horizontal bars below engine name.
//     Colors: M1=XO Gold #E9C46A, M2=Phosphor Green #00FF41,
//             M3=Prism Violet #BF40FF, M4=Teal #00B4A0.
//     Fill proportional to macro1–macro4 APVTS values (0–1). Updated in
//     timerCallback() at 10Hz (same timer as voice count).
//   • Coupling dots — up to 4 dots (4×4pt) in a row. Color per coupling
//     category: XO Gold = modulation, Twilight Blue = audio-rate,
//     Midnight Violet = KnotTopology. Count = active routes touching this slot.
//     Updated in timerCallback().
class CompactEngineTile : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    std::function<void(int)> onSelect; // called with slot index when clicked

    CompactEngineTile(XOlokunProcessor& proc, int slotIndex)
        : processor(proc), slot(slotIndex), miniWave(proc)
    {
        A11y::setup (*this, "Engine Slot " + juce::String (slotIndex + 1),
                     "Click to select engine, right-click for options");
        setExplicitFocusOrder (slotIndex + 1);
        addAndMakeVisible(miniWave);
        macroValues.fill(0.0f);

        // P10 fix: cache parameter ID strings once in constructor to avoid
        // 4 juce::String allocations per tile per 10Hz tick.
        for (int m = 0; m < 4; ++m)
            cachedMacroIds[m] = "macro" + juce::String(m + 1);

        refresh();
        startTimerHz(10); // poll voice count + macros + coupling at 10Hz
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
        miniWave.setSlot(slot);
        if (eng) miniWave.setAccentColour(eng->getAccentColour());
        repaint();
    }

    void timerCallback() override
    {
        bool needsRepaint = false;

        // ── Voice count ────────────────────────────────────────────────────────
        auto* eng = processor.getEngine(slot);
        int newCount = eng ? eng->getActiveVoiceCount() : 0;
        if (newCount != voiceCount)
        {
            voiceCount = newCount;
            needsRepaint = true;
        }

        // ── Macro values (APVTS, message-thread safe) ──────────────────────────
        // P10 fix: use pre-built cachedMacroIds — no juce::String allocation here.
        if (hasEngine)
        {
            auto& apvts = processor.getAPVTS();
            for (int m = 0; m < 4; ++m)
            {
                auto* p = apvts.getRawParameterValue(cachedMacroIds[m]);
                float newVal = p ? p->load() : 0.0f;
                if (std::abs(newVal - macroValues[m]) > 0.001f)
                {
                    macroValues[m] = newVal;
                    needsRepaint = true;
                }
            }
        }

        // ── Coupling route count ───────────────────────────────────────────────
        // P3 fix: only check coupling routes every 5th tick (2Hz effective) to
        // avoid copying the full routes vector 50×/sec across all tiles.
        ++couplingCheckCounter;
        if (couplingCheckCounter >= 5)
        {
            couplingCheckCounter = 0;
            auto routes = processor.getCouplingMatrix().getRoutes();
            int modCount   = 0; // LFO/Env/Amp/Filter/Pitch/Rhythm
            int audioCount = 0; // AudioTo*
            int knotCount  = 0; // KnotTopology
            for (const auto& r : routes)
            {
                if (!r.active) continue;
                if (r.sourceSlot != slot && r.destSlot != slot) continue;

                switch (r.type)
                {
                    case CouplingType::AudioToFM:
                    case CouplingType::AudioToRing:
                    case CouplingType::AudioToWavetable:
                    case CouplingType::AudioToBuffer:
                        audioCount = juce::jmin(audioCount + 1, 4);
                        break;
                    case CouplingType::KnotTopology:
                        knotCount = juce::jmin(knotCount + 1, 4);
                        break;
                    default:
                        modCount = juce::jmin(modCount + 1, 4);
                        break;
                }
            }
            int totalDots = juce::jmin(modCount + audioCount + knotCount, 4);
            if (totalDots != couplingDotCount
                || modCount   != couplingModCount
                || audioCount != couplingAudioCount
                || knotCount  != couplingKnotCount)
            {
                couplingDotCount   = totalDots;
                couplingModCount   = modCount;
                couplingAudioCount = audioCount;
                couplingKnotCount  = knotCount;
                needsRepaint = true;
            }
        }

        if (needsRepaint) repaint();
    }

    // Ecological tile: porthole circle + accent strip + depth-zone gradient on select.
    // Voice-reactive: porthole ring brightens and strip glows when voices are playing.
    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;
        auto b = getLocalBounds().toFloat().reduced(3.0f, 2.0f);
        bool hovered = isMouseOver();

        // ── Tile background ── prototype: rgba(255,255,255,0.04) active, transparent inactive
        if (isSelected && hasEngine)
        {
            // Active tile: subtle accent tint + faint white fill for depth
            g.setColour(juce::Colour(0x0AFFFFFF)); // rgba(255,255,255,0.04)
            g.fillRoundedRectangle(b, 4.0f);
            // Overlay a soft accent gradient
            juce::ColourGradient grad(accent.withAlpha(0.09f), b.getX(), b.getCentreY(),
                                      juce::Colours::transparentBlack,
                                      b.getRight(), b.getCentreY(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(b, 4.0f);
        }
        else if (hovered)
        {
            g.setColour(juce::Colour(0x07FFFFFF)); // rgba(255,255,255,0.028) hover
            g.fillRoundedRectangle(b, 4.0f);
        }
        // Empty/inactive: transparent background — tile sits on body bg #080809

        // Bottom border — 1px rgba(255,255,255,0.07) layer separator
        g.setColour(juce::Colour(0x12FFFFFF)); // rgba(255,255,255,0.07)
        g.fillRect(b.getX(), b.getBottom() - 1.0f, b.getWidth(), 1.0f);

        if (isLoading)
        {
            g.setColour(get(xoGold).withAlpha(0.5f));
            g.setFont(GalleryFonts::body(9.0f));
            g.drawText("LOADING...", b.toNearestInt(), juce::Justification::centred);
            if (hasKeyboardFocus(true)) A11y::drawFocusRing(g, b, 8.0f);
            return;
        }

        // ── Mute toggle — top-left corner, 4pt margin ─────────────────────
        paintMuteToggle(g, b);

        if (hasEngine)
        {
            // Voice density: smooth sqrt ramp 0 (silent) → 1 (full polyphony)
            const float kMaxVoices = 8.0f;
            float voiceDensity = (voiceCount > 0)
                ? juce::jmin(1.0f, std::sqrt((float)voiceCount / kMaxVoices))
                : 0.0f;

            float fillAlpha  = 0.09f + voiceDensity * 0.19f;
            float ringAlpha  = 0.38f + voiceDensity * 0.52f;
            float ringStroke = 1.0f  + voiceDensity * 1.0f;
            float stripAlpha = 0.38f + voiceDensity * 0.50f;

            // ── Left accent bar — 3px wide, engine accent color, rounded right edge ─
            // Prototype: 3px wide, border-radius 0 2px 2px 0 on the right side
            float stripX = b.getX() + 24.0f + 1.5f;
            float stripH = b.getHeight() * 0.80f; // taller for more presence
            float stripY = b.getCentreY() - stripH * 0.5f;
            g.setColour(accent.withAlpha(stripAlpha));
            g.fillRoundedRectangle(stripX, stripY, 3.0f, stripH, 1.5f);

            // Active glow behind accent bar (prototype: box-shadow 0 0 8px accent)
            if (isSelected)
            {
                // Paint a soft blurred glow behind the strip using layered rects
                for (int gx = 1; gx <= 6; ++gx)
                {
                    float glowAlpha = (0.12f / (float)gx) * (stripAlpha * 1.4f);
                    g.setColour(accent.withAlpha(juce::jmin(glowAlpha, 0.18f)));
                    g.fillRoundedRectangle(stripX - (float)gx, stripY - 1.0f,
                                           3.0f + (float)(gx * 2), stripH + 2.0f, 2.0f);
                }
            }

            // ── Porthole circle — 8pt right of the strip ──────────────────
            const float porW = 30.0f;
            float porCx = stripX + 3.0f + 8.0f + porW * 0.5f;
            float porCy = b.getCentreY();
            float porR  = porW * 0.5f;

            g.setColour(accent.withAlpha(fillAlpha));
            g.fillEllipse(porCx - porR, porCy - porR, porW, porW);

            g.setColour(accent.withAlpha(ringAlpha));
            g.drawEllipse(porCx - porR, porCy - porR, porW, porW, ringStroke);

            // Glass highlight arc
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

            // Slot number inside porthole — 9px mono, T3 color
            g.setFont(GalleryFonts::value(9.0f));
            g.setColour(juce::Colour(0xFF5E5C5A).withAlpha(0.50f + voiceDensity * 0.37f)); // T3
            g.drawText(juce::String(slot + 1),
                       (int)(porCx - porR), (int)(porCy - porR),
                       (int)porW, (int)porW, juce::Justification::centred);

            // ── Engine name — right of porthole ────────────────────────────
            // Prototype: 11px, weight 600 (Space Grotesk SemiBold), uppercase,
            //            engine accent color, letter-spacing ~1.5px
            float nameX = porCx + porR + 7.0f;
            // Reserve 24pt on right for voice dots; leave room for bars/dots below
            float nameW = b.getRight() - 24.0f - nameX - 4.0f;
            g.setFont(GalleryFonts::display(11.0f)); // Space Grotesk Bold for weight
            // Always use accent color for engine name (prototype spec)
            g.setColour(hasEngine ? accent : juce::Colour(0xFF3A3938)); // T4 when empty
            // Name draws in the upper portion of the tile
            float nameH = b.getHeight() * 0.40f;
            float nameY = b.getY();
            g.drawText(engineId.toUpperCase(),
                       (int)nameX, (int)nameY, (int)nameW, (int)nameH,
                       juce::Justification::centredLeft);

            // ── Macro indicator bars ───────────────────────────────────────
            paintMacroBars(g, nameX, nameY + nameH + 2.0f);

            // ── Coupling dots ──────────────────────────────────────────────
            float dotsY = nameY + nameH + 2.0f + 4 * 5.0f + 2.0f; // below bars
            paintCouplingDots(g, nameX, dotsY);

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
            // Empty slot — dashed border + "+" affordance
            // Prototype: 1px dashed rgba(255,255,255,0.11) border, "+" text T4
            // P8 fix: use cachedDashedPath built once in resized() instead of
            // recreating it (createDashedStroke allocates) on every paint call.
            juce::Colour dashCol(0x1CFFFFFF); // rgba(255,255,255,0.11)
            g.setColour(dashCol);
            g.strokePath(cachedDashedPath, juce::PathStrokeType(1.0f));

            // "+" icon center
            float cx = b.getCentreX(), cy = b.getCentreY();
            float armLen = 6.0f, armW = 1.5f;
            g.setColour(juce::Colour(0xFF3A3938)); // T4 text
            g.fillRoundedRectangle(cx - armLen, cy - armW * 0.5f, armLen * 2.0f, armW, armW * 0.5f);
            g.fillRoundedRectangle(cx - armW * 0.5f, cy - armLen, armW, armLen * 2.0f, armW * 0.5f);
        }

        // Focus ring (WCAG 2.4.7)
        if (hasKeyboardFocus(true))
            A11y::drawFocusRing(g, b, 8.0f);
    }

    void resized() override
    {
        // MiniWaveform: 32×16pt, positioned below engine name in center section.
        // Center section starts after 24pt toggle zone + 4pt strip + 38pt porthole.
        // Place waveform in bottom-center of the content area (right of porthole).
        auto b = getLocalBounds().reduced(3, 2);

        // The strip is at b.getX()+24+1.5, porthole right edge is at:
        //   stripX + 3 + 8 + 30 = b.getX() + 24 + 1.5 + 3 + 8 + 30 = b.getX() + 66.5
        // Content right is b.getRight() - 24 (voice dots zone).
        // Place miniWave at bottom of content zone, left-aligned after porthole.
        int contentLeft = b.getX() + 68; // right of porthole
        int waveW = 40, waveH = 16;
        int waveX = contentLeft;
        int waveY = b.getBottom() - waveH - 2;
        miniWave.setBounds(waveX, waveY, waveW, waveH);

        // P8 fix: pre-compute the dashed border path for empty slots here so
        // paint() can blit it cheaply without calling createDashedStroke every frame.
        {
            auto bf = b.toFloat();
            const float dashLen = 6.0f, gap = 4.0f, radius = 4.0f;
            juce::Path roundRect;
            roundRect.addRoundedRectangle(bf, radius);
            juce::PathStrokeType stroke(1.0f);
            float dashPattern[] = { dashLen, gap };
            cachedDashedPath.clear();
            stroke.createDashedStroke(cachedDashedPath, roundRect, dashPattern, 2);
        }
    }

    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&)  override { repaint(); }
    void focusGained (juce::Component::FocusChangeType) override { repaint(); }
    void focusLost   (juce::Component::FocusChangeType) override { repaint(); }

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
        // Check if click is on the mute toggle (top-left 16×16pt, 4pt margin)
        auto toggleBounds = getMuteToggleBounds();
        if (toggleBounds.contains(e.getPosition()))
        {
            // Handle mute toggle click — toggle the muted state
            if (!e.mods.isPopupMenu())
            {
                isMuted = !isMuted;
                processor.setSlotMuted(slot, isMuted);
                repaint();
                return;
            }
        }

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

        // Don't navigate to engine if mute toggle was clicked
        auto toggleBounds = getMuteToggleBounds();
        if (toggleBounds.contains(e.getPosition()))
            return;

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
    // ── Mute toggle geometry ─────────────────────────────────────────────────
    // 16×16pt circle, 4pt from top-left of the tile bounds (after reduction).
    juce::Rectangle<int> getMuteToggleBounds() const
    {
        auto b = getLocalBounds().reduced(3, 2);
        return { b.getX() + 4, b.getY() + (b.getHeight() - 16) / 2, 16, 16 };
    }

    void paintMuteToggle(juce::Graphics& g, juce::Rectangle<float> tileBounds) const
    {
        // Position: 4pt inset from tile left edge, vertically centred
        float tx = tileBounds.getX() + 4.0f;
        float ty = tileBounds.getCentreY() - 8.0f;
        float tw = 16.0f, th = 16.0f;

        if (!hasEngine)
            return; // no toggle on empty slot

        if (!isMuted)
        {
            // Active: accent-filled circle, 100% opacity
            g.setColour(accent);
            g.fillEllipse(tx, ty, tw, th);
            // Thin white ring inside for depth
            g.setColour(juce::Colours::white.withAlpha(0.30f));
            g.drawEllipse(tx + 1.5f, ty + 1.5f, tw - 3.0f, th - 3.0f, 1.0f);
        }
        else
        {
            // Muted: gray filled circle with diagonal line, 40% opacity
            g.setColour(juce::Colour(0xFF888888).withAlpha(0.40f));
            g.fillEllipse(tx, ty, tw, th);
            g.setColour(juce::Colour(0xFFFFFFFF).withAlpha(0.55f));
            // Diagonal line from top-right to bottom-left (mute convention)
            g.drawLine(tx + tw * 0.75f, ty + th * 0.15f,
                       tx + tw * 0.25f, ty + th * 0.85f, 1.5f);
        }
    }

    // ── Macro indicator bars ─────────────────────────────────────────────────
    // 4 horizontal bars, each 40×3pt, stacked with 2pt gap.
    // Colors: M1=XO Gold, M2=Phosphor Green, M3=Prism Violet, M4=Teal.
    void paintMacroBars(juce::Graphics& g, float startX, float startY) const
    {
        if (!hasEngine) return;

        static constexpr uint32_t macroColors[4] = {
            0xFFE9C46A,  // M1 — XO Gold
            0xFF00FF41,  // M2 — Phosphor Green
            0xFFBF40FF,  // M3 — Prism Violet
            0xFF00B4A0   // M4 — Teal
        };

        const float barMaxW = 40.0f;
        const float barH    = 3.0f;
        const float gap     = 2.0f;

        for (int m = 0; m < 4; ++m)
        {
            float barY = startY + static_cast<float>(m) * (barH + gap);
            float fillW = juce::jlimit(0.0f, barMaxW, macroValues[m] * barMaxW);

            // Background track (dim)
            g.setColour(juce::Colour(macroColors[m]).withAlpha(0.15f));
            g.fillRoundedRectangle(startX, barY, barMaxW, barH, 1.0f);

            // Filled portion
            if (fillW > 0.5f)
            {
                g.setColour(juce::Colour(macroColors[m]).withAlpha(0.70f));
                g.fillRoundedRectangle(startX, barY, fillW, barH, 1.0f);
            }
        }
    }

    // ── Coupling indicator dots ──────────────────────────────────────────────
    // Up to 4 dots (4×4pt) in a row. Color per coupling category:
    //   XO Gold (#E9C46A)        — modulation routes (LFO/Env/Amp/Filter/Pitch/Rhythm)
    //   Twilight Blue (#1B4F8A)  — audio-rate routes (AudioTo*)
    //   Midnight Violet (#7B2FBE) — KnotTopology routes
    void paintCouplingDots(juce::Graphics& g, float startX, float startY) const
    {
        if (!hasEngine || couplingDotCount == 0) return;

        // Build a color list: mod dots first, then audio, then knot
        juce::Colour dotColors[4];
        int idx = 0;
        for (int i = 0; i < couplingModCount   && idx < 4; ++i, ++idx)
            dotColors[idx] = juce::Colour(0xFFE9C46A); // XO Gold
        for (int i = 0; i < couplingAudioCount && idx < 4; ++i, ++idx)
            dotColors[idx] = juce::Colour(0xFF1B4F8A); // Twilight Blue
        for (int i = 0; i < couplingKnotCount  && idx < 4; ++i, ++idx)
            dotColors[idx] = juce::Colour(0xFF7B2FBE); // Midnight Violet

        const float dotSize    = 4.0f;
        const float dotSpacing = 6.0f;

        for (int d = 0; d < couplingDotCount && d < 4; ++d)
        {
            float dotX = startX + static_cast<float>(d) * dotSpacing;
            g.setColour(dotColors[d].withAlpha(0.85f));
            g.fillEllipse(dotX, startY, dotSize, dotSize);
        }
    }

    void showLoadMenu()
    {
        auto* picker = new EnginePickerPopup();
        // Use SafePointer so the callback is a no-op if the tile is destroyed
        // before the CallOutBox closes (e.g. rapid slot changes or window close).
        auto safeThis = juce::Component::SafePointer<CompactEngineTile>(this);
        picker->onEngineSelected = [safeThis](const juce::String& engineId)
        {
            if (safeThis == nullptr) return;
            safeThis->isLoading = true;
            safeThis->repaint();
            safeThis->processor.loadEngine(safeThis->slot, engineId.toStdString());
            juce::Timer::callAfterDelay(0, [safeThis]
            {
                if (safeThis == nullptr) return;
                if (safeThis->onSelect) safeThis->onSelect(safeThis->slot);
            });
        };
        picker->setSize(280, 400);
        juce::CallOutBox::launchAsynchronously(
            std::unique_ptr<juce::Component>(picker),
            getScreenBounds(), nullptr);
    }

    XOlokunProcessor& processor;
    int slot;
    juce::String engineId;
    juce::Colour accent;
    bool hasEngine  = false;
    bool isSelected = false;
    bool isLoading  = false;
    bool isMuted    = false; // visual-only; no APVTS param yet
    int  voiceCount = 0;

    // Macro bar values — updated in timerCallback at 10Hz
    std::array<float, 4> macroValues {};

    // Coupling dot state — updated in timerCallback at 10Hz
    int couplingDotCount   = 0;
    int couplingModCount   = 0;
    int couplingAudioCount = 0;
    int couplingKnotCount  = 0;

    // P3 fix: tick counter to run coupling check only every 5th tick (2Hz effective)
    int couplingCheckCounter = 0;

    // P10 fix: pre-built parameter ID strings — avoids 4 allocations/tick/tile
    std::array<juce::String, 4> cachedMacroIds;

    // P8 fix: dashed border path for empty slot — built once in resized()
    juce::Path cachedDashedPath;

    MiniWaveform miniWave;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompactEngineTile)
};

} // namespace xolokun
