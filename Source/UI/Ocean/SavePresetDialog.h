// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Core/PresetManager.h"
#include "../../Core/PresetTaxonomy.h"
#include "../Tokens.h"
#include "../GalleryColors.h"

namespace xoceanus
{

//==============================================================================
// MoodDropdownLookAndFeel — custom L&F for the mood ComboBox.
// Renders a 12×12px filled colour circle preceding the mood name in each item.
// Reusable in future components that need mood-pill display (#1428 ChainMatrix, etc.)
//==============================================================================
class MoodDropdownLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MoodDropdownLookAndFeel() = default;

    /** Returns the canonical swatch colour for a mood name string. */
    static juce::Colour moodSwatchColour (const juce::String& moodName)
    {
        // Per-mood accent colours calibrated to the Ocean depth palette.
        // Derived from the engine accent table + mood category semantics.
        if (moodName == "Foundation")  return juce::Colour (0xFF4A7090);
        if (moodName == "Atmosphere")  return juce::Colour (0xFF6AB0C8);
        if (moodName == "Entangled")   return juce::Colour (0xFF7B2FBE);
        if (moodName == "Prism")       return juce::Colour (0xFF48CAE4);
        if (moodName == "Flux")        return juce::Colour (0xFFE9C46A);
        if (moodName == "Aether")      return juce::Colour (0xFFC0A8E0);
        if (moodName == "Family")      return juce::Colour (0xFF7DD3A0);
        if (moodName == "Submerged")   return juce::Colour (0xFF0096C7);
        if (moodName == "Coupling")    return juce::Colour (0xFFE89B4A);
        if (moodName == "Crystalline") return juce::Colour (0xFFB8E8F8);
        if (moodName == "Deep")        return juce::Colour (0xFF04040A).brighter (0.4f);
        if (moodName == "Ethereal")    return juce::Colour (0xFFD8C8F0);
        if (moodName == "Kinetic")     return juce::Colour (0xFFFF6B4A);
        if (moodName == "Luminous")    return juce::Colour (0xFFFFE08A);
        if (moodName == "Organic")     return juce::Colour (0xFF88C870);
        if (moodName == "Shadow")      return juce::Colour (0xFF505870);
        return juce::Colour (0xFF808080); // fallback
    }

    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu,
                            const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override
    {
        if (isSeparator || ! isActive)
        {
            juce::LookAndFeel_V4::drawPopupMenuItem (g, area, isSeparator, isActive,
                isHighlighted, isTicked, hasSubMenu, text, shortcutKeyText, icon, textColour);
            return;
        }

        if (isHighlighted)
            g.fillAll (findColour (juce::PopupMenu::highlightedBackgroundColourId));

        // Colour swatch — 12×12px circle, vertically centred
        auto swatchBounds = juce::Rectangle<float> (
            (float) area.getX() + 6.0f,
            (float) area.getCentreY() - 6.0f,
            12.0f, 12.0f);
        g.setColour (moodSwatchColour (text));
        g.fillEllipse (swatchBounds);

        // Label text
        const juce::Colour labelColour = isHighlighted
            ? findColour (juce::PopupMenu::highlightedTextColourId)
            : findColour (juce::PopupMenu::textColourId);
        g.setColour (labelColour);
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (13.0f)));
        g.drawText (text, area.withTrimmedLeft (26), juce::Justification::centredLeft);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoodDropdownLookAndFeel)
};

//==============================================================================
// SavePresetDialog — modal form for saving / save-as operations.
//
// Required fields: Name + Mood (Save button disabled until both filled).
// Optional fields: Category, Tags (comma-separated), Description.
//
// Usage:
//   auto* dialog = new SavePresetDialog (initial, isFirstSave, onSave, onCancel);
//   juce::DialogWindow::LaunchOptions opts;
//   opts.content.setOwned (dialog);
//   opts.dialogTitle = isFirstSave ? "Save Preset" : "Save Preset As...";
//   opts.dialogBackgroundColour = XO::Tokens::Color::surface();
//   opts.escapeKeyTriggersCloseButton = true;
//   opts.useNativeTitleBar = false;
//   opts.resizable = false;
//   opts.launchAsync();
//==============================================================================
class SavePresetDialog : public juce::Component
{
public:
    using SaveCallback   = std::function<void (PresetData)>;
    using CancelCallback = std::function<void()>;

