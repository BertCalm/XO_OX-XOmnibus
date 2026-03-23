# Outshine Empty State — "The Forge Awaits"

**Feature**: Outshine — First Launch / Empty State
**Designer**: Issea (ceremony concept)
**Engineer**: Lucy (JUCE implementation)
**Status**: Specification v1.0

---

## Philosophy

This is the first thing a user sees when they open Outshine for the first time. It should feel like a moment, not an error.

Empty states are pure *ma* — the meaning in the absence. "No files loaded" is clinical. "The forge is ready" is an invitation.

The negative space is the most important element. The icon and text occupy roughly 20% of the window area. The other 80% is intentional emptiness — room for the user's imagination to fill with what they are about to create.

---

## Visual Layout

Center of the window, vertically and horizontally centered, with generous negative space on all sides.

```
╔══════════════════════════════════════════════════════════╗
║                                                          ║
║                                                          ║
║                                                          ║
║                      ┌─────────┐                        ║
║                      │         │                        ║
║                      │  ◊──◊   │  ← Anvil/forge icon    ║
║                      │  │  │   │     (subtle, outlined)  ║
║                      │  ╘══╛   │                        ║
║                      └─────────┘                        ║
║                                                          ║
║              The forge is ready.                         ║
║                                                          ║
║          Drop samples here to begin.                     ║
║    WAV files, folders, or XPN archives.                  ║
║                                                          ║
║           ┌──────────────────────┐                       ║
║           │   Browse Files...    │  ← Ghost button       ║
║           └──────────────────────┘                       ║
║                                                          ║
║                                                          ║
║                                                          ║
╚══════════════════════════════════════════════════════════╝
```

---

## Element Specifications

### Forge Icon

- **Art**: Custom SVG — stylized anvil with two glowing contact points (◊) in XO Gold `#E9C46A`
- **Size**: 64×64px
- **Style**: Outlined, 2px stroke, `#666666` body with `#E9C46A` highlights
- **Animation**: The two gold points pulse gently (opacity 0.6→1.0→0.6) on a 3-second cycle
- **Character**: The pulse is like embers — the forge is warm, waiting, alive
- **Accessibility**: `prefers-reduced-motion` — static, no pulse

### Title: "The forge is ready."

- **Font**: `--xo-type-title-lg` (Space Grotesk 13px, Regular)
- **Color**: `#E0E0E0`
- **Margin-top**: 24px below icon

### Subtitle: "Drop samples here to begin."

- **Font**: `--xo-type-body-lg` (Inter 13px, Regular)
- **Color**: `#A0A0A0`
- **Margin-top**: 8px below title

### Supporting text: "WAV files, folders, or XPN archives."

- **Font**: `--xo-type-body-sm` (Inter 11px)
- **Color**: `#666666`
- **Margin-top**: 4px

### Browse Button

- **Tier**: Ghost
- **Size**: Medium (36px height)
- **Label**: "Browse Files..."
- **Margin-top**: 24px
- **Note**: This is the ONLY interactive element — the empty state is calm

---

## Drop Zone Behavior

### Drag Enter

When files are dragged over the window:

1. **Entire window** becomes a drop zone (not just the icon area)
2. Border: 2px dashed `#E9C46A` appears around the window interior (16px inset)
3. Background: `rgba(233, 196, 106, 0.03)` — barely perceptible gold wash
4. Forge icon: scales to 1.05× and the gold points brighten to full opacity
5. Title changes to: "Release to begin forging."
6. Transition: 200ms ease-out for all changes

### Drop

When files are released:

1. Drop zone border fills solid briefly — 100ms flash of `rgba(233, 196, 106, 0.1)`
2. Forge icon pulses once (scale 1.0→1.15→1.0 over 300ms) — the forge "catches" the material
3. Transition to Auto Mode begins at 300ms — the empty state fades out, classification begins

### Drag Exit

- Border and background wash removed
- Title restores to "The forge is ready."
- Forge icon returns to ambient ember pulse
- Transition: 200ms ease-out

---

## Alternative First States

### Returning User (no active project)

- **Title**: "Ready for the next piece."
- **Subtitle**: "Drop samples or open a recent project."
- **Addition**: A "Recent" section below the main prompt showing the last 3 projects as small subtle cards

### Error Recovery (files rejected)

