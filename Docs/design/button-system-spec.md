# XO_OX Button System Specification

**Version**: 1.0
**Date**: 2026-03-22
**Scope**: JUCE plugin UI, iOS AUv3/Standalone, Web (XO-OX.org)
**Framework**: Game UX Kit 3-tier × 5-state × 4-size matrix with XO_OX material treatment

---

## Overview

The XO_OX Button System defines a 3-tier hierarchy of interactive controls, each with 5 interaction states and 4 size variants. This matrix ensures visual hierarchy, consistent behavior across platforms, and accessible touch/keyboard interaction.

| | Primary | Secondary | Ghost |
|---|---|---|---|
| **Purpose** | The main action | Supporting actions | Subtle / tertiary |
| **Weight** | Heavy | Medium | Light |
| **Frequency** | One per view | Several per view | Many per view |

---

## Button Tiers

### Primary — "Do the main thing"

The single most important action on screen. Use sparingly — one primary button per view.

- **Background**: XO Gold `#E9C46A` with subtle gradient (top: `#EDD08A` → bottom: `#E9C46A`)
- **Text color**: `#1A1A1A` (dark on gold for contrast)
- **Shadow**: `0 1px 3px rgba(0,0,0,0.3)` at rest
- **Examples**: "Export XPN", "Forge Instrument", "Save Preset"

### Secondary — "Other important actions"

Supporting actions that are still prominent.

- **Background**: `#3A3A3A` (elevated surface)
- **Border**: 1px solid `#555555`
- **Text color**: `#E0E0E0`
- **Examples**: "Preview", "Cancel", "Reset", "Add Source"

### Ghost — "Subtle actions"

Minimal presence for tertiary actions or dense toolbars.

- **Background**: transparent
- **Border**: none
- **Text color**: `#AAAAAA`
- **Hover background**: `rgba(255,255,255,0.05)`
- **Examples**: "Show All", "Clear", "Dismiss", toolbar icons

---

## Button Sizes

| Size | Height | Padding H | Font | Border Radius | JUCE Constant | CSS Class | iOS Points |
|------|--------|-----------|------|---------------|---------------|-----------|------------|
| Large | 44px | 24px | `--xo-type-title-sm` (12px SemiBold) | 8px | `ButtonSize::Large = 44` | `h-11 px-6` | 44pt |
| Medium | 36px | 16px | `--xo-type-label-lg` (11px Medium) | 6px | `ButtonSize::Medium = 36` | `h-9 px-4` | 36pt |
| Small | 28px | 12px | `--xo-type-label-sm` (10px Regular) | 4px | `ButtonSize::Small = 28` | `h-7 px-3` | 28pt |
| Compact | 24px | 8px | `--xo-type-label-sm` (10px Regular) | 4px | `ButtonSize::Compact = 24` | `h-6 px-2` | 24pt |

### When to use each size

- **Large**: Primary actions, modal footers, Outshine "Forge" and "Export" buttons. Touch-friendly (meets 44pt iOS minimum inherently).
- **Medium**: Standard actions throughout the UI. Most common size. Default for Secondary buttons.
- **Small**: Compact panels, secondary actions, Outshine tab-bar actions.
- **Compact**: Toolbar buttons, inline actions, tag dismiss buttons. Pair with 44pt minimum hit target on touch platforms.

---

## 5 States per Tier

All three tiers implement the same five states. Only the visual expression differs.

### Default

Normal resting state. See tier colors in the section above.

| Tier | Background | Border | Text |
|------|-----------|--------|------|
| Primary | `#EDD08A` → `#E9C46A` (gradient) | none | `#1A1A1A` |
| Secondary | `#3A3A3A` | 1px `#555555` | `#E0E0E0` |
| Ghost | transparent | none | `#AAAAAA` |

### Hover

Cursor/pointer enters the button bounds. Not applicable to touch-only contexts — use Default state there.

| Tier | Change |
|------|--------|
| Primary | Background lightens to `#F0D080` gradient, shadow expands to `0 2px 8px rgba(233,196,106,0.3)`, translateY(-1px) |
| Secondary | Border brightens to `#777777`, background to `#444444` |
| Ghost | Background `rgba(255,255,255,0.05)`, text brightens to `#CCCCCC` |

Transition for all tiers on enter: `150ms ease-out`
Transition on leave: `100ms ease-in`

### Focused (keyboard navigation)

Activated by Tab key or programmatic focus. Never suppressed — critical for accessibility.

