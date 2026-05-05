// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

// SavePresetDialog — rich modal dialog for Save / Save As preset workflow.
//
// Architecture: one new file (this), two modified (XOceanusEditor.h, PresetManager.h).
// Spec: Docs/plans/2026-05-04-save-as-design.md
// Tracking: issue #1405 (TODO #1354)
//
// Form: 480x360 modal Component hosted in a juce::DialogWindow.
// Required fields: Name + Mood. Optional: Category, Tags, Description.
// Reuses XO::Tokens::Color::* (D1-D5 locked tokens) — NO new tokens introduced.
//
// MoodDropdownLookAndFeel is exposed as a public nested class so future components
// (e.g. #1428 ChainMatrix mood filter) can reuse it.

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Core/PresetManager.h"
#include "../../Core/PresetTaxonomy.h"
#include "../Tokens.h"
#include "../GalleryColors.h"
#include "../ToastOverlay.h"

namespace xoceanus
{

//==============================================================================
/// Rich Save / Save As dialog for XOceanus presets.
/// Hosts Name (required), Mood (required), Category / Tags / Description (optional).
class SavePresetDialog : public juce::Component
{
public:
    using SaveCallback   = std::function<void(PresetData)>;
    using CancelCallback = std::function<void()>;

    //==========================================================================
    /// Custom LookAndFeel for the 16-mood ComboBox — renders 12x12 color swatches.
    /// Exposed as public so other components can reuse it (e.g. #1428 ChainMatrix).
    class MoodDropdownLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        MoodDropdownLookAndFeel()
        {
            // Match the submarine dark palette.
            setColour(juce::ComboBox::backgroundColourId,    juce::Colour(GalleryColors::Ocean::shallow));
            setColour(juce::ComboBox::outlineColourId,       juce::Colour(GalleryColors::Ocean::surface).brighter(0.3f));
            setColour(juce::ComboBox::textColourId,          juce::Colour(GalleryColors::Ocean::foam));
            setColour(juce::ComboBox::arrowColourId,         juce::Colour(GalleryColors::Ocean::salt));
            setColour(juce::PopupMenu::backgroundColourId,   juce::Colour(GalleryColors::Ocean::shallow));
            setColour(juce::PopupMenu::textColourId,         juce::Colour(GalleryColors::Ocean::foam));
            setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(GalleryColors::Ocean::surface).brighter(0.2f));
            setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(GalleryColors::Ocean::foam));
        }

        void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                               bool isSeparator, bool isActive, bool isHighlighted,
                               bool isTicked, bool hasSubMenu,
                               const juce::String& text, const juce::String& shortcutKeyText,
                               const juce::Drawable* icon, const juce::Colour* textColour) override
        {
            if (isSeparator || !isActive)
            {
                juce::LookAndFeel_V4::drawPopupMenuItem(g, area, isSeparator, isActive,
                    isHighlighted, isTicked, hasSubMenu, text, shortcutKeyText, icon, textColour);
                return;
            }

            if (isHighlighted)
                g.fillAll(findColour(juce::PopupMenu::highlightedBackgroundColourId));

            // 12x12 color swatch
            auto swatchBounds = area.withWidth(20).reduced(4);
            g.setColour(moodToColour(text));
            g.fillEllipse(swatchBounds.toFloat());

            // Mood name label
            auto textBounds = area.withTrimmedLeft(24);
            g.setColour(isHighlighted
                ? findColour(juce::PopupMenu::highlightedTextColourId)
                : findColour(juce::PopupMenu::textColourId));
            g.setFont(juce::Font(juce::FontOptions{}.withHeight(12.0f)));
            g.drawText(text, textBounds, juce::Justification::centredLeft);
        }

        /// Returns the mood color for a given mood name string.
        static juce::Colour moodToColour(const juce::String& mood)
        {
            // Same table as DnaMapBrowser::moodColour() and XOceanusEditor::moodColourFor().
            if (mood == "Foundation")  return juce::Colour(0xFF9E9B97);
            if (mood == "Atmosphere")  return juce::Colour(0xFF48CAE4);
            if (mood == "Entangled")   return juce::Colour(0xFF9B5DE5);
            if (mood == "Prism")       return juce::Colour(0xFFBF40FF);
            if (mood == "Flux")        return juce::Colour(0xFFFF6B6B);
            if (mood == "Aether")      return juce::Colour(0xFFA8D8EA);
            if (mood == "Family")      return juce::Colour(0xFFE9C46A);
            if (mood == "Submerged")   return juce::Colour(0xFF0096C7);
            if (mood == "Coupling")    return juce::Colour(0xFFE9C46A);
            if (mood == "Crystalline") return juce::Colour(0xFFFFFFF0);
            if (mood == "Deep")        return juce::Colour(0xFF1A6B5A);
            if (mood == "Ethereal")    return juce::Colour(0xFF7FDBCA);
            if (mood == "Kinetic")     return juce::Colour(0xFFFF8C00);
            if (mood == "Luminous")    return juce::Colour(0xFFC6E377);
            if (mood == "Organic")     return juce::Colour(0xFF228B22);
            if (mood == "Shadow")      return juce::Colour(0xFF546E7A);
            return juce::Colour(GalleryColors::Ocean::plankton); // fallback
        }
    };

    //==========================================================================
    SavePresetDialog(PresetData       initial,
                     bool             /*isFirstSave*/,  // intent captured by caller's LaunchOptions.dialogTitle
                     SaveCallback     onSave,
                     CancelCallback   onCancel)
        : initial_   (std::move(initial))
        , onSave_    (std::move(onSave))
        , onCancel_  (std::move(onCancel))
    {
        // ── Name field ────────────────────────────────────────────────────────
        addAndMakeVisible(nameLabel_);
        nameLabel_.setText("Name *", juce::dontSendNotification);
        styleLabel(nameLabel_);

        addAndMakeVisible(nameField_);
        nameField_.setText(initial_.name, juce::dontSendNotification);
        nameField_.setSelectAllWhenFocused(true);
        nameField_.setColour(juce::TextEditor::backgroundColourId, juce::Colour(GalleryColors::Ocean::shallow));
        nameField_.setColour(juce::TextEditor::textColourId,       juce::Colour(GalleryColors::Ocean::foam));
        nameField_.setColour(juce::TextEditor::outlineColourId,    juce::Colour(GalleryColors::Ocean::surface).brighter(0.3f));
        nameField_.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(XO::Tokens::Color::Accent));
        nameField_.onTextChange = [this] { updateSaveButtonState(); };

        // ── Mood dropdown ─────────────────────────────────────────────────────
        addAndMakeVisible(moodLabel_);
        moodLabel_.setText("Mood *", juce::dontSendNotification);
        styleLabel(moodLabel_);

        addAndMakeVisible(moodDropdown_);
        moodDropdown_.setLookAndFeel(&moodLAF_);
        // 16 moods alphabetical (spec §3 decision Q3 — mood is required field)
        static const juce::StringArray kMoods {
            "Aether", "Atmosphere", "Coupling", "Crystalline", "Deep", "Entangled",
            "Ethereal", "Family", "Flux", "Foundation", "Kinetic", "Luminous",
            "Organic", "Prism", "Shadow", "Submerged"
        };
        for (int i = 0; i < kMoods.size(); ++i)
            moodDropdown_.addItem(kMoods[i], i + 1);

        if (initial_.mood.isNotEmpty())
            moodDropdown_.setText(initial_.mood, juce::dontSendNotification);
        moodDropdown_.onChange = [this] { updateSaveButtonState(); };

        // ── Category dropdown ─────────────────────────────────────────────────
        addAndMakeVisible(categoryLabel_);
        categoryLabel_.setText("Category", juce::dontSendNotification);
        styleLabel(categoryLabel_);

        addAndMakeVisible(categoryDropdown_);
        categoryDropdown_.setColour(juce::ComboBox::backgroundColourId, juce::Colour(GalleryColors::Ocean::shallow));
        categoryDropdown_.setColour(juce::ComboBox::textColourId,       juce::Colour(GalleryColors::Ocean::foam));
        categoryDropdown_.setColour(juce::ComboBox::outlineColourId,    juce::Colour(GalleryColors::Ocean::surface).brighter(0.3f));
        categoryDropdown_.addItem("— None —", 1);
        for (int i = 0; i < (int)kPresetCategories.size(); ++i)
            categoryDropdown_.addItem(kPresetCategories[i], i + 2);

        if (initial_.category.has_value() && initial_.category->isNotEmpty())
            categoryDropdown_.setText(*initial_.category, juce::dontSendNotification);
        else
            categoryDropdown_.setSelectedId(1, juce::dontSendNotification); // "— None —"

        // ── Tags field ────────────────────────────────────────────────────────
        addAndMakeVisible(tagsLabel_);
        tagsLabel_.setText("Tags", juce::dontSendNotification);
        styleLabel(tagsLabel_);

        addAndMakeVisible(tagsField_);
        tagsField_.setText(initial_.tags.joinIntoString(", "), juce::dontSendNotification);
        styleTextEditor(tagsField_);

        // ── Description field ─────────────────────────────────────────────────
        addAndMakeVisible(descriptionLabel_);
        descriptionLabel_.setText("Description", juce::dontSendNotification);
        styleLabel(descriptionLabel_);

        addAndMakeVisible(descriptionField_);
        descriptionField_.setMultiLine(true);
        descriptionField_.setReturnKeyStartsNewLine(true);
        descriptionField_.setText(initial_.description, juce::dontSendNotification);
        styleTextEditor(descriptionField_);

        // ── Buttons ───────────────────────────────────────────────────────────
        addAndMakeVisible(saveButton_);
        saveButton_.setButtonText("Save");
        saveButton_.setColour(juce::TextButton::buttonColourId,    juce::Colour(XO::Tokens::Color::Accent));
        saveButton_.setColour(juce::TextButton::textColourOffId,   juce::Colour(GalleryColors::Ocean::deep));
        saveButton_.setColour(juce::TextButton::buttonOnColourId,  juce::Colour(XO::Tokens::Color::AccentBright));
        saveButton_.onClick = [this] { handleSaveClicked(); };

        addAndMakeVisible(cancelButton_);
        cancelButton_.setButtonText("Cancel");
        cancelButton_.setColour(juce::TextButton::buttonColourId,  juce::Colour(GalleryColors::Ocean::shallow));
        cancelButton_.setColour(juce::TextButton::textColourOffId, juce::Colour(GalleryColors::Ocean::foam));
        cancelButton_.onClick = [this] { if (onCancel_) onCancel_(); };

        updateSaveButtonState();
        setSize(480, 380);
    }

    ~SavePresetDialog() override
    {
        moodDropdown_.setLookAndFeel(nullptr);
    }

    void paint(juce::Graphics& g) override
    {
        // D1: submarine console depth-tone background
        g.fillAll(juce::Colour(GalleryColors::Ocean::twilight));

        // Depth-ring border (D5 style: subtle inner border)
        g.setColour(juce::Colour(GalleryColors::Ocean::surface).brighter(0.15f));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 4.0f, 1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(24);

        // Button row at bottom
        auto buttonRow = bounds.removeFromBottom(36);
        bounds.removeFromBottom(12);

        // Layout helper: label row above field
        auto layoutLabeled = [&](juce::Label& lbl, juce::Component& field, int fieldH)
        {
            auto row = bounds.removeFromTop(16 + fieldH);
            lbl.setBounds(row.removeFromTop(16));
            field.setBounds(row.withHeight(fieldH));
            bounds.removeFromTop(8);
        };

        layoutLabeled(nameLabel_,        nameField_,         28);
        layoutLabeled(moodLabel_,        moodDropdown_,      28);
        layoutLabeled(categoryLabel_,    categoryDropdown_,  28);
        layoutLabeled(tagsLabel_,        tagsField_,         28);
        layoutLabeled(descriptionLabel_, descriptionField_,  60);

        cancelButton_.setBounds(buttonRow.removeFromRight(100));
        buttonRow.removeFromRight(10);
        saveButton_.setBounds(buttonRow.removeFromRight(100));
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            if (onCancel_) onCancel_();
            return true;
        }
        if (key == juce::KeyPress::returnKey && saveButton_.isEnabled()
            && !descriptionField_.hasKeyboardFocus(false))
        {
            handleSaveClicked();
            return true;
        }
        return false;
    }

