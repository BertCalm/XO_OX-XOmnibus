// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

// CommandPalette.h — Cmd+K modal palette for fuzzy-searching presets + engines.
// Spec: Docs/plans/2026-05-05-cmdk-palette-design.md
// V1 scope: presets + engines only. Params/FX deferred to v1.1.
//
// Architecture: overlay child of XOceanusEditor. Uses std::function injection
// for engine loading to avoid a circular include with XOceanusProcessor.h.

#include <juce_gui_basics/juce_gui_basics.h>
#include <deque>
#include <functional>
#include <string>
#include <vector>

#include "../../Core/PresetManager.h"
#include "../../Core/EngineRegistry.h"

namespace xoceanus {

//==============================================================================
// Result kind
enum class CommandResultKind { Preset, Engine };

//==============================================================================
// A single ranked result entry in the palette.
struct CommandResult
{
    CommandResultKind kind        = CommandResultKind::Preset;
    juce::String      displayName;
    juce::String      secondaryLabel;
    juce::Colour      accent;
    int               presetIndex = -1;
    std::string       engineId;
    float             score       = 0.0f;
};

//==============================================================================
// Scoring helpers — free functions for testability.

// Literal fuzzy match against a preset's name (3pt), tags (2pt), description+mood (1pt each).
inline float scorePresetLiteral(const PresetData& p, const juce::StringArray& tokens)
{
    float score = 0.0f;
    auto nameLower = p.name.toLowerCase();
    auto descLower = p.description.toLowerCase();
    auto moodLower = p.mood.toLowerCase();

    for (const auto& token : tokens)
    {
        auto t = token.toLowerCase();
        if (nameLower.contains(t)) score += 3.0f;
        for (const auto& tag : p.tags)
            if (tag.toLowerCase().contains(t)) { score += 2.0f; break; }
        if (descLower.contains(t)) score += 1.0f;
        if (moodLower.contains(t)) score += 1.0f;
    }
    return score;
}

// DNA dimension match — maps query tokens to the 6 DNA axes (warmth/brightness/etc.).
inline float scorePresetDNA(const PresetData& p, const juce::StringArray& tokens)
{
    float score = 0.0f;
    for (const auto& token : tokens)
    {
        auto t = token.toLowerCase();
        if (t.contains("bright"))                        score += p.dna.brightness * 2.0f;
        if (t.contains("warm"))                          score += p.dna.warmth     * 2.0f;
        if (t.contains("mov"))                           score += p.dna.movement   * 2.0f;
        if (t.contains("dense") || t.contains("density")) score += p.dna.density  * 2.0f;
        if (t.contains("space") || t.contains("spacious")) score += p.dna.space   * 2.0f;
        if (t.contains("aggress"))                       score += p.dna.aggression * 2.0f;
    }
    return score;
}

// Engine literal match — id (3pt), displayName (2pt), kindTag (1pt).
inline float scoreEngineLiteral(const juce::String& engineId,
                                const juce::String& displayName,
                                const juce::String& kindTag,
                                const juce::StringArray& tokens)
{
    float score = 0.0f;
    auto idLower      = engineId.toLowerCase();
    auto displayLower = displayName.toLowerCase();
    auto kindLower    = kindTag.toLowerCase();

    for (const auto& token : tokens)
    {
        auto t = token.toLowerCase();
        if (idLower.contains(t))      score += 3.0f;
        if (displayLower.contains(t)) score += 2.0f;
        if (kindLower.contains(t))    score += 1.0f;
    }
    return score;
}

//==============================================================================
// CommandPalette — Cmd+K overlay component.
//
// Usage (from XOceanusEditor):
//   commandPalette_ = std::make_unique<CommandPalette>(
//       processor.getPresetManager(),
//       [this](int slot, std::string id) { processor.loadEngine(slot, id); },
//       [this] { closeCommandPalette(); });
//   addAndMakeVisible(*commandPalette_);
//   commandPalette_->setBounds(getLocalBounds());
//   commandPalette_->grabSearchFocus();
//
class CommandPalette : public juce::Component, private juce::Timer
{
public:
    CommandPalette(PresetManager& pm,
                   std::function<void(int slot, std::string engineId)> loadEngineFn,
                   std::function<void()> onClose);