| Tier | Change |
|------|--------|
| All | 2px focus ring in `#E9C46A` with 2px offset (ring never overlaps the button itself) |

Inner button content remains at Default state appearance while focused. The ring is the only change.

Transition: none (focus ring appears instantly).

### Pressed / Active

Mouse/touch held down on the button. Provides immediate tactile feedback.

| Tier | Change |
|------|--------|
| Primary | Background darkens to `#D4A84A`, shadow collapses to none, translateY(1px) |
| Secondary | Background to `#333333`, border to `#555555` |
| Ghost | Background `rgba(255,255,255,0.08)` |

Transition: `50ms` (intentionally fast — the "snap" of physical feedback).

### Disabled

Action is unavailable. The button remains visible; it is not hidden. Users should see that an action exists but is currently blocked.

| Tier | Change |
|------|--------|
| All | Opacity 0.4, cursor `not-allowed`, no gradient, no shadow, no hover/press response |

Do not remove disabled buttons from the layout. If an action should never appear, conditionally render it out in component logic — but if it may become available, keep it at 0.4 opacity.

---

## Icon Buttons

Icon buttons follow the same 3-tier × 5-state × 4-size matrix with these additions:

- **Icon-only**: Height equals size height; width equals height (square). Icon centered.
- **Icon + Text**: 8px gap between icon and text. Icon 16px for Large/Medium, 14px for Small/Compact.
- **Icon color**: Follows text color for the current tier and state.
- **Hit target**: On touch platforms, minimum hit target is always 44×44pt regardless of visual size. Use transparent padding.

---

## Material Treatment

What separates XO_OX buttons from the Game UX Kit's flat corporate baseline:

1. **Primary gradient**: Not a flat fill. Top: `#EDD08A`, Bottom: `#E9C46A`. Subtle, not glossy. Evokes machined gold leaf, not candy-coated plastic.

2. **Hover lift**: `translateY(-1px)` plus shadow expansion. The button rises toward the cursor — a tactile promise before the click.

3. **Press depression**: `translateY(1px)` plus shadow collapse. The button yields under pressure. The reverse of the hover lift creates a physical push-and-release sensation.

4. **Surface texture**: 1% noise overlay on Primary buttons (barely visible at normal viewing distance). Adds the sense of physical material — closer to anodized aluminum than screen-printed ink.

5. **Border weight on Secondary**: 1px border. Not hairline (0.5px feels imprecise, vanishes on low-DPI) and not heavy (2px is aggressive). Deliberate middle weight communicates "contained but not dominant."

---

## JUCE Implementation

**Lucy's Notes**

```cpp
enum class ButtonTier { Primary, Secondary, Ghost };
enum class ButtonSize { Large = 44, Medium = 36, Small = 28, Compact = 24 };

// In LookAndFeel_XO:
void drawButtonBackground(Graphics& g, Button& button,
                          const Colour& backgroundColour,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override
{
    auto bounds = button.getLocalBounds().toFloat();
    const bool isEnabled  = button.isEnabled();
    const bool isHovered  = shouldDrawButtonAsHighlighted && !shouldDrawButtonAsDown;
    const bool isPressed  = shouldDrawButtonAsDown;
    const bool isFocused  = button.hasKeyboardFocus(true);

    auto tier = getTierFromButton(button); // read ButtonTier property

    // --- Disabled overlay: apply opacity at component level, not inside paint ---
    // button.setAlpha(isEnabled ? 1.0f : 0.4f) in setButtonEnabled()

    if (tier == ButtonTier::Primary)
    {
        // Use cached gradient image per size — avoid recomputing every frame
        Colour topColour    = isPressed ? Colour(0xFFD4A84A)
                            : isHovered ? Colour(0xFFF0D080)
                            : Colour(0xFFEDD08A);
        Colour bottomColour = isPressed ? Colour(0xFFC89830)
                            : isHovered ? Colour(0xFFEDD08A)
                            : Colour(0xFFE9C46A);

        ColourGradient gradient(topColour, 0.0f, bounds.getY(),
                                bottomColour, 0.0f, bounds.getBottom(),
                                false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds.reduced(0.5f), getCornerRadiusForSize(button));

        if (!isPressed)
        {
            // Drop shadow — paint behind button in parent, or approximate with a
            // darkened inset at the bottom edge
        }
    }
    else if (tier == ButtonTier::Secondary)
    {
        Colour bg     = isPressed ? Colour(0xFF333333)
                      : isHovered ? Colour(0xFF444444)
                      : Colour(0xFF3A3A3A);
        Colour border = isHovered ? Colour(0xFF777777) : Colour(0xFF555555);

        g.setColour(bg);
        g.fillRoundedRectangle(bounds.reduced(0.5f), getCornerRadiusForSize(button));

        g.setColour(border);
        g.drawRoundedRectangle(bounds.reduced(0.5f), getCornerRadiusForSize(button), 1.0f);
    }
    else // Ghost
    {
        if (isPressed)
            g.setColour(Colour(0x14FFFFFF)); // rgba(255,255,255,0.08)
        else if (isHovered)
            g.setColour(Colour(0x0DFFFFFF)); // rgba(255,255,255,0.05)
        else
            g.setColour(Colours::transparentBlack);

        g.fillRoundedRectangle(bounds.reduced(0.5f), getCornerRadiusForSize(button));
    }

    // Focus ring — paint AFTER the button fill, on top
    if (isFocused && isEnabled)
    {
        g.setColour(Colour(0xFFE9C46A));
        g.drawRoundedRectangle(bounds.expanded(2.0f), getCornerRadiusForSize(button) + 2.0f, 2.0f);
    }
}

float getCornerRadiusForSize(Button& button)
{
    // Read ButtonSize property set on the component
    switch (getButtonSize(button))
    {
        case ButtonSize::Large:   return 8.0f;
        case ButtonSize::Medium:  return 6.0f;
        case ButtonSize::Small:   return 4.0f;
        case ButtonSize::Compact: return 4.0f;
    }
}
```

