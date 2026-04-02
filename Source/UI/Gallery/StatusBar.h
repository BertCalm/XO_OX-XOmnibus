#pragma once
// StatusBar.h — XOceanus 28pt status bar strip at the bottom of the editor window.
//
// Layout (left → right):
//   [FIRE] [XOSEND] [ECHO CUT] [PANIC]  |  BPM · Voices · CPU  |  [●][●][●][●]  [🔒]
//
// Trigger pads fire performance actions via std::function callbacks.
// Status values (BPM, voice count, CPU) are pushed by XOceanusEditor::timerCallback().
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

namespace xoceanus {

//==============================================================================
class StatusBar : public juce::Component
{
public:
    //==========================================================================
    // Public callbacks — wired to processor actions in XOceanusEditor constructor
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
                           const juce::String& tooltip)
        {
            btn.setButtonText(label);
            btn.setName(label);
            btn.setTooltip(tooltip);
            addAndMakeVisible(btn);
        };

        // Trigger pads hidden from status bar — they belong on PlaySurface pop-out.
        // Callbacks (onFire, onXoSend, onEchoCut, onPanic) remain wired for
        // keyboard shortcuts (Z/X/C/V) and future PlaySurface integration.
        makePad(fireBtn,    "FIRE",     "Fire chord machine (Z)");
        makePad(xoSendBtn,  "XOSEND",  "Trigger coupling burst (X)");
        makePad(echoCutBtn, "ECHO CUT","Kill delay tails (C)");
        makePad(panicBtn,   "PANIC",   "All notes off / reset engines (V)");

        fireBtn.setVisible(false);
        xoSendBtn.setVisible(false);
        echoCutBtn.setVisible(false);
        panicBtn.setVisible(false);

        fireBtn.setEnabled(true);
        xoSendBtn.setEnabled(true);
        echoCutBtn.setEnabled(true);

        fireBtn.onClick    = [this] { if (onFire)    onFire();    };
        xoSendBtn.onClick  = [this] { if (onXoSend)  onXoSend();  };
        echoCutBtn.onClick = [this] { if (onEchoCut) onEchoCut(); };
        panicBtn.onClick   = [this] { if (onPanic)   onPanic();   };

        // ── Status labels ──────────────────────────────────────────────────────
        auto makeLabel = [&](juce::Label& lbl, const juce::String& text)
        {
            lbl.setText(text, juce::dontSendNotification);
            lbl.setFont(GalleryFonts::value(10.0f));
            lbl.setJustificationType(juce::Justification::centred);
            lbl.setInterceptsMouseClicks(false, false);
            addAndMakeVisible(lbl);
        };

        makeLabel(bpmLabel,    "120 BPM");
        makeLabel(voiceLabel,  "Voices: 0");
        makeLabel(cpuLabel,    "CPU: 0%");

        // ── Performance Lock icon ─────────────────────────────────────────────
        lockBtn.setButtonText("LK"); // plain ASCII — reliable cross-platform rendering
        lockBtn.setTooltip("Performance Lock — block parameter changes during performance");
        lockBtn.setClickingTogglesState(true);
        addAndMakeVisible(lockBtn);

        // Slot dots are drawn in paint() from slotAccents[] / slotActive[].
        // No child components needed for the dots — pure paint geometry.

