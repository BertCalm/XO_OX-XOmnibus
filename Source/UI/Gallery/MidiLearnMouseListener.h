// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../GalleryColors.h"
#include "../../Core/MIDILearnManager.h"

namespace xoceanus
{

//==============================================================================
// MidiLearnMouseListener — right-click context menu for MIDI Learn on any Slider.
//
// Attach to any juce::Slider via addMouseListener().  Intercepts right-click and
// shows a context menu: "MIDI Learn", optional "Mapped: CC N", "Clear", "Cancel".
// Delegates to the shared MIDILearnManager — no audio-thread work here.
//
class MidiLearnMouseListener : public juce::MouseListener
{
public:
    MidiLearnMouseListener(juce::String pid, MIDILearnManager& mgr) : paramId(std::move(pid)), manager(mgr) {}

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (!e.mods.isRightButtonDown())
            return;

        bool isLearning = manager.isLearning() && manager.getLearningParam() == paramId;
        bool hasMapping = manager.hasMapping(paramId);

        juce::PopupMenu menu;
        if (isLearning)
            menu.addItem(1, "Listening for CC...  (click to cancel)", true, false);
        else
            menu.addItem(2, "MIDI Learn", true, false);

        if (hasMapping)
        {
            for (const auto& m : manager.getMappings())
            {
                if (m.paramId == paramId)
                {
                    juce::String info = "Mapped to CC " + juce::String(m.ccNumber);
                    if (m.channel != 0)
                        info += "  Ch " + juce::String(m.channel);
                    menu.addItem(3, info, false, false); // info label (non-selectable)
                    break;
                }
            }
            menu.addItem(4, "Clear MIDI Mapping", true, false);
        }
        menu.addSeparator();
        menu.addItem(5, "Cancel", true, false);

        // Capture state for the async callback — no reference captures (lifetime unsafe)
        juce::String pid = paramId;
        MIDILearnManager* mgr = &manager;
        juce::Component::SafePointer<juce::Component> safeComp(e.eventComponent);

        menu.showMenuAsync(
            juce::PopupMenu::Options().withTargetScreenArea(juce::Rectangle<int>(e.getScreenX(), e.getScreenY(), 1, 1)),
            [pid, mgr, safeComp](int result)
            {
                if (result == 2) // MIDI Learn
                {
                    mgr->enterLearnMode(pid);
                    if (safeComp != nullptr)
                        safeComp->repaint();
                }
                else if (result == 1) // cancel while listening
                {
                    mgr->exitLearnMode();
                    if (safeComp != nullptr)
                        safeComp->repaint();
                }
                else if (result == 4) // clear
                {
                    mgr->removeMappingForParam(pid);
                    if (safeComp != nullptr)
                        safeComp->repaint();
                }
            });
    }

private:
    juce::String paramId;
    MIDILearnManager& manager;
    JUCE_DECLARE_NON_COPYABLE(MidiLearnMouseListener)
};

//==============================================================================
// attachMidiLearn — attach MIDI Learn right-click to a plain juce::Slider.
// Use for array-member sliders (MacroSection, MasterFXSection) that cannot
// be changed to MidiLearnSlider.  Caller stores the returned pointer.
//
inline MidiLearnMouseListener* attachMidiLearn(juce::Slider& slider, const juce::String& paramId, MIDILearnManager& mgr)
{
    auto* ml = new MidiLearnMouseListener(paramId, mgr);
    slider.addMouseListener(ml, false);
    return ml;
}

} // namespace xoceanus