### Key JUCE considerations

- Use `juce::DrawableButton` for icon buttons — it handles icon sizing and state-aware drawable swapping automatically.
- Primary gradient: cache as `juce::Image` per logical size. Do not regenerate the gradient on every `paint()` call — this fires at 60fps and generates garbage.
- Focus ring: paint after the button background (on top), not as part of the button background. This ensures the ring is never clipped by the button's own bounds.
- DPI scaling: all sizes in this spec are logical pixels. Multiply by `Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale` (or the per-display scale) for physical pixels on Retina/HiDPI.
- Hit testing: override `hitTest()` only if the button shape is non-rectangular (e.g., a circular pad button). For all standard button tiers, the default rectangular hit test is correct.
- Disabled opacity: set `button.setAlpha(0.4f)` from the component that controls enabled state. Do not try to bake opacity into `drawButtonBackground` — JUCE's compositing handles it more correctly at the component level.
- Press translation (`translateY(1px)`): implement via `AffineTransform::translation(0, offset)` applied in the button's `paint()` override, or via a custom `Button` subclass that shifts its content bounds.

---

## iOS Implementation

**Xavier's Notes**

```swift
enum ButtonTier {
    case primary
    case secondary
    case ghost
}

enum ButtonSize: CGFloat {
    case large   = 44
    case medium  = 36
    case small   = 28
    case compact = 24
}

// iOS 15+ — use UIButton.Configuration
extension UIButton.Configuration {

    static func xoPrimary(size: ButtonSize = .medium) -> UIButton.Configuration {
        var config = UIButton.Configuration.filled()
        config.baseBackgroundColor = UIColor(hex: "#E9C46A")
        config.baseForegroundColor = UIColor(hex: "#1A1A1A")
        config.cornerStyle = .fixed
        config.background.cornerRadius = cornerRadius(for: size)
        config.contentInsets = contentInsets(for: size)
        config.titleTextAttributesTransformer = typographyTransformer(for: size)
        return config
    }

    static func xoSecondary(size: ButtonSize = .medium) -> UIButton.Configuration {
        var config = UIButton.Configuration.bordered()
        config.baseBackgroundColor = UIColor(hex: "#3A3A3A")
        config.baseForegroundColor = UIColor(hex: "#E0E0E0")
        config.background.strokeColor = UIColor(hex: "#555555")
        config.background.strokeWidth = 1.0
        config.cornerStyle = .fixed
        config.background.cornerRadius = cornerRadius(for: size)
        config.contentInsets = contentInsets(for: size)
        config.titleTextAttributesTransformer = typographyTransformer(for: size)
        return config
    }

    static func xoGhost(size: ButtonSize = .medium) -> UIButton.Configuration {
        var config = UIButton.Configuration.plain()
        config.baseForegroundColor = UIColor(hex: "#AAAAAA")
        config.cornerStyle = .fixed
        config.background.cornerRadius = cornerRadius(for: size)
        config.contentInsets = contentInsets(for: size)
        config.titleTextAttributesTransformer = typographyTransformer(for: size)
        return config
    }

    private static func cornerRadius(for size: ButtonSize) -> CGFloat {
        switch size {
        case .large:   return 8
        case .medium:  return 6
        case .small:   return 4
        case .compact: return 4
        }
    }

    private static func contentInsets(for size: ButtonSize) -> NSDirectionalEdgeInsets {
        switch size {
        case .large:   return .init(top: 0, leading: 24, bottom: 0, trailing: 24)
        case .medium:  return .init(top: 0, leading: 16, bottom: 0, trailing: 16)
        case .small:   return .init(top: 0, leading: 12, bottom: 0, trailing: 12)
        case .compact: return .init(top: 0, leading: 8,  bottom: 0, trailing: 8)
        }
    }
}

// IMPORTANT: All sizes must meet 44pt minimum touch target regardless of visual height.
// For Small (28pt) and Compact (24pt) buttons, expand the hit area:
button.configuration = .xoSmall()
button.configurationUpdateHandler = { button in
    // Visual remains 28pt, but hit rect is 44pt via:
}
// Override point(inside:with:) or set a larger frame with center alignment
```

