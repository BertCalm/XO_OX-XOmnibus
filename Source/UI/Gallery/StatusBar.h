#pragma once
// StatusBar.h — XOlokun 28pt status bar strip at the bottom of the editor window.
//
// Layout (left → right):
//   [FIRE] [XOSEND] [ECHO CUT] [PANIC]  |  BPM · Voices · CPU  |  [●][●][●][●]  [🔒]
//
// Trigger pads fire performance actions via std::function callbacks.
// Status values (BPM, voice count, CPU) are pushed by XOlokunEditor::timerCallback().
// Slot dots are updated via setSlotActive() from the same timer.
//
// Keyboard shortcuts (Z/X/C/V) are intercepted via an inner KeyListener, which
// is registered with the editor (not the component itself) so shortcuts work
// regardless of which child has focus. KAI-P2-03 guard: shortcuts are suppressed
// when a TextEditor has keyboard focus.
//
// B041 Dark Cockpit exception: PANIC is always 100% opacity (never-dim list).

#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"
#include "ColumnLayoutManager.h"

namespace xolokun {

//==============================================================================
class StatusBar : public juce::Component
{
public:
    //==========================================================================
    // Public callbacks — wired to processor actions in XOlokunEditor constructor
    std::function<void()> onFire;
    std::function<void()> onXoSend;
    std::function<void()> onEchoCut;
    std::function<void()> onPanic;

    //==========================================================================
    StatusBar()
    {
        // ── Trigger pads ──────────────────────────────────────────────────────
        auto makePad = [&](juce::TextButton& btn,
                           const juce::String& label,
                           const juce::String& tooltip,
                           juce::Colour fillCol)
        {
            btn.setButtonText(label);
            btn.setTooltip(tooltip);
            btn.setColour(juce::TextButton::buttonColourId,   fillCol);
            btn.setColour(juce::TextButton::buttonOnColourId, fillCol.brighter(0.15f));
            btn.setColour(juce::TextButton::textColourOffId,  juce::Colours::black.withAlpha(0.85f));
            btn.setColour(juce::TextButton::textColourOnId,   juce::Colours::black.withAlpha(0.85f));
            addAndMakeVisible(btn);
        };

        // FIRE — green (#4ADE80)
        makePad(fireBtn,    "FIRE",     "Fire chord machine one-shot (Z)",   juce::Colour(0xFF4ADE80));
        // XOSEND — warm amber
        makePad(xoSendBtn,  "XOSEND",  "Trigger coupling burst (X)",        juce::Colour(0xFFF5C97A));
        // ECHO CUT — warm amber
        makePad(echoCutBtn, "ECHO CUT","Kill delay tails (C)",              juce::Colour(0xFFF5C97A));
        // PANIC — red, always 100% opacity (B041 never-dim)
        makePad(panicBtn,   "PANIC",   "All notes off / reset engines (V)", juce::Colour(0xFFEF4444));

        fireBtn.onClick    = [this] { if (onFire)    onFire();    };
        xoSendBtn.onClick  = [this] { if (onXoSend)  onXoSend();  };
        echoCutBtn.onClick = [this] { if (onEchoCut) onEchoCut(); };
        panicBtn.onClick   = [this] { if (onPanic)   onPanic();   };

        // ── Status labels ──────────────────────────────────────────────────────
        auto makeLabel = [&](juce::Label& lbl, const juce::String& text)
        {
            lbl.setText(text, juce::dontSendNotification);
            lbl.setFont(GalleryFonts::value(10.0f));
            lbl.setColour(juce::Label::textColourId,
                          GalleryColors::get(GalleryColors::textMid()));
            lbl.setJustificationType(juce::Justification::centred);
            lbl.setInterceptsMouseClicks(false, false);
            addAndMakeVisible(lbl);
        };

        makeLabel(bpmLabel,    "120 BPM");
        makeLabel(voiceLabel,  "Voices: 0");
        makeLabel(cpuLabel,    "CPU: 0%");

        // ── Performance Lock icon ─────────────────────────────────────────────
        lockBtn.setButtonText(juce::CharPointer_UTF8("\xf0\x9f\x94\x92")); // UTF-8 padlock
        lockBtn.setTooltip("Performance Lock — block parameter changes during performance");
        lockBtn.setClickingTogglesState(true);
        lockBtn.setColour(juce::TextButton::buttonColourId,
                          GalleryColors::get(GalleryColors::emptySlot()));
        lockBtn.setColour(juce::TextButton::buttonOnColourId,
                          GalleryColors::get(GalleryColors::xoGold));
        addAndMakeVisible(lockBtn);

        // Slot dots are drawn in paint() from slotAccents[] / slotActive[].
        // No child components needed for the dots — pure paint geometry.
    }