        // Apply initial theme colours (moved out of paint() to avoid repaint cascades).
        applyTheme();
    }

    //==========================================================================
    // applyTheme() — sets all child component colours once.
    // Called from constructor and lookAndFeelChanged() so that colour updates
    // happen at the right time without triggering repaint() inside paint().
    void applyTheme()
    {
        using namespace GalleryColors;

        // ── Trigger pad buttons ───────────────────────────────────────────────
        // Prototype: elevated bg, border at 7%, 3px radius

        // FIRE button style — teal accent-bright (rgba 30,139,126 @ 70%)
        fireBtn.setColour(juce::TextButton::buttonColourId,   get(elevated()));
        fireBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(30, 139, 126).withAlpha(0.85f));
        fireBtn.setColour(juce::TextButton::textColourOffId,  juce::Colour(30, 139, 126).withAlpha(0.70f));
        fireBtn.setColour(juce::TextButton::textColourOnId,   get(surface()));

        // XOSEND button — T3 text default, T1 on active
        xoSendBtn.setColour(juce::TextButton::buttonColourId,   get(elevated()));
        xoSendBtn.setColour(juce::TextButton::buttonOnColourId, get(t1()).withAlpha(0.25f));
        xoSendBtn.setColour(juce::TextButton::textColourOffId,  get(t3()));
        xoSendBtn.setColour(juce::TextButton::textColourOnId,   get(t1()));

        // ECHO CUT button — T3 text default, T1 on active
        echoCutBtn.setColour(juce::TextButton::buttonColourId,   get(elevated()));
        echoCutBtn.setColour(juce::TextButton::buttonOnColourId, get(t1()).withAlpha(0.25f));
        echoCutBtn.setColour(juce::TextButton::textColourOffId,  get(t3()));
        echoCutBtn.setColour(juce::TextButton::textColourOnId,   get(t1()));

        // PANIC button — #FF6B6B red (B041: always 100% opacity, never-dim)
        panicBtn.setColour(juce::TextButton::buttonColourId,   get(elevated()));
        panicBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFFFF6B6B).withAlpha(0.85f));
        panicBtn.setColour(juce::TextButton::textColourOffId,  juce::Colour(0xFFFF6B6B));
        panicBtn.setColour(juce::TextButton::textColourOnId,   juce::Colour(0xFFFFFFFF));

        // ── Status label colours ──────────────────────────────────────────────
        bpmLabel.setColour(juce::Label::textColourId,   get(t2())); // T2
        voiceLabel.setColour(juce::Label::textColourId, get(t3())); // T3
        cpuLabel.setColour(juce::Label::textColourId,   get(t3())); // T3

        // ── Lock button ───────────────────────────────────────────────────────
        lockBtn.setColour(juce::TextButton::buttonColourId,   get(emptySlot()));
        lockBtn.setColour(juce::TextButton::buttonOnColourId, get(xoGold));
    }

    //==========================================================================
    // Status update API — called from XOceanusEditor::timerCallback()

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

    // setCpuVisible() — show or hide the CPU label + update.
    // Called from XOceanusEditor when the "CPU Meters" settings toggle changes.
    void setCpuVisible(bool visible)
    {
        cpuLabel.setVisible(visible);
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

        // ── Background — surface layer (GalleryColors::surface()) ───────────
        // Prototype: status bar shares the surface color with header and tab bar
        g.setColour(get(surface()));
        g.fillRect(getLocalBounds());

        // ── Top border — border() = rgba(255,255,255,0.07) layer separator ───
        g.setColour(border());
        g.drawHorizontalLine(0, 0.0f, (float)getWidth());

        // ── Slot dots (right side, before lock button) — paint() is READ-ONLY ──
        // Prototype: 7×7px dots, T4 default, accent when active with 0 0 5px glow
        const float dotR       = 3.5f; // 7px diameter
        const float dotDia     = dotR * 2.0f;
        const int   dotSpacing = 4;
        const int   dotsWidth  = juce::roundToInt((float)kNumSlots * dotDia) + (kNumSlots - 1) * dotSpacing;
        const int lockW      = 22;
        const int lockPad    = 6;
        int dotsRight = getWidth() - lockPad - lockW - lockPad - 2;
        int dotsLeft  = dotsRight - dotsWidth;
        float dotY    = (float)(getHeight() / 2);

        for (int i = 0; i < kNumSlots; ++i)
        {
            float cx = (float)dotsLeft + (float)i * (dotDia + (float)dotSpacing) + dotR;

            if (slotActive[i])
            {
                // Active: accent color with soft glow layers (simulates box-shadow 0 0 5px accent)
                juce::Colour ac = slotAccents[i];
                // Glow rings behind the dot
                for (int gx = 3; gx >= 1; --gx)
                {
                    float glowAlpha = 0.08f / (float)gx;
                    g.setColour(ac.withAlpha(glowAlpha));
                    float expand = (float)gx;
                    g.fillEllipse(cx - dotR - expand,
                                  dotY - dotR - expand,
                                  dotDia + expand * 2.0f,
                                  dotDia + expand * 2.0f);
                }
                // Core dot
                g.setColour(ac);
                g.fillEllipse(cx - dotR, dotY - dotR, dotDia, dotDia);
            }
            else
            {
                // Inactive: T4 color (GalleryColors::t4())
                g.setColour(get(t4()));
                g.fillEllipse(cx - dotR, dotY - dotR, dotDia, dotDia);
            }
        }

        // ── Cockpit bypass indicator — small text badge left of slot dots ─────
        {
            juce::String badge = cockpitBypassed_ ? "COCKPIT: OFF" : "COCKPIT: ON";
            juce::Colour badgeColour = cockpitBypassed_
                ? juce::Colour(0xFFFF6B6B)          // red when bypassed (full bright)
                : get(t3());                          // T3 gray when active (dimming on)
            g.setColour(badgeColour);
            g.setFont(GalleryFonts::value(8.0f));
            const int badgeW = 68;
            const int badgeX = dotsLeft - badgeW - 6;
            g.drawText(badge, badgeX, 0, badgeW, getHeight(), juce::Justification::centredRight);
        }

    }

    void resized() override
    {
        // ── Trigger pads hidden (belong on PlaySurface pop-out) ────────────────
        // Buttons are off-screen; callbacks still fire via keyboard shortcuts.

        // ── Right: lock button ───────────────────────────────────────────────
        const int lockW   = 22;
        const int lockH   = 18;
        const int lockPad = 6;
        lockBtn.setBounds(getWidth() - lockPad - lockW, (getHeight() - lockH) / 2, lockW, lockH);

        // ── Left: status labels (now start from left edge) ───────────────────
        const int dotDia      = 10;
        const int dotSpacing2 = 4;
        const int dotsWidth   = kNumSlots * dotDia + (kNumSlots - 1) * dotSpacing2;
        int centreLeft  = 12; // left padding
        int centreRight = getWidth() - lockPad - lockW - lockPad - dotsWidth - lockPad;

        int centreW = centreRight - centreLeft;
        int labelW  = centreW / 3;
        int labelH  = getHeight();
        bpmLabel.setBounds(   centreLeft,              0, labelW, labelH);
        voiceLabel.setBounds( centreLeft + labelW,     0, labelW, labelH);
        cpuLabel.setBounds(   centreLeft + labelW * 2, 0, labelW, labelH);
    }

    //==========================================================================
    // setLocked() — sync the lock button's toggle state from an external source
    // (e.g. the Settings panel's onPerformanceLockChanged callback).
    // Must be called on the message thread.
    void setLocked(bool locked)
    {
        if (lockBtn.getToggleState() != locked)
            lockBtn.setToggleState(locked, juce::dontSendNotification);
    }

    bool isLocked() const noexcept { return lockBtn.getToggleState(); }

    // setCockpitBypass() — show/hide a small COCKPIT: ON/OFF badge in paint().
    // Called from XOceanusEditor::timerCallback() (and from the 'B' keypress handler
    // via repaint trigger) so the indicator stays in sync.
    void setCockpitBypass(bool bypassed)
    {
        if (cockpitBypassed_ == bypassed) return;
        cockpitBypassed_ = bypassed;
        repaint();
    }

    // Re-apply theme colours when the LookAndFeel changes (avoids colour drift
    // and keeps setColour() calls out of paint() where they would trigger
    // recursive repaint cascades).
    void lookAndFeelChanged() override { applyTheme(); }

    ~StatusBar() override
    {
        // L-NEW-04: Remove the shortcut listener before this object is destroyed.
        // Guards against UAF if the owning editor forgets to call removeKeyListener
        // or if StatusBar is destroyed before the editor's destructor runs.
        if (keyListenerOwner != nullptr)
        {
            keyListenerOwner->removeKeyListener(&shortcutListener);
            keyListenerOwner = nullptr;
        }
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
            // Don't fire triggers when a text input field or combo box has focus (KAI-P2-03)
            auto* focused = juce::Component::getCurrentlyFocusedComponent();
            if (dynamic_cast<juce::TextEditor*>(focused) != nullptr
             || dynamic_cast<juce::ComboBox*>(focused) != nullptr)
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

    // Register the shortcut listener with an editor component.
    // StatusBar's destructor will automatically call removeKeyListener, preventing UAF.
    void registerKeyListener(juce::Component* editorComponent)
    {
        jassert(keyListenerOwner == nullptr); // call once only
        keyListenerOwner = editorComponent;
        editorComponent->addKeyListener(&shortcutListener);
    }

    // Returns the KeyListener (for callers that manage registration themselves).
    juce::KeyListener* getKeyListener() { return &shortcutListener; }

private:
    static constexpr int kNumSlots = XOceanusProcessor::MaxSlots; // 5 (4 primary + Ghost Slot)

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

    // ── Cockpit bypass state ─────────────────────────────────────────────────
    bool cockpitBypassed_ = false;

    // ── Keyboard shortcut listener ───────────────────────────────────────────
    ShortcutListener   shortcutListener { *this };
    juce::Component*   keyListenerOwner { nullptr }; // set by registerKeyListener(); cleared in destructor (L-NEW-04)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusBar)
};

} // namespace xoceanus
