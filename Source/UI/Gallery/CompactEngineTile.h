#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../XOlokunProcessor.h"
#include "../../Core/EngineRegistry.h"
#include "../GalleryColors.h"
#include "WaveformDisplay.h"
#include "EnginePickerPopup.h"
#include "CockpitHost.h"

namespace xolokun
{

//==============================================================================
// CompactEngineTile — slim tile in the left Column A (260pt wide).
//
// Layout (top to bottom, inside 9/11/8/14 px padding):
//   1. Left accent bar (3px wide, full tile height, flush x=0)
//   2. Header row: slot number (12px wide, 9px mono, T3) | engine name (flex, 11px bold, accent) | power button (16×16)
//   3. Mini macro knobs row: 4 arcs (SRC1/FILT/ENV/FX), ~20px wide each, 5px gap
//   4. Mini waveform: 22px tall, painted directly in paint() using accent color polyline
//   5. CPU bar: 3px tall, full content width, accent at 0.55 alpha
//
// NOTE: Mood dots, FX indicator, and footer row were removed in the v05 polish pass.
// Redesigned 2026-03-27: Removed porthole circle and creature renderer.
// Power button replaces mute toggle visually (same underlying isMuted state).
class CompactEngineTile : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    std::function<void(int)> onSelect; // called with slot index when clicked