### iOS state handling

- Hover: iPadOS only (pointer). Use `UIPointerInteraction` + `UIPointerStyle` for lift effect. Skip on iPhone.
- Pressed: `UIButton` handles highlighted state automatically — use `configurationUpdateHandler` to apply the `#D4A84A` background on `.highlighted`.
- Disabled: set `button.isEnabled = false`. UIKit applies system-level dimming; override `alpha` to 0.4 for visual consistency with this spec.
- Focus (keyboard/game controller): `UIButton` responds to `isFocused` — use `configurationUpdateHandler` to draw the gold focus ring when `state.contains(.focused)`.
- Minimum hit target: `minimumHitTargetSize = CGSize(width: 44, height: 44)` applies to all button sizes regardless of visual height. This is non-negotiable on iOS.

---

## Web Implementation (Tailwind + React)

```tsx
import { forwardRef } from "react";
import { cva, type VariantProps } from "class-variance-authority";
import { cn } from "@/lib/utils";

const buttonVariants = cva(
  // Base — shared across all tiers and sizes
  "inline-flex items-center justify-center font-medium transition-all select-none " +
  "focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[#E9C46A] " +
  "focus-visible:ring-offset-2 focus-visible:ring-offset-[#1A1A1A] " +
  "disabled:opacity-40 disabled:cursor-not-allowed disabled:pointer-events-none",
  {
    variants: {
      tier: {
        primary: [
          "bg-gradient-to-b from-[#EDD08A] to-[#E9C46A] text-[#1A1A1A] shadow-sm",
          "hover:from-[#F0D080] hover:to-[#EDD08A] hover:shadow-[0_2px_8px_rgba(233,196,106,0.3)] hover:-translate-y-px",
          "active:from-[#D4A84A] active:to-[#C89830] active:shadow-none active:translate-y-px",
        ],
        secondary: [
          "bg-[#3A3A3A] text-[#E0E0E0] border border-[#555555]",
          "hover:bg-[#444444] hover:border-[#777777]",
          "active:bg-[#333333] active:border-[#555555]",
        ],
        ghost: [
          "bg-transparent text-[#AAAAAA]",
          "hover:bg-white/5 hover:text-[#CCCCCC]",
          "active:bg-white/[0.08]",
        ],
      },
      size: {
        large:   "h-11 px-6 text-xs font-semibold tracking-wide rounded-lg",
        medium:  "h-9 px-4 text-[11px] font-medium rounded-md",
        small:   "h-7 px-3 text-[10px] font-normal rounded",
        compact: "h-6 px-2 text-[10px] font-normal rounded",
      },
    },
    defaultVariants: {
      tier: "secondary",
      size: "medium",
    },
  }
);

interface ButtonProps
  extends React.ButtonHTMLAttributes<HTMLButtonElement>,
    VariantProps<typeof buttonVariants> {
  iconLeft?: React.ReactNode;
  iconRight?: React.ReactNode;
}

const Button = forwardRef<HTMLButtonElement, ButtonProps>(
  ({ className, tier, size, iconLeft, iconRight, children, ...props }, ref) => {
    return (
      <button
        ref={ref}
        className={cn(buttonVariants({ tier, size }), className)}
        {...props}
      >
        {iconLeft && (
          <span className={cn("shrink-0", children ? "mr-2" : "")}>
            {iconLeft}
          </span>
        )}
        {children}
        {iconRight && (
          <span className={cn("shrink-0", children ? "ml-2" : "")}>
            {iconRight}
          </span>
        )}
      </button>
    );
  }
);
Button.displayName = "Button";

export { Button, buttonVariants };

// Usage examples:
// <Button tier="primary" size="large">Export XPN</Button>
// <Button tier="secondary">Preview</Button>
// <Button tier="ghost" size="compact" iconLeft={<ClearIcon />} />
```