    SavePresetDialog (PresetData initial,
                      bool       /*isFirstSave*/,
                      SaveCallback   onSave,
                      CancelCallback onCancel)
        : initial_  (std::move (initial))
        , onSave_   (std::move (onSave))
        , onCancel_ (std::move (onCancel))
    {
        // Apply submarine colour scheme to all children
        auto fgColour  = XO::Tokens::Color::text();
        auto fieldBg   = juce::Colour (GalleryColors::Ocean::twilight);
        auto borderCol = XO::Tokens::Color::accent();

        // ── Name field ──────────────────────────────────────────────────────────
        addAndMakeVisible (nameLabel_);
        nameLabel_.setText ("Name *", juce::dontSendNotification);
        nameLabel_.setColour (juce::Label::textColourId, fgColour);

        addAndMakeVisible (nameField_);
        nameField_.setText (initial_.name, juce::dontSendNotification);
        nameField_.setSelectAllWhenFocused (true);
        nameField_.setColour (juce::TextEditor::backgroundColourId, fieldBg);
        nameField_.setColour (juce::TextEditor::textColourId, fgColour);
        nameField_.setColour (juce::TextEditor::outlineColourId, borderCol);
        nameField_.onTextChange = [this] { updateSaveButtonState(); };

        // ── Mood dropdown ───────────────────────────────────────────────────────
        addAndMakeVisible (moodLabel_);
        moodLabel_.setText ("Mood *", juce::dontSendNotification);
        moodLabel_.setColour (juce::Label::textColourId, fgColour);

        addAndMakeVisible (moodDropdown_);
        moodDropdown_.setLookAndFeel (&moodLAF_);
        moodDropdown_.setColour (juce::ComboBox::backgroundColourId, fieldBg);
        moodDropdown_.setColour (juce::ComboBox::textColourId, fgColour);
        moodDropdown_.setColour (juce::ComboBox::outlineColourId, borderCol);

        const juce::StringArray moods {
            "Aether", "Atmosphere", "Coupling", "Crystalline", "Deep", "Entangled",
            "Ethereal", "Family", "Flux", "Foundation", "Kinetic", "Luminous",
            "Organic", "Prism", "Shadow", "Submerged"
        };
        for (int i = 0; i < moods.size(); ++i)
            moodDropdown_.addItem (moods[i], i + 1);

        moodDropdown_.setText (initial_.mood, juce::dontSendNotification);
        moodDropdown_.onChange = [this] { updateSaveButtonState(); };

        // ── Category dropdown ───────────────────────────────────────────────────
        addAndMakeVisible (categoryLabel_);
        categoryLabel_.setText ("Category", juce::dontSendNotification);
        categoryLabel_.setColour (juce::Label::textColourId, fgColour);

        addAndMakeVisible (categoryDropdown_);
        categoryDropdown_.setColour (juce::ComboBox::backgroundColourId, fieldBg);
        categoryDropdown_.setColour (juce::ComboBox::textColourId, fgColour);
        categoryDropdown_.setColour (juce::ComboBox::outlineColourId, borderCol);
        categoryDropdown_.addItem ("— None —", 1);
        for (int i = 0; i < (int) xoceanus::kPresetCategories.size(); ++i)
            categoryDropdown_.addItem (juce::String (xoceanus::kPresetCategories[(size_t) i]), i + 2);

        {
            juce::String catText = "— None —";
            if (initial_.category.has_value() && initial_.category->isNotEmpty())
                catText = *initial_.category;
            categoryDropdown_.setText (catText, juce::dontSendNotification);
        }

        // ── Tags field ──────────────────────────────────────────────────────────
        addAndMakeVisible (tagsLabel_);
        tagsLabel_.setText ("Tags  (comma-separated)", juce::dontSendNotification);
        tagsLabel_.setColour (juce::Label::textColourId, fgColour);

        addAndMakeVisible (tagsField_);
        tagsField_.setColour (juce::TextEditor::backgroundColourId, fieldBg);
        tagsField_.setColour (juce::TextEditor::textColourId, fgColour);
        tagsField_.setColour (juce::TextEditor::outlineColourId, borderCol);
        tagsField_.setText (initial_.tags.joinIntoString (", "), juce::dontSendNotification);

        // ── Description field ───────────────────────────────────────────────────
        addAndMakeVisible (descriptionLabel_);
        descriptionLabel_.setText ("Description", juce::dontSendNotification);
        descriptionLabel_.setColour (juce::Label::textColourId, fgColour);

        addAndMakeVisible (descriptionField_);
        descriptionField_.setMultiLine (true);
        descriptionField_.setReturnKeyStartsNewLine (true);
        descriptionField_.setColour (juce::TextEditor::backgroundColourId, fieldBg);
        descriptionField_.setColour (juce::TextEditor::textColourId, fgColour);
        descriptionField_.setColour (juce::TextEditor::outlineColourId, borderCol);
        descriptionField_.setText (initial_.description, juce::dontSendNotification);

        // ── Buttons ─────────────────────────────────────────────────────────────
        addAndMakeVisible (saveButton_);
        saveButton_.setButtonText ("Save");
        saveButton_.setColour (juce::TextButton::buttonColourId, XO::Tokens::Color::accent());
        saveButton_.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
        saveButton_.onClick = [this] { handleSaveClicked(); };

        addAndMakeVisible (cancelButton_);
        cancelButton_.setButtonText ("Cancel");
        cancelButton_.setColour (juce::TextButton::buttonColourId, fieldBg);
        cancelButton_.setColour (juce::TextButton::textColourOffId, fgColour);
        cancelButton_.onClick = [this] { if (onCancel_) onCancel_(); };

        updateSaveButtonState();
        setSize (480, 380);
    }