    //==========================================================================
    // Status update API — called from XOlokunEditor::timerCallback()

    void setVoiceCount(int count)
    {
        juce::String txt = "Voices: " + juce::String(count);
        if (voiceLabel.getText() != txt)
            voiceLabel.setText(txt, juce::dontSendNotification);
    }

    void setCpuPercent(float pct)
    {
        juce::String txt = "CPU: " + juce::String(juce::roundToInt(pct)) + "%";
        if (cpuLabel.getText() != txt)
            cpuLabel.setText(txt, juce::dontSendNotification);
    }

    void setBpm(double bpm)
    {
        juce::String txt = juce::String(bpm, 1) + " BPM";
        if (bpmLabel.getText() != txt)
            bpmLabel.setText(txt, juce::dontSendNotification);
    }

    // slot: 0-3. accent is the engine colour when active, or ignored when inactive.
    void setSlotActive(int slot, bool active, juce::Colour accent)
    {
        if (slot < 0 || slot >= kNumSlots) return;
        if (slotActive[slot] == active && slotAccents[slot] == accent) return;
        slotActive[slot]  = active;
        slotAccents[slot] = accent;
        repaint();
    }

    //==========================================================================
    void paint(juce::Graphics& g) override
    {
        using namespace GalleryColors;

        // Background — slightly darker than shell
        auto bg = get(shellWhite()).darker(0.04f);
        g.fillAll(bg);

        // Top border — 1px borderGray at 40% alpha
        g.setColour(get(borderGray()).withAlpha(0.40f));
        g.drawHorizontalLine(0, 0.0f, (float)getWidth());

        // ── Slot dots (right side, before lock button) ──────────────────────
        // Drawn in paint() because they are tiny geometry, not interactive children.
        const int dotR       = 5;
        const int dotDia     = dotR * 2;
        const int dotSpacing = 4;
        const int dotsWidth  = kNumSlots * dotDia + (kNumSlots - 1) * dotSpacing;
        const int lockW      = 22;
        const int lockPad    = 6;
        int dotsRight = getWidth() - lockPad - lockW - lockPad - 2;
        int dotsLeft  = dotsRight - dotsWidth;
        int dotY      = getHeight() / 2;

        for (int i = 0; i < kNumSlots; ++i)
        {
            int cx = dotsLeft + i * (dotDia + dotSpacing) + dotR;
            juce::Colour dotColour = slotActive[i]
                ? slotAccents[i]
                : get(emptySlot());

            g.setColour(dotColour);
            g.fillEllipse((float)(cx - dotR), (float)(dotY - dotR),
                          (float)dotDia, (float)dotDia);

            // Subtle ring around active dots for clarity on bright accents
            if (slotActive[i])
            {
                g.setColour(dotColour.darker(0.3f).withAlpha(0.6f));
                g.drawEllipse((float)(cx - dotR), (float)(dotY - dotR),
                              (float)dotDia, (float)dotDia, 1.0f);
            }
        }
    }