- **Title**: Unchanged — "The forge is ready."
- **Toast**: Warning status — e.g. "3 files skipped: unsupported format (.mp3, .aiff). Convert to WAV first."
- **Reasoning**: The empty state stays calm and inviting. The forge does not falter.

---

## Emotional Design Notes (Issea)

This empty state communicates three things without saying them:

1. **You are welcome here.** Not "No files loaded" (clinical) but "The forge is ready" (inviting).
2. **This is simple.** One action: drop files. One alternative: browse. Nothing else competes for attention.
3. **Something beautiful will happen.** The pulsing embers suggest potential energy — the forge is alive, waiting for your material to transform.

The ceremony is in the restraint. There are no tooltips explaining what Outshine does, no feature callouts, no onboarding arrows. The user dropped audio software on their machine. They know what to do. This screen honors that knowledge.

---

## JUCE Implementation (Lucy)

### Component Structure

```cpp
class OutshineEmptyState : public juce::Component, private juce::Timer
{
public:
    std::function<void(const juce::StringArray&)> onFilesDropped;
    std::function<void()> onBrowseClicked;

    OutshineEmptyState()
    {
        // Render forge SVG to cached Image at construction — not per frame
        forgeImage = renderForgeIcon(64, 64);

        addAndMakeVisible(browseButton);
        browseButton.setButtonText("Browse Files...");
        browseButton.onClick = [this] { if (onBrowseClicked) onBrowseClicked(); };

        startTimer(33); // 30Hz for ember pulse
    }

    void paint(juce::Graphics& g) override
    {
        // 1. Draw forge icon with ember alpha applied to gold contact points
        float alpha = 0.6f + 0.4f * std::sin(emberPhase);
        g.setOpacity(1.0f);
        drawForgeIcon(g, iconBounds, alpha);

        // 2. Draw text (center-aligned)
        g.setFont(getTitleFont());
        g.setColour(juce::Colour(0xFFE0E0E0));
        g.drawFittedText(titleText, titleBounds, juce::Justification::centred, 1);

        g.setFont(getBodyLargeFont());
        g.setColour(juce::Colour(0xFF888888));
        g.drawFittedText("Drop samples here to begin.", subtitleBounds, juce::Justification::centred, 1);

        g.setFont(getBodySmallFont());
        g.setColour(juce::Colour(0xFF666666));
        g.drawFittedText("WAV files, folders, or XPN archives.", supportBounds, juce::Justification::centred, 1);

        // 3. If drag is active, draw dashed border and gold wash
        if (isDragOver)
        {
            g.setColour(juce::Colour(0x08E9C46A)); // rgba(233,196,106,0.03)
            g.fillAll();

            g.setColour(juce::Colour(0xFFE9C46A));
            auto borderRect = getLocalBounds().reduced(16).toFloat();
            // Draw dashed border — 6px dash, 4px gap
            drawDashedBorder(g, borderRect, 2.0f, 6.0f, 4.0f);
        }
    }

    void resized() override
    {
        // Compute centered layout
        auto bounds = getLocalBounds();
        auto center = bounds.getCentre();

        iconBounds    = juce::Rectangle<int>(center.x - 32, center.y - 90, 64, 64);
        titleBounds   = juce::Rectangle<int>(bounds.getX(), iconBounds.getBottom() + 24, bounds.getWidth(), 20);
        subtitleBounds = juce::Rectangle<int>(bounds.getX(), titleBounds.getBottom() + 8, bounds.getWidth(), 20);
        supportBounds  = juce::Rectangle<int>(bounds.getX(), subtitleBounds.getBottom() + 4, bounds.getWidth(), 16);

        auto btnWidth = 160;
        browseButton.setBounds(center.x - btnWidth / 2, supportBounds.getBottom() + 24, btnWidth, 36);
    }

    void timerCallback() override
    {
        // Ember pulse: sinusoidal phase on ~3-second period at 30Hz
        // 3000ms / 33ms ≈ 90.9 ticks per cycle → step = (2π / 90.9)
        emberPhase += juce::MathConstants<float>::twoPi / 90.9f;
        if (emberPhase > juce::MathConstants<float>::twoPi)
            emberPhase -= juce::MathConstants<float>::twoPi;

        repaint(iconBounds); // Repaint icon area only — not the full component
    }

    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray&) override { return true; }

    void fileDragEnter(const juce::StringArray&, int, int) override
    {
        isDragOver = true;
        titleText = "Release to begin forging.";
        repaint();
    }

    void fileDragExit(const juce::StringArray&) override
    {
        isDragOver = false;
        titleText = "The forge is ready.";
        repaint();
    }

    void filesDropped(const juce::StringArray& files, int, int) override
    {
        isDragOver = false;
        titleText = "The forge is ready.";
        triggerForgeCapturePulse();
        if (onFilesDropped) onFilesDropped(files);
    }

private:
    juce::Image forgeImage;
    juce::TextButton browseButton;

    juce::Rectangle<int> iconBounds, titleBounds, subtitleBounds, supportBounds;
    juce::String titleText = "The forge is ready.";
    bool isDragOver = false;
    float emberPhase = 0.0f;

    void triggerForgeCapturePulse()
    {
        // Scale 1.0 → 1.15 → 1.0 over 300ms
        // Implemented via a short juce::Animator or manual phase tracking
    }

    static void drawDashedBorder(juce::Graphics& g, juce::Rectangle<float> rect,
                                  float strokeWidth, float dashLen, float gapLen);
    static juce::Image renderForgeIcon(int w, int h);
    void drawForgeIcon(juce::Graphics& g, juce::Rectangle<int> bounds, float emberAlpha);
};
```