    ~CommandPalette() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

    void grabSearchFocus();
    void startEntryAnimation();  // TODO v1.1: 200ms fade-in

    // Exposed for testing: re-rank against the current library + query.
    void rerankForTesting(const juce::String& query);
    const std::vector<CommandResult>& getResults() const { return results_; }

private:
    void timerCallback() override;
    void rerank(const juce::String& query);
    void rebuildEmptyState();
    void activateResult(int index, int targetSlot);
    void renderResults(juce::Graphics& g, const juce::Rectangle<int>& resultsArea);
    void renderFooter(juce::Graphics& g);

    PresetManager&                                presetManager_;
    std::function<void(int, std::string)>         loadEngineFn_;
    std::function<void()>                         onClose_;

    juce::TextEditor                              searchField_;
    std::vector<CommandResult>                    results_;
    int                                           selectedIndex_ = 0;
};

//==============================================================================
// Inline implementations — kept in header per XOceanus DSP-in-headers convention.

inline CommandPalette::CommandPalette(PresetManager& pm,
                                      std::function<void(int, std::string)> loadEngineFn,
                                      std::function<void()> onClose)
    : presetManager_(pm),
      loadEngineFn_(std::move(loadEngineFn)),
      onClose_(std::move(onClose))
{
    addAndMakeVisible(searchField_);
    searchField_.setMultiLine(false);
    searchField_.setReturnKeyStartsNewLine(false);
    searchField_.onTextChange = [this] { startTimer(50); };

    // Transparent searchField so the palette's own background shows through.
    searchField_.setColour(juce::TextEditor::backgroundColourId,
                           juce::Colour::fromRGBA(255, 255, 255, 18));
    searchField_.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    searchField_.setColour(juce::TextEditor::outlineColourId,
                           juce::Colour::fromRGBA(255, 255, 255, 60));
    searchField_.setColour(juce::TextEditor::focusedOutlineColourId,
                           juce::Colour::fromRGBA(60, 180, 190, 200));
    searchField_.setFont(juce::Font(16.0f, juce::Font::plain));
    searchField_.setTextToShowWhenEmpty("Type to search presets and engines...",
                                        juce::Colour::fromRGBA(255, 255, 255, 80));

    setWantsKeyboardFocus(true);
}

inline CommandPalette::~CommandPalette() { stopTimer(); }

inline void CommandPalette::timerCallback()
{
    stopTimer();
    rerank(searchField_.getText());
    repaint();
}

inline void CommandPalette::rerank(const juce::String& query)
{
    results_.clear();
    selectedIndex_ = 0;

    if (query.isEmpty()) { rebuildEmptyState(); return; }

    juce::StringArray tokens;
    tokens.addTokens(query.toLowerCase(), " \t", "");
    tokens.removeEmptyStrings();
    if (tokens.isEmpty()) { rebuildEmptyState(); return; }

    auto lib = presetManager_.getPresetLibrary();
    if (lib != nullptr)
    {
        for (int i = 0; i < static_cast<int>(lib->size()); ++i)
        {
            const auto& p = (*lib)[static_cast<size_t>(i)];
            float litScore = scorePresetLiteral(p, tokens);
            float dnaScore = scorePresetDNA(p, tokens);
            float total    = litScore + dnaScore;
            if (total > 0.0f)
            {
                CommandResult r;
                r.kind          = CommandResultKind::Preset;
                r.displayName   = p.name;
                r.secondaryLabel = p.mood;
                r.accent        = juce::Colour::fromRGBA(60, 180, 190, 200); // teal stub
                r.presetIndex   = i;
                r.score         = total;
                results_.push_back(std::move(r));
            }
        }
    }

    auto engineIds = EngineRegistry::instance().getRegisteredIds();
    for (const auto& id : engineIds)
    {
        juce::String idStr(id);
        float score = scoreEngineLiteral(idStr, idStr, "", tokens);
        if (score > 0.0f)
        {
            CommandResult r;
            r.kind           = CommandResultKind::Engine;
            r.displayName    = idStr;
            r.secondaryLabel = "Engine";
            r.accent         = juce::Colour::fromRGBA(200, 150, 60, 200); // amber stub
            r.engineId       = id;
            r.score          = score;
            results_.push_back(std::move(r));
        }
    }

    std::sort(results_.begin(), results_.end(),
        [](const CommandResult& a, const CommandResult& b) {
            if (a.score != b.score) return a.score > b.score;
            if (a.kind  != b.kind)  return a.kind == CommandResultKind::Preset;
            return a.displayName < b.displayName;
        });

    if (results_.size() > 10) results_.resize(10);
}

inline void CommandPalette::rerankForTesting(const juce::String& query) { rerank(query); }

inline void CommandPalette::rebuildEmptyState()
{
    auto lib              = presetManager_.getPresetLibrary();
    const auto& recentP   = presetManager_.getRecentPresetIndices();
    int presetsAdded = 0;
    if (lib != nullptr)
    {
        for (int idx : recentP)
        {
            if (presetsAdded >= 3) break;
            if (idx < 0 || idx >= static_cast<int>(lib->size())) continue;
            const auto& p = (*lib)[static_cast<size_t>(idx)];
            CommandResult r;
            r.kind           = CommandResultKind::Preset;
            r.displayName    = p.name;
            r.secondaryLabel = p.mood;
            r.accent         = juce::Colour::fromRGBA(60, 180, 190, 200);
            r.presetIndex    = idx;
            results_.push_back(std::move(r));
            ++presetsAdded;
        }
    }

    const auto& recentE = EngineRegistry::instance().getRecentEngineIds();
    int enginesAdded = 0;
    for (const auto& id : recentE)
    {
        if (enginesAdded >= 3) break;
        CommandResult r;
        r.kind           = CommandResultKind::Engine;
        r.displayName    = juce::String(id);
        r.secondaryLabel = "Engine";
        r.accent         = juce::Colour::fromRGBA(200, 150, 60, 200);
        r.engineId       = id;
        results_.push_back(std::move(r));
        ++enginesAdded;
    }
}

inline void CommandPalette::paint(juce::Graphics& g)
{
    // Backdrop (50% black overlay) — full-bounds.
    g.fillAll(juce::Colour::fromRGBA(0, 0, 0, 128));

    // Content rect — centered 640×480.
    auto contentRect = juce::Rectangle<int>(640, 480)
                         .withCentre(getLocalBounds().getCentre());

    // Panel background (XO::Tokens::Color::Surface approximation).
    g.setColour(juce::Colour::fromRGB(26, 31, 42));
    g.fillRoundedRectangle(contentRect.toFloat(), 8.0f);

    // Depth ring border (subtle).
    g.setColour(juce::Colour::fromRGBA(60, 70, 90, 200));
    g.drawRoundedRectangle(contentRect.toFloat(), 8.0f, 1.5f);

    // Render results below the search field.
    auto resultsArea = contentRect.reduced(12).withTrimmedTop(56).withTrimmedBottom(36);
    renderResults(g, resultsArea);

    renderFooter(g);
}

inline void CommandPalette::renderResults(juce::Graphics& g,
                                          const juce::Rectangle<int>& area)
{
    juce::Rectangle<int> remaining = area;
    for (size_t i = 0; i < results_.size(); ++i)
    {
        if (remaining.getHeight() < 44) break;
        auto rowRect  = remaining.removeFromTop(44);
        bool selected = (static_cast<int>(i) == selectedIndex_);

        if (selected)
        {
            g.setColour(juce::Colour::fromRGBA(60, 180, 190, 40));
            g.fillRoundedRectangle(rowRect.toFloat().reduced(2.0f), 4.0f);
        }

        // Accent bar (4px left edge).
        auto barRect = rowRect.removeFromLeft(4).reduced(0, 6);
        g.setColour(results_[i].accent);
        g.fillRoundedRectangle(barRect.toFloat(), 2.0f);

        rowRect.removeFromLeft(8); // gap after bar

        // Kind badge (right-aligned).
        auto badgeStr = (results_[i].kind == CommandResultKind::Preset) ? "PRESET" : "ENGINE";
        g.setColour(juce::Colour::fromRGBA(255, 255, 255, 100));
        g.setFont(juce::Font(10.0f, juce::Font::plain));
        g.drawText(badgeStr, rowRect.reduced(4, 0), juce::Justification::centredRight, false);

        // Display name (primary).
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(15.0f, juce::Font::plain));
        g.drawText(results_[i].displayName,
                   rowRect.removeFromTop(26).reduced(4, 0),
                   juce::Justification::centredLeft, true);

        // Secondary label (mood / "Engine").
        g.setColour(juce::Colour::fromRGBA(255, 255, 255, 153));
        g.setFont(juce::Font(11.0f, juce::Font::plain));
        g.drawText(results_[i].secondaryLabel,
                   rowRect.reduced(4, 0),
                   juce::Justification::centredLeft, true);
    }