    void resized() override
    {
        // ── Left: trigger pads ────────────────────────────────────────────────
        const int padW   = 52;
        const int padH   = 20;
        const int padGap = 4;
        const int padTop = (getHeight() - padH) / 2;

        int x = 6;
        auto placePad = [&](juce::TextButton& btn) {
            btn.setBounds(x, padTop, padW, padH);
            x += padW + padGap;
        };
        placePad(fireBtn);
        placePad(xoSendBtn);
        placePad(echoCutBtn);
        placePad(panicBtn);

        // ── Right: lock button ───────────────────────────────────────────────
        const int lockW   = 22;
        const int lockH   = 18;
        const int lockPad = 6;
        lockBtn.setBounds(getWidth() - lockPad - lockW, (getHeight() - lockH) / 2, lockW, lockH);

        // ── Centre: status labels ─────────────────────────────────────────────
        // Slot dots are in paint(); labels fill the gap between pads and dots.
        const int dotDia      = 10;
        const int dotSpacing2 = 4;
        const int dotsWidth   = kNumSlots * dotDia + (kNumSlots - 1) * dotSpacing2;
        int centreLeft  = x + 6;
        int centreRight = getWidth() - lockPad - lockW - lockPad - dotsWidth - lockPad;

        int centreW = centreRight - centreLeft;
        int labelW  = centreW / 3;
        int labelH  = getHeight();
        bpmLabel.setBounds(   centreLeft,              0, labelW, labelH);
        voiceLabel.setBounds( centreLeft + labelW,     0, labelW, labelH);
        cpuLabel.setBounds(   centreLeft + labelW * 2, 0, labelW, labelH);
    }

    //==========================================================================
    // Inner KeyListener — registered with the editor so Z/X/C/V fire anywhere
    // in the window without needing this component to have keyboard focus.
    //
    // KAI-P2-03: shortcuts are suppressed when a TextEditor has keyboard focus.
    //
    // Using an inner struct keeps the KeyListener virtual table separate from
    // juce::Component, avoiding the -Woverloaded-virtual warning that arises
    // when both base classes declare `keyPressed` with different signatures.

    struct ShortcutListener : public juce::KeyListener
    {
        explicit ShortcutListener(StatusBar& owner) : bar(owner) {}

        bool keyPressed(const juce::KeyPress& key,
                        juce::Component* /*originatingComponent*/) override
        {
            // Don't fire triggers when a text input field has focus (KAI-P2-03)
            if (auto* focused = juce::Component::getCurrentlyFocusedComponent())
                if (dynamic_cast<juce::TextEditor*>(focused) != nullptr)
                    return false;

            if (key == juce::KeyPress('z')) { if (bar.onFire)    bar.onFire();    return true; }
            if (key == juce::KeyPress('x')) { if (bar.onXoSend)  bar.onXoSend();  return true; }
            if (key == juce::KeyPress('c')) { if (bar.onEchoCut) bar.onEchoCut(); return true; }
            if (key == juce::KeyPress('v')) { if (bar.onPanic)   bar.onPanic();   return true; }
            return false;
        }

        bool keyStateChanged(bool, juce::Component*) override { return false; }

        StatusBar& bar;
    };

    // Returns the KeyListener to register/remove with the editor.
    juce::KeyListener* getKeyListener() { return &shortcutListener; }

private:
    static constexpr int kNumSlots = XOlokunProcessor::MaxSlots; // 5 (4 primary + Ghost Slot)

    // ── Trigger pad buttons ──────────────────────────────────────────────────
    juce::TextButton fireBtn;
    juce::TextButton xoSendBtn;
    juce::TextButton echoCutBtn;
    juce::TextButton panicBtn;

    // ── Status labels (centre) ───────────────────────────────────────────────
    juce::Label bpmLabel;
    juce::Label voiceLabel;
    juce::Label cpuLabel;

    // ── Slot dot state ───────────────────────────────────────────────────────
    std::array<bool,         kNumSlots> slotActive  = {};
    std::array<juce::Colour, kNumSlots> slotAccents = { juce::Colours::transparentBlack,
                                                         juce::Colours::transparentBlack,
                                                         juce::Colours::transparentBlack,
                                                         juce::Colours::transparentBlack,
                                                         juce::Colours::transparentBlack };

    // ── Performance lock toggle ──────────────────────────────────────────────
    juce::TextButton lockBtn;

    // ── Keyboard shortcut listener ───────────────────────────────────────────
    ShortcutListener shortcutListener { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusBar)
};

} // namespace xolokun