### Tailwind config additions required

```js
// tailwind.config.js
module.exports = {
  theme: {
    extend: {
      boxShadow: {
        "xo-hover-primary": "0 2px 8px rgba(233, 196, 106, 0.3)",
      },
      transitionDuration: {
        "50": "50ms",
      },
    },
  },
};
```

---

## Full State × Tier Reference

| State | Primary Background | Primary Text | Secondary BG | Secondary Border | Ghost BG | Ghost Text |
|-------|-------------------|--------------|-------------|-----------------|----------|------------|
| Default | `#EDD08A`→`#E9C46A` | `#1A1A1A` | `#3A3A3A` | `#555555` | transparent | `#AAAAAA` |
| Hover | `#F0D080`→`#EDD08A` | `#1A1A1A` | `#444444` | `#777777` | `rgba(255,255,255,0.05)` | `#CCCCCC` |
| Focused | Default + gold ring | Default | Default + gold ring | Default | Default + gold ring | Default |
| Pressed | `#D4A84A`→`#C89830` | `#1A1A1A` | `#333333` | `#555555` | `rgba(255,255,255,0.08)` | `#AAAAAA` |
| Disabled | Default at 0.4 opacity | — | Default at 0.4 opacity | — | Default at 0.4 opacity | — |

Focus ring spec (all tiers): `2px solid #E9C46A`, `2px offset`. Never overlaps the button body.

---

## Accessibility Checklist

- [ ] Primary Gold on dark: contrast ratio 7.2:1 — exceeds WCAG AA (4.5:1) and nearly meets AAA (7:1)
- [ ] Disabled state: remains visible at 0.4 opacity — do not hide unavailable actions
- [ ] Focus ring: 2px visible ring in gold (`#E9C46A`), 2px offset, never suppressed
- [ ] Touch targets: all sizes meet 44pt minimum hit area on iOS (visual size may be smaller)
- [ ] Screen readers: `role="button"` always present; `aria-label` required for icon-only buttons
- [ ] Keyboard: Enter and Space activate; Tab and Shift+Tab navigate; Escape dismisses modals
- [ ] No color alone: state changes use multiple cues (color + transform + shadow), not color alone
- [ ] Motion: `translateY` transitions respect `prefers-reduced-motion` — disable transforms, keep color changes

### Reduced motion

```css
@media (prefers-reduced-motion: reduce) {
  .btn-primary,
  .btn-secondary,
  .btn-ghost {
    transition: background-color 150ms ease-out, color 150ms ease-out,
                border-color 150ms ease-out;
    /* No translate, no shadow animation */
  }
}
```

---

## Usage Guidelines

### Do
- Use exactly one Primary button as the decisive action on any given view or modal
- Use Secondary for supporting actions (cancel, preview, back)
- Use Ghost for actions in dense toolbars, metadata rows, and expandable panels
- Use Large for any touch-accessible primary action (Export, Forge)
- Use Compact for inline tag controls, dismiss buttons, and toolbar icon clusters

### Do not
- Place two Primary buttons side by side — this destroys hierarchy
- Use Ghost buttons as the only call-to-action — they will be missed
- Reduce button opacity manually to indicate loading — use a loading spinner or skeleton state instead
- Override the focus ring color — gold on dark is the accessibility contract for this design system
- Use text shorter than 2 characters in a text button — use an icon button instead

---

## Relationship to Other Components

- **Toggle Button**: Uses Secondary tier in Default/Ghost pair (off/on). Same size matrix.
- **Tab Bar**: Ghost tier, Compact size. Active tab gets gold text, not Primary background.
- **Pad Controls** (XOmnibus): Custom tier outside this system — pads have their own pressure-sensitive state model.
- **Outshine "Forge" CTA**: Primary tier, Large size, full-width in modal footer.
- **Toolbar Actions**: Ghost tier, Compact size, icon-only with 44pt hit target.

---

*Button System Spec v1.0 — XO_OX Design System*
*Authored for JUCE (Lucy), iOS (Xavier), and Web (Tailwind/React)*