### Performance Budget

| Concern | Approach |
|---|---|
| Component count | 2 total: `OutshineEmptyState` + `browseButton` |
| Repaint frequency | 30Hz during ember pulse (`startTimer(33)`) |
| Repaint area | Icon rect only (`repaint(iconBounds)`) — not full component |
| Image allocation | Forge icon rendered to `juce::Image` once at construction |
| Drop zone border | Drawn directly in `paint()` — no additional Component |
| Reduced motion | Check `juce::Desktop::getInstance().isReducedMotionEnabled()` at construction; if true, skip `startTimer` |

---

## Forge Icon SVG Reference

The icon is a stylized anvil — two diamond contact points (◊) at the top connected by a crossbar, a central post, and a wide base. The body is outlined in `#666666` (2px stroke, no fill). The two contact points are rendered in XO Gold `#E9C46A` and carry the ember animation alpha.

```svg
<!-- Forge Icon — 64×64 viewport -->
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" fill="none">
  <!-- Anvil body -->
  <rect x="16" y="38" width="32" height="10" rx="2" stroke="#666666" stroke-width="2"/>
  <rect x="22" y="48" width="20" height="6" rx="1" stroke="#666666" stroke-width="2"/>
  <!-- Post -->
  <line x1="32" y1="22" x2="32" y2="38" stroke="#666666" stroke-width="2"/>
  <!-- Crossbar -->
  <line x1="20" y1="30" x2="44" y2="30" stroke="#666666" stroke-width="2"/>
  <!-- Contact points — XO Gold, animated -->
  <polygon points="20,26 24,30 20,34 16,30" stroke="#E9C46A" stroke-width="1.5" fill="none" class="ember"/>
  <polygon points="44,26 48,30 44,34 40,30" stroke="#E9C46A" stroke-width="1.5" fill="none" class="ember"/>
</svg>
```

---

## Token Reference

| Token | Value | Usage |
|---|---|---|
| `--xo-gold` | `#E9C46A` | Forge contact points, drag border |
| `--xo-type-title-lg` | Space Grotesk 13px Regular | Title text |
| `--xo-type-body-lg` | Inter 13px Regular | Subtitle text |
| `--xo-type-body-sm` | Inter 11px Regular | Supporting text |
| `--xo-surface-drag-wash` | `rgba(233,196,106,0.03)` | Drag-over background |
| `--xo-surface-drop-flash` | `rgba(233,196,106,0.10)` | Drop confirmation flash |

---

## Open Questions

1. **Forge icon art**: Confirm final SVG path with Issea before Lucy renders to `juce::Image`. The diamond (◊) glyph form above is a placeholder geometry — may want to revisit proportions.
2. **Recent projects cards**: Exact card spec for the returning-user variant is deferred until the project persistence model is finalized.
3. **Reduced motion detection in JUCE**: `juce::Desktop::isReducedMotionEnabled()` availability needs to be confirmed against the target JUCE version (8.0.4). Fallback: expose a settings flag.
4. **Capture pulse animation**: The 1.0→1.15→1.0 forge capture pulse may use `juce::ComponentAnimator` or a manual phase variable — Lucy to choose based on existing animation patterns in the codebase.