    CompactEngineTile(XOlokunProcessor& proc, int slotIndex)
        : processor(proc), slot(slotIndex), miniWave(proc)
    {
        A11y::setup (*this, "Engine Slot " + juce::String (slotIndex + 1),
                     "Click to open engine detail, right-click for options");
        setExplicitFocusOrder (slotIndex + 1);
        // miniWave is kept as a member for potential future use but hidden —
        // waveform is now painted directly in paint() per the mockup spec.
        addChildComponent(miniWave); // add but invisible
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
        setTooltip(hasEngine ? "Click to open engine detail \xe2\x80\x94 right-click for options"
                             : "Slot " + juce::String(slot + 1) + ": empty — click to load engine");
        accent    = hasEngine ? eng->getAccentColour()
                              : GalleryColors::get(GalleryColors::emptySlot());
        miniWave.setSlot(slot);
        if (eng) miniWave.setAccentColour(eng->getAccentColour());
        // Sync mute state from processor so tile visual matches processor state
        // after preset load, session restore, or any external state change.
        isMuted = processor.isSlotMuted(slot);
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

    // Mockup-matched tile: accent bar + header row + macro knobs + waveform + footer + CPU bar.
    // No porthole, no creature renderer. Power button replaces old mute toggle visually.
    void paint(juce::Graphics& g) override
    {
        // Dark Cockpit B041: active/sounding tiles stay fully lit.
        // Non-active tiles dim with cockpit opacity.
        // Fix #7: use cachedCockpitHost_ (set in parentHierarchyChanged()) instead
        // of dynamic_cast walk on every paint() call.
        {
            bool isSounding = (voiceCount > 0);
            float opacity = 1.0f;
            if (!isSounding)
            {
                if (cachedCockpitHost_ != nullptr)
                    opacity = cachedCockpitHost_->getCockpitOpacity();
                if (opacity < 0.05f) return; // B041 performance optimization
            }
            g.setOpacity(opacity);
        }

        using namespace GalleryColors;
        // Asymmetric padding: 9px top, 11px right, 8px bottom, 14px left
        auto content = getLocalBounds().toFloat();
        content.removeFromTop(9.0f);
        content.removeFromRight(11.0f);
        content.removeFromBottom(8.0f);
        content.removeFromLeft(14.0f);

        auto b = getLocalBounds().toFloat().reduced(1.0f, 0.0f);
        bool hovered = isMouseOver();

        // ── Tile background ──────────────────────────────────────────────────
        if (isSelected && hasEngine)
        {
            g.setColour(juce::Colour(0x0AFFFFFF));
            g.fillRoundedRectangle(b, 4.0f);
            juce::ColourGradient grad(accent.withAlpha(0.09f), b.getX(), b.getCentreY(),
                                      juce::Colours::transparentBlack,
                                      b.getRight(), b.getCentreY(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(b, 4.0f);
        }
        else if (hovered)
        {
            g.setColour(juce::Colour(0x07FFFFFF));
            g.fillRoundedRectangle(b, 4.0f);
        }

        // Bottom border — 1px separator
        g.setColour(GalleryColors::border());
        g.fillRect(b.getX(), b.getBottom() - 1.0f, b.getWidth(), 1.0f);

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
            // Voice density for reactive alpha (accent bar, power button)
            const float kMaxVoices = 8.0f;
            float voiceDensity = (voiceCount > 0)
                ? juce::jmin(1.0f, std::sqrt((float)voiceCount / kMaxVoices))
                : 0.0f;
            float stripAlpha = 0.38f + voiceDensity * 0.50f;

            // ── 1. Left accent bar ─────────────────────────────────────────
            // 3px wide, full tile height, flush x=0, rounded-right corners
            {
                float stripH = (float)getHeight();
                g.setColour(accent.withAlpha(stripAlpha));
                g.fillRoundedRectangle(0.0f, 0.0f, 3.0f, stripH, 1.5f);

                // Active glow (box-shadow: 0 0 8px accent) when selected
                if (isSelected)
                {
                    for (int gx = 1; gx <= 6; ++gx)
                    {
                        float glowAlpha = (0.12f / (float)gx) * (stripAlpha * 1.4f);
                        g.setColour(accent.withAlpha(juce::jmin(glowAlpha, 0.18f)));
                        g.fillRoundedRectangle(0.0f, -1.0f,
                                               3.0f + (float)(gx * 2), stripH + 2.0f, 2.0f);
                    }
                }
            }

            // ── 2. Header row (inside content area) ───────────────────────
            // Slot number | engine name (flex) | power button
            // Row height: 14px, sits at content top
            {
                const float rowH   = 14.0f;
                const float rowY   = content.getY();
                const float slotW  = 12.0f;
                const float pwrW   = 16.0f;
                const float pwrH   = 16.0f;

                // Slot number — 9px mono, T3 color, 12px wide
                g.setFont(GalleryFonts::value(9.0f));
                g.setColour(GalleryColors::get(GalleryColors::t3()));
                g.drawText(juce::String(slot + 1),
                           (int)content.getX(), (int)rowY, (int)slotW, (int)rowH,
                           juce::Justification::centredLeft);

                // Engine name — 14px Space Grotesk bold, uppercase, accent color
                float nameX = content.getX() + slotW + 3.0f;
                float nameW = content.getWidth() - slotW - 3.0f - pwrW - 3.0f;
                g.setFont(GalleryFonts::display(14.0f));
                g.setColour(accent);
                g.drawText(engineId.toUpperCase(),
                           (int)nameX, (int)rowY, (int)nameW, (int)rowH,
                           juce::Justification::centredLeft);

                // Power button — 16×16 circle, right edge of content
                // Active (unmuted): border + text in accent color
                // Muted: border + text in T3 color
                {
                    float pwrX = content.getRight() - pwrW;
                    float pwrY = rowY + (rowH - pwrH) * 0.5f;
                    juce::Colour pwrColor = isMuted
                        ? GalleryColors::get(GalleryColors::t3())
                        : accent;
                    g.setColour(pwrColor);
                    g.drawEllipse(pwrX, pwrY, pwrW, pwrH, 1.0f);
                    g.setFont(GalleryFonts::value(8.0f));
                    g.drawText(isMuted ? "o" : "I",
                               (int)pwrX, (int)pwrY, (int)pwrW, (int)pwrH,
                               juce::Justification::centred);
                }
            }

            // Top-down layout: each row stacks below the previous with 5px gaps.
            // Waveform stretches to fill remaining space (most flexible element).
            // Top-down layout: header → knobs → waveform (stretches to bottom).
            // Mood dots, FX indicator, CPU bar removed — not functional yet, wasted space.
            const float gap      = 4.0f;
            const float knobArcDiam = 40.0f;
            const float knobLblH    = 10.0f;
            const float knobRowH    = knobArcDiam + knobLblH;

            float knobY   = content.getY() + 16.0f + gap;  // below header (16px for larger font)
            float waveTop = knobY + knobRowH + gap;
            float waveH   = content.getBottom() - gap - waveTop;  // stretches to bottom
            float waveY   = waveTop;
            if (waveH < 10.0f) waveH = 10.0f;

            // ── 3. Mini macro knobs row ──────────────────────────────────
            {
                const float arcDiam   = knobArcDiam;
                const float arcRadius = arcDiam * 0.5f;
                const float arcGap    = 5.0f;
                const float arcStep   = arcDiam + arcGap;

                // Left-align knobs (matches mockup)
                float kx = content.getX();

                static const char* kLabels[4] = { "SRC1", "FILT", "ENV", "FX" };

                for (int k = 0; k < 4; ++k)
                {
                    float cx = kx + arcRadius;
                    float cy = knobY + arcRadius;

                    // 270° arc sweep, starts at ~135° (bottom-left), sweeps CW
                    const float startAngle = juce::MathConstants<float>::pi * 0.75f;
                    const float sweepAngle = juce::MathConstants<float>::pi * 1.5f;
                    const float fillPos    = macroValues[k];

                    // Track arc (T4 color, 2.0px)
                    juce::Path trackArc;
                    trackArc.addCentredArc(cx, cy, arcRadius - 2.0f, arcRadius - 2.0f,
                                           0.0f, startAngle, startAngle + sweepAngle, true);
                    g.setColour(GalleryColors::get(GalleryColors::t4()));
                    g.strokePath(trackArc, juce::PathStrokeType(2.0f,
                                 juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

                    // Fill arc (accent, 2.5px)
                    juce::Path fillArc;
                    fillArc.addCentredArc(cx, cy, arcRadius - 2.0f, arcRadius - 2.0f,
                                          0.0f, startAngle, startAngle + sweepAngle * fillPos, true);
                    g.setColour(accent.withAlpha(0.85f));
                    g.strokePath(fillArc, juce::PathStrokeType(2.5f,
                                 juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

                    // Label below arc (8px mono, T2 — lighter for legibility)
                    g.setFont(GalleryFonts::value(8.0f));
                    g.setColour(GalleryColors::get(GalleryColors::t2()));
                    g.drawText(kLabels[k],
                               (int)(kx - 2.0f), (int)(knobY + arcDiam + 1.0f),
                               (int)(arcDiam + 4.0f), (int)knobLblH,
                               juce::Justification::centred);

                    kx += arcStep;
                }
            }

            // ── 4. Mini waveform area ────────────────────────────────────
            // 22px tall, fake sine polyline, accent at 0.7 alpha, 1.2px stroke
            {
                float waveX = content.getX();
                float waveW = content.getWidth();

                // Background
                g.setColour(juce::Colour(0x06FFFFFF)); // rgba(255,255,255,0.025)
                g.fillRoundedRectangle(waveX, waveY, waveW, waveH, 3.0f);

                // Waveform polyline — ~20 points, sine-ish, ±8px amplitude
                {
                    const int   kPoints    = 20;
                    const float amplitude  = 8.0f;
                    const float cy         = waveY + waveH * 0.5f;
                    const float xStep      = waveW / (float)(kPoints - 1);
                    const float phaseOff   = (float)(slot * 37) * 0.1f; // per-slot visual variation

                    juce::Path wavePath;
                    for (int i = 0; i < kPoints; ++i)
                    {
                        float px = waveX + (float)i * xStep;
                        float t  = (float)i / (float)(kPoints - 1);
                        // Sine-ish with a second harmonic for organic feel
                        float py = cy - amplitude * (0.65f * std::sin(t * juce::MathConstants<float>::twoPi + phaseOff)
                                                    + 0.35f * std::sin(t * juce::MathConstants<float>::twoPi * 2.0f + phaseOff * 1.3f));
                        if (i == 0)
                            wavePath.startNewSubPath(px, py);
                        else
                            wavePath.lineTo(px, py);
                    }
                    g.setColour(accent.withAlpha(0.7f));
                    g.strokePath(wavePath, juce::PathStrokeType(1.2f,
                                 juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                }
            }

            // Footer (mood dots, FX indicator) and CPU bar removed —
            // not wired to real data yet, reclaimed space for larger elements.

            // ── 5. Coupling dots ─────────────────────────────────────────
            // Overlaid at bottom-left of waveform area (4px dots, 6px from bottom)
            paintCouplingDots(g, content.getX(), waveY + waveH - 6.0f);
        }
        else
        {
            // Empty slot — 28×28 dashed rounded rect centered, "+" inside,
            // "Add engine" label below. T4 color throughout.
            juce::Colour t4col = GalleryColors::get(GalleryColors::t4());

            float tileCx = (float)getWidth() * 0.5f;
            float tileCy = (float)getHeight() * 0.5f;

            const float btnW = 28.0f, btnH = 28.0f, btnR = 6.0f;
            float btnX = tileCx - btnW * 0.5f;
            float btnY = tileCy - btnH * 0.5f - 7.0f;

            {
                juce::Path btnRect;
                btnRect.addRoundedRectangle(btnX, btnY, btnW, btnH, btnR);
                juce::PathStrokeType stroke(1.0f);
                float dashPattern[] = { 4.0f, 3.0f };
                juce::Path dashedBtn;
                stroke.createDashedStroke(dashedBtn, btnRect, dashPattern, 2);
                g.setColour(t4col.withAlpha(0.70f));
                g.strokePath(dashedBtn, juce::PathStrokeType(1.0f));
            }

            g.setFont(GalleryFonts::body(16.0f));
            g.setColour(t4col);
            g.drawText("+", (int)btnX, (int)btnY, (int)btnW, (int)btnH,
                       juce::Justification::centred);

            g.setFont(GalleryFonts::body(10.0f));
            g.setColour(t4col.withAlpha(0.70f));
            float labelY = btnY + btnH + 4.0f;
            g.drawText("Add engine",
                       (int)(tileCx - 40.0f), (int)labelY, 80, 14,
                       juce::Justification::centred);
        }

        // Focus ring (WCAG 2.4.7)
        if (hasKeyboardFocus(true))
            A11y::drawFocusRing(g, b, 8.0f);
    }

    void resized() override
    {
        // miniWave is hidden (addChildComponent, not addAndMakeVisible) — waveform
        // is drawn directly in paint(). No bounds to set.

        // Pre-compute the dashed border path for empty slots — blit cheaply in paint().
        {
            auto bf = getLocalBounds().toFloat().reduced(1.0f, 0.0f);
            const float dashLen = 6.0f, gap = 4.0f, radius = 4.0f;
            juce::Path roundRect;
            roundRect.addRoundedRectangle(bf, radius);
            juce::PathStrokeType stroke(1.0f);
            float dashPattern[] = { dashLen, gap };
            cachedDashedPath.clear();
            stroke.createDashedStroke(cachedDashedPath, roundRect, dashPattern, 2);
        }
    }

    // Fix #7: cache CockpitHost pointer once when the component hierarchy is set up,
    // avoiding a dynamic_cast walk on every paint() call (O(depth) per frame).
    void parentHierarchyChanged() override
    {
        cachedCockpitHost_ = CockpitHost::find(this);
    }

    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseMove(const juce::MouseEvent& e) override
    {
        if (hasEngine && getMuteToggleBounds().contains(e.getPosition()))
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
        else
            setMouseCursor(juce::MouseCursor::NormalCursor);
    }
    void mouseExit(const juce::MouseEvent&) override
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
    }
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
            if (i == slot)
                continue;
            auto* targetEng = processor.getEngine(i);
            if (targetEng != nullptr)
            {
                // Target slot is occupied — warn user inline via item label
                juce::String occupantName = targetEng->getEngineId().toUpperCase();
                moveMenu.addItem(200 + i,
                    "Replace " + occupantName + " in Slot " + juce::String(i + 1));
            }
            else
            {
                moveMenu.addItem(200 + i, "Move to Slot " + juce::String(i + 1));
            }
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
    // ── Power button hit area ────────────────────────────────────────────────
    // 16×16 button, top-right of content area (right edge = getWidth()-11, top = 9).
    // Hit area extended to 24×24 for touch comfort.
    juce::Rectangle<int> getMuteToggleBounds() const
    {
        // content right = getWidth() - 11; power button right-aligned there
        // content top = 9; header row height = 14; button centered in row
        int pwrRight = getWidth() - 11;
        int pwrX     = pwrRight - 16;
        int rowCy    = 9 + 7; // content top + half header row
        return { pwrX - 4, rowCy - 12, 24, 24 };
    }

    // ── Macro indicator bars ─────────────────────────────────────────────────
    // 4 horizontal bars, each 40×3pt, stacked with 2pt gap.
    // Colors: M1=XO Gold, M2=Phosphor Green, M3=Prism Violet, M4=Teal.
    void paintMacroBars(juce::Graphics& g, float startX, float startY) const
    {
        if (!hasEngine) return;

        static constexpr uint32_t macroColors[4] = {
            GalleryColors::xoGold,  // M1 — XO Gold
            0xFF00FF41,             // M2 — Phosphor Green
            0xFFBF40FF,             // M3 — Prism Violet
            0xFF00B4A0              // M4 — Teal
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
            dotColors[idx] = juce::Colour(GalleryColors::xoGold); // XO Gold
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
    bool isMuted    = false; // synced to processor.slotMuted[] via setSlotMuted/isSlotMuted
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

    // Fix #7: cached CockpitHost pointer — set in parentHierarchyChanged(),
    // used in paint() to avoid dynamic_cast walk on every frame.
    CockpitHost* cachedCockpitHost_ = nullptr;

    MiniWaveform miniWave;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompactEngineTile)
};

} // namespace xolokun