private:
    //==========================================================================
    void styleLabel(juce::Label& lbl)
    {
        lbl.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
        lbl.setColour(juce::Label::textColourId, juce::Colour(GalleryColors::Ocean::salt));
    }

    void styleTextEditor(juce::TextEditor& te)
    {
        te.setColour(juce::TextEditor::backgroundColourId, juce::Colour(GalleryColors::Ocean::shallow));
        te.setColour(juce::TextEditor::textColourId,       juce::Colour(GalleryColors::Ocean::foam));
        te.setColour(juce::TextEditor::outlineColourId,    juce::Colour(GalleryColors::Ocean::surface).brighter(0.3f));
        te.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(XO::Tokens::Color::Accent));
    }

    void updateSaveButtonState()
    {
        const bool nameOK = nameField_.getText().trim().isNotEmpty();
        const bool moodOK = moodDropdown_.getSelectedId() > 0;
        saveButton_.setEnabled(nameOK && moodOK);
    }

    void handleSaveClicked()
    {
        if (!onSave_) return;

        // Build output preset from form fields
        PresetData out = initial_;
        out.name        = juce::File::createLegalFileName(nameField_.getText().trim());
        out.mood        = moodDropdown_.getText();

        const auto catText = categoryDropdown_.getText();
        if (catText.isEmpty() || catText == "— None —")
            out.category = std::nullopt;
        else
            out.category = catText;

        out.tags = juce::StringArray::fromTokens(tagsField_.getText(), ",", "\"");
        out.tags.trim();
        out.tags.removeEmptyStrings();
        out.description = descriptionField_.getText();

        // Collision-check via the confirm-overwrite overload
        auto presetDir = PresetManager::getUserPresetDirectory();
        auto target    = presetDir.getChildFile(out.name + ".xometa");

        // Check for collision without using the blocking showOkCancelBox.
        // JUCE_MODAL_LOOPS_PERMITTED=0 in plugin context — use async confirm.
        if (target.existsAsFile())
        {
            // Collision: show async confirm dialog. Fields remain intact.
            const juce::String capturedName = out.name;
            auto capturedOut = out;
            auto capturedOnSave = onSave_;
            auto capturedTarget = target;

            auto* confirmDialog = new juce::AlertWindow(
                "Overwrite preset?",
                "A preset named \"" + capturedName + "\" already exists.\n\nOverwrite it?",
                juce::MessageBoxIconType::QuestionIcon,
                this);
            confirmDialog->addButton("Overwrite", 1);
            confirmDialog->addButton("Cancel",    0);

            confirmDialog->enterModalState(
                true,
                juce::ModalCallbackFunction::create(
                    [capturedTarget, capturedOut, capturedOnSave](int result) mutable
                    {
                        if (result != 1) return; // user cancelled
                        PresetManager pm;
                        if (pm.savePresetToFile(capturedTarget, capturedOut))
                        {
                            if (capturedOnSave)
                                capturedOnSave(capturedOut);
                        }
                    }),
                true /* deleteWhenDismissed */);
        }
        else
        {
            // No collision — save directly.
            PresetManager pm;
            if (pm.savePresetToFile(target, out))
                onSave_(out); // caller is responsible for closing the dialog
            else
                ToastOverlay::show("Save failed — check disk space or permissions.",
                                   Toast::Level::Warn);
        }
    }

    //==========================================================================
    PresetData     initial_;
    // isFirstSave_ is used only by the caller (XOceanusEditor) to set the
    // DialogWindow title and choose the initial name — not stored here.
    SaveCallback   onSave_;
    CancelCallback onCancel_;

    MoodDropdownLookAndFeel moodLAF_;

    juce::Label      nameLabel_;
    juce::TextEditor nameField_;

    juce::Label    moodLabel_;
    juce::ComboBox moodDropdown_;

    juce::Label    categoryLabel_;
    juce::ComboBox categoryDropdown_;

    juce::Label      tagsLabel_;
    juce::TextEditor tagsField_;

    juce::Label      descriptionLabel_;
    juce::TextEditor descriptionField_;

    juce::TextButton saveButton_;
    juce::TextButton cancelButton_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SavePresetDialog)
};

} // namespace xoceanus