    // Empty-state placeholder.
    if (results_.empty())
    {
        g.setColour(juce::Colour::fromRGBA(255, 255, 255, 80));
        g.setFont(juce::Font(13.0f, juce::Font::plain));
        g.drawText("No results — try a different query",
                   area, juce::Justification::centred, true);
    }
}

inline void CommandPalette::renderFooter(juce::Graphics& g)
{
    auto contentRect = juce::Rectangle<int>(640, 480)
                         .withCentre(getLocalBounds().getCentre());
    auto footerRect = contentRect.reduced(12).removeFromBottom(28);

    juce::String hint;
    if (!results_.empty())
    {
        bool isEngine = (selectedIndex_ < static_cast<int>(results_.size())
                         && results_[static_cast<size_t>(selectedIndex_)].kind
                                == CommandResultKind::Engine);
        hint = isEngine
             ? juce::String("ENTER slot 1   CMD+2/3/4 slot   ESC close")
             : juce::String("ENTER load   ESC close");
    }
    else
    {
        hint = "Type to search presets and engines   ESC close";
    }

    g.setColour(juce::Colour::fromRGBA(255, 255, 255, 120));
    g.setFont(juce::Font(11.0f, juce::Font::plain));
    g.drawText(hint, footerRect, juce::Justification::centredRight, true);
}