    ~SavePresetDialog() override
    {
        moodDropdown_.setLookAndFeel (nullptr);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (XO::Tokens::Color::surface());

        // Depth-ring border
        g.setColour (XO::Tokens::Color::accent().withAlpha (0.6f));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 4.0f, 1.5f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (24);

        // Button row at bottom
        auto buttonRow = bounds.removeFromBottom (36);
        bounds.removeFromBottom (12);

        cancelButton_.setBounds (buttonRow.removeFromRight (100));
        buttonRow.removeFromRight (8);
        saveButton_.setBounds (buttonRow.removeFromRight (100));

        // Helper: label above field
        auto layoutLabeled = [&] (juce::Label& lbl, juce::Component& field, int fieldHeight) {
            auto row = bounds.removeFromTop (16);
            lbl.setBounds (row);
            bounds.removeFromTop (2);
            field.setBounds (bounds.removeFromTop (fieldHeight));
            bounds.removeFromTop (10);
        };

        layoutLabeled (nameLabel_,        nameField_,         28);
        layoutLabeled (moodLabel_,        moodDropdown_,      28);
        layoutLabeled (categoryLabel_,    categoryDropdown_,  28);
        layoutLabeled (tagsLabel_,        tagsField_,         28);
        layoutLabeled (descriptionLabel_, descriptionField_,  64);
    }

private:
    void updateSaveButtonState()
    {
        const bool nameOK = nameField_.getText().trim().isNotEmpty();
        const bool moodOK = moodDropdown_.getSelectedId() > 0;
        saveButton_.setEnabled (nameOK && moodOK);
    }

    void handleSaveClicked()
    {
        if (! onSave_) return;

        PresetData out = initial_;
        out.name = juce::File::createLegalFileName (nameField_.getText().trim());
        out.mood = moodDropdown_.getText();

        const auto catText = categoryDropdown_.getText();
        out.category = (catText.isEmpty() || catText == "— None —")
                           ? std::optional<juce::String>{}
                           : std::optional<juce::String>{ catText };

        out.tags = juce::StringArray::fromTokens (tagsField_.getText(), ",", "\"");
        out.tags.trim();
        out.tags.removeEmptyStrings();

        out.description = descriptionField_.getText();

        auto presetDir = PresetManager::getUserPresetDirectory();
        auto target    = presetDir.getChildFile (out.name + ".xometa");

        auto confirmCallback = [this, name = out.name] (juce::File) -> bool {
            return juce::AlertWindow::showOkCancelBox (
                juce::MessageBoxIconType::QuestionIcon,
                "Overwrite preset?",
                "A preset named \"" + name + "\" already exists.\n\nOverwrite it?",
                "Overwrite", "Cancel", this, nullptr);
        };

        PresetManager pm;
        const bool saved = pm.savePresetToFile (target, out, confirmCallback);
        if (saved)
            onSave_ (out); // closes dialog via caller
        // else: user cancelled overwrite — dialog stays open with fields preserved
    }

    // ── Data ──────────────────────────────────────────────────────────────────
    PresetData     initial_;
    SaveCallback   onSave_;
    CancelCallback onCancel_;

    // ── L&F ───────────────────────────────────────────────────────────────────
    MoodDropdownLookAndFeel moodLAF_;

    // ── Controls ──────────────────────────────────────────────────────────────
    juce::Label      nameLabel_,  moodLabel_,  categoryLabel_,
                     tagsLabel_,  descriptionLabel_;

    juce::TextEditor nameField_;
    juce::ComboBox   moodDropdown_;
    juce::ComboBox   categoryDropdown_;
    juce::TextEditor tagsField_;
    juce::TextEditor descriptionField_;

    juce::TextButton saveButton_;
    juce::TextButton cancelButton_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SavePresetDialog)
};

} // namespace xoceanus