inline void CommandPalette::resized()
{
    auto contentRect = juce::Rectangle<int>(640, 480)
                         .withCentre(getLocalBounds().getCentre());
    searchField_.setBounds(contentRect.reduced(12).withHeight(44));
}

inline void CommandPalette::grabSearchFocus()
{
    searchField_.grabKeyboardFocus();
}

inline void CommandPalette::startEntryAnimation()
{
    // TODO v1.1: 200ms fade-in matching XO::Tokens::Motion::Duration
}

inline bool CommandPalette::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        if (onClose_) onClose_();
        return true;
    }

    if (key == juce::KeyPress::downKey)
    {
        if (!results_.empty())
        {
            selectedIndex_ = juce::jmin(selectedIndex_ + 1,
                                        static_cast<int>(results_.size()) - 1);
            repaint();
        }
        return true;
    }

    if (key == juce::KeyPress::upKey)
    {
        selectedIndex_ = juce::jmax(selectedIndex_ - 1, 0);
        repaint();
        return true;
    }

    if (key == juce::KeyPress::returnKey)
    {
        activateResult(selectedIndex_, /*targetSlot=*/0);
        return true;
    }

    if (key.getModifiers().isCommandDown())
    {
        auto kc = key.getKeyCode();
        if (kc == '2') { activateResult(selectedIndex_, 1); return true; }
        if (kc == '3') { activateResult(selectedIndex_, 2); return true; }
        if (kc == '4') { activateResult(selectedIndex_, 3); return true; }
    }

    return false;
}

inline void CommandPalette::activateResult(int index, int targetSlot)
{
    if (index < 0 || index >= static_cast<int>(results_.size())) return;
    const auto& r = results_[static_cast<size_t>(index)];

    if (r.kind == CommandResultKind::Preset)
    {
        if (presetManager_.loadPresetByIndex(r.presetIndex))
            presetManager_.recordPresetLoad(r.presetIndex);
    }
    else
    {
        if (loadEngineFn_) loadEngineFn_(targetSlot, r.engineId);
        EngineRegistry::instance().recordEngineLoad(r.engineId, targetSlot);
    }

    if (onClose_) onClose_();
}

} // namespace xoceanus
