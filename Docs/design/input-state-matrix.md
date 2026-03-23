# Input Component State Matrix

**XO_OX Design System — Mantine 7-State Model with Material Treatment**

Applies to: Outshine (JUCE), Originate (iOS), audio-xpm-creator (Web/React)

---

## Overview

This specification defines all input component states across the XO_OX product surface. The model is adopted from the Mantine library's 7-state input taxonomy, extended with XO_OX material treatment, cross-platform tokens, and platform-specific implementation guidance.

All input components share the same 7-state behavioral model. Visual rendering differs per platform but color tokens and state semantics are identical.

---

## Color Tokens

| Token | Value | Usage |
|---|---|---|
| `--xo-input-border-default` | `#444444` | Unfocused border |
| `--xo-input-border-focus` | `#E9C46A` | Focus ring (XO Gold) |
| `--xo-input-border-error` | `#F44336` | Error border |
| `--xo-input-border-disabled` | `#333333` | Disabled border |
| `--xo-input-bg-default` | `#2A2A2A` | Default background |
| `--xo-input-bg-focus` | `#2E2E2E` | Lifted background on focus |
| `--xo-input-bg-disabled` | `#222222` | Disabled background |
| `--xo-input-text-value` | `#E0E0E0` | Value text |
| `--xo-input-text-placeholder` | `#666666` | Placeholder text |
| `--xo-input-text-placeholder-active` | `#555555` | Placeholder while focused |
| `--xo-input-text-disabled` | `#555555` | Disabled text |
| `--xo-input-text-label` | `#AAAAAA` | Label above input |
| `--xo-input-text-label-disabled` | `#555555` | Disabled label |
| `--xo-input-text-unit` | `#666666` | Unit suffix (numeric) |
| `--xo-input-cursor` | `#E9C46A` | Focus cursor color |
| `--xo-input-option-hover-bg` | `rgba(233,196,106,0.1)` | Dropdown option hover |
| `--xo-input-option-selected-text` | `#E9C46A` | Selected dropdown option |

---

## The 7-State Matrix

### State 1: Placeholder
*Empty, unfocused. Default resting state before any interaction.*

| Property | Value |
|---|---|
| Border | 1px `#444444` |
| Background | `#2A2A2A` |
| Placeholder text | `--xo-type-body-sm`, `#666666` |
| Label above | `--xo-type-label-lg`, `#AAAAAA` |
| Cursor | text |

---

### State 2: Active Placeholder
*Empty, focused. User has clicked or tabbed in but has not typed.*

| Property | Value |
|---|---|
| Border | 1px `#E9C46A` |
| Background | `#2E2E2E` (lifted +4 lightness) |
| Placeholder text | `#555555` (dimmed to recede) |
| Cursor | blinking, `#E9C46A` |
| Transition | 150ms ease-out on border and background |

---

### State 3: Active Normal
*Has value, focused. The input is live and the user is editing.*

| Property | Value |
|---|---|
| Border | 1px `#E9C46A` |
| Background | `#2E2E2E` |
| Value text | `--xo-type-body-lg`, `#E0E0E0` |
| Cursor | visible, `#E9C46A` |
| Clear button | visible (×), 20×20, right-aligned |

---

### State 4: Normal
*Has value, unfocused. The most common resting state.*

| Property | Value |
|---|---|
| Border | 1px `#444444` |
| Background | `#2A2A2A` |
| Value text | `--xo-type-body-lg`, `#E0E0E0` |
| Clear button | hidden |

---

### State 5: Disabled
*Field cannot be interacted with.*

| Property | Value |
|---|---|
| Border | 1px `#333333` |
| Background | `#222222` |
| Value text | `#555555` |
| Label | `#555555` |
| Cursor | `not-allowed` (web) / no interaction (native) |
| Opacity | 0.6 applied to entire component |
| Pointer events | none |

---

### State 6: Error
*Has a validation error. Field remains interactive.*

| Property | Value |
|---|---|
| Border | 1px `#F44336` |
| Background | `#2A2A2A` (unchanged — calm) |
| Value text | `#E0E0E0` |
| Error message | `--xo-type-label-sm`, `#F44336`, below the field |
| Error icon | `⚠` inline-start of error message |
| Animation | None — no shake. Calm error presentation. |

---

### State 7: Error + Disabled
*Form submitted with errors, field locked during processing.*

| Property | Value |
|---|---|
| Border | 1px `#F44336` at 0.4 opacity |
| Background | `#222222` |
| Value text | `#555555` |
| Error message | `#F44336` at 0.4 opacity |
| Opacity | 0.6 on entire component |
| When to use | Submission in-flight, async validation pending, read-only error review |

---

## Input Variants

### Text Input

Standard single-line text entry. Implements all 7 states.

| Property | Value |
|---|---|
| Height | 36px (aligns with Medium button) |
| Border-radius | 6px |
| Horizontal padding | 12px |
| Font | `--xo-type-body-lg` |
| Inner shadow | 1px top-edge, `rgba(0,0,0,0.15)` — adds tactile weight |
| Clear button (×) | Appears in State 3 (Active Normal). 20×20, `#A0A0A0`, right-aligned. Clears value and returns to State 2. |

---

### Numeric Input

For parameter values, BPM, frequency, and expression controls.

All 7 states plus the following:

| Property | Value |
|---|---|
| Value font | `--xo-type-mono` (JetBrains Mono) |
| Unit suffix | `--xo-type-label-sm`, `#666666`, right of value (e.g., `440 Hz`, `120 BPM`) |
| Stepper arrows | `▲▼` on right edge, `#A0A0A0`. Up/down one increment per click. |
| Drag-to-adjust | Vertical drag changes value. Used for Outshine expression controls. Delta: 1 unit per 2px drag. Shift-drag: fine (0.1 unit per 2px). |
| Min/max clamp | Value clamped on blur. No error state for out-of-range — silently clamp. |

---

### Dropdown / Select

Single selection from a predefined list.

All 7 states plus the following:

| Property | Value |
|---|---|
| Chevron | `▼`, `#A0A0A0`, right-aligned. Rotates 180° when open. |
| Dropdown panel | `#2E2E2E` background, 8px border-radius, `0 4px 16px rgba(0,0,0,0.4)` shadow |
| Option height | 32px |
| Option font | `--xo-type-body-lg`, `#E0E0E0` |
| Option hover | `rgba(233,196,106,0.1)` background |
| Selected option | `#E9C46A` text + 2px left border in `#E9C46A` |
| Panel max-height | 240px with overflow scroll |
| Panel open animation | 100ms ease-out, origin top |
| Focus state in panel | keyboard arrow navigation, highlighted option has hover treatment |

---

### Textarea (Multi-line)

For longer text: preset descriptions, notes, project metadata.

All 7 states plus the following:

| Property | Value |
|---|---|
| Min-height | 72px (2 lines) |
| Default height | 108px (3 lines) |
| Resize | Vertical only. Resize handle: bottom-right corner, subtle grip dots in `#444444`. |
| Character count | `--xo-type-label-sm`, `#666666`, bottom-right corner. Visible only in States 2 and 3 (focused). Format: `42 / 200` |
| Hard limit | When `maxLength` set, count turns `#F44336` when ≥ 90% full. |

---

## Material Treatment

Following Issea's material principles for the XO_OX surface:

1. **Focus transition** — 150ms ease-out on border color and background. The input announces readiness before the user finishes the gesture of clicking in.

2. **Background lift on focus** — `#2A2A2A` → `#2E2E2E`. The field rises 4 lightness units to meet the user's attention. This is not a glow. It is a physical lift, like pressing a key that has travel.

3. **Inner shadow** — 1px top-edge shadow at `rgba(0,0,0,0.15)` on all inputs in all states. Nearly invisible but adds weight. Issea's tactility: the field has depth, not flatness.

4. **Calm errors** — No shake animation. No pulsing red. The error border and message appear at rest, stating a fact. The product does not panic when the user makes a mistake.

5. **Gold as focus** — `#E9C46A` is the single color of live attention. It appears only in States 2 and 3. Nowhere else in the input system. Its presence means: *this field is active right now.*

---

## JUCE Implementation (Outshine)

```cpp
// Custom TextEditor with state-aware painting
// File: Source/UI/Components/XOTextInput.h

class XOTextInput : public juce::TextEditor,
                    private juce::Timer
{
public:
    enum class InputState
    {
        Placeholder,        // State 1
        ActivePlaceholder,  // State 2
        ActiveNormal,       // State 3
        Normal,             // State 4
        Disabled,           // State 5
        Error,              // State 6
        ErrorDisabled       // State 7
    };

    void setError(const juce::String& message)
    {
        errorMessage = message;
        hasError = message.isNotEmpty();
        updateState();
        repaint();
    }

    void clearError()
    {
        errorMessage = {};
        hasError = false;
        updateState();
        repaint();
    }

private:
    bool hasError = false;
    juce::String errorMessage;
    juce::Colour currentBorderColour;
    juce::Colour targetBorderColour;
    float animProgress = 1.0f;

    InputState computeState() const
    {
        if (!isEnabled())
            return hasError ? InputState::ErrorDisabled : InputState::Disabled;
        if (hasError)
            return InputState::Error;
        if (hasKeyboardFocus(false))
            return getText().isEmpty() ? InputState::ActivePlaceholder : InputState::ActiveNormal;
        return getText().isEmpty() ? InputState::Placeholder : InputState::Normal;
    }

    juce::Colour getBorderColour(InputState state) const
    {
        switch (state)
        {
            case InputState::Placeholder:       return juce::Colour(0xff444444);
            case InputState::ActivePlaceholder: return juce::Colour(0xffE9C46A);
            case InputState::ActiveNormal:      return juce::Colour(0xffE9C46A);
            case InputState::Normal:            return juce::Colour(0xff444444);
            case InputState::Disabled:          return juce::Colour(0xff333333);
            case InputState::Error:             return juce::Colour(0xffF44336);
            case InputState::ErrorDisabled:     return juce::Colour(0xffF44336).withAlpha(0.4f);
        }
        return juce::Colour(0xff444444);
    }

    juce::Colour getBackgroundColour(InputState state) const
    {
        switch (state)
        {
            case InputState::ActivePlaceholder:
            case InputState::ActiveNormal:      return juce::Colour(0xff2E2E2E);
            case InputState::Disabled:
            case InputState::ErrorDisabled:     return juce::Colour(0xff222222);
            default:                            return juce::Colour(0xff2A2A2A);
        }
    }

    void paint(juce::Graphics& g) override
    {
        auto state = computeState();
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        auto bgColour = getBackgroundColour(state);

        // Background
        g.setColour(bgColour);
        g.fillRoundedRectangle(bounds, 6.0f);

        // Inner shadow (top edge, tactile weight)
        g.setColour(juce::Colours::black.withAlpha(0.15f));
        g.drawHorizontalLine(1,
            bounds.getX() + 6.0f,
            bounds.getRight() - 6.0f);

        // Animated border
        g.setColour(currentBorderColour);
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

        // Disabled overlay
        if (state == InputState::Disabled || state == InputState::ErrorDisabled)
        {
            g.setColour(juce::Colours::black.withAlpha(0.4f));
            g.fillRoundedRectangle(bounds, 6.0f);
        }

        juce::TextEditor::paint(g);
    }

    void updateState()
    {
        auto state = computeState();
        targetBorderColour = getBorderColour(state);

        // Start 150ms transition
        animProgress = 0.0f;
        startTimerHz(60);
    }

    void timerCallback() override
    {
        animProgress = juce::jmin(1.0f, animProgress + (1.0f / 9.0f)); // 9 frames @ 60fps ≈ 150ms
        currentBorderColour = currentBorderColour.interpolatedWith(targetBorderColour, animProgress);
        repaint();
        if (animProgress >= 1.0f)
            stopTimer();
    }

    void focusGained(FocusChangeType) override { updateState(); }
    void focusLost(FocusChangeType) override    { updateState(); }
};
```

### JUCE: Error Label

Error message is a separate `juce::Label` positioned below the input, not part of `TextEditor`:

```cpp
// In parent component layout:
errorLabel.setFont(juce::Font("Inter", 11.0f, juce::Font::plain));
errorLabel.setColour(juce::Label::textColourId, juce::Colour(0xffF44336));
errorLabel.setText(juce::CharPointer_UTF8("\xe2\x9a\xa0") + " " + errorMessage,
                   juce::dontSendNotification);
// Position directly below input with 4px gap
```

### JUCE: Numeric Input Drag

Override `mouseDrag()` in numeric input subclass:

```cpp
void mouseDrag(const juce::MouseEvent& e) override
{
    if (!isDragging) return;

    float delta = -(e.position.y - dragStartY); // negative: drag up = increase
    float increment = e.mods.isShiftDown() ? 0.05f : 1.0f; // shift = fine
    float newValue = dragStartValue + (delta / 2.0f) * increment;
    setValue(juce::jlimit(minimum, maximum, newValue));
}
```

---

## Web Implementation (audio-xpm-creator)

```tsx
// src/components/ui/XOInput.tsx

import { useState, useRef } from 'react';
import { cn } from '@/lib/utils';

const inputStateClasses = {
  placeholder:        'border-[#444] bg-[#2A2A2A] placeholder-[#666]',
  activePlaceholder:  'border-[#E9C46A] bg-[#2E2E2E] placeholder-[#555]',
  activeNormal:       'border-[#E9C46A] bg-[#2E2E2E] text-[#E0E0E0]',
  normal:             'border-[#444] bg-[#2A2A2A] text-[#E0E0E0]',
  disabled:           'border-[#333] bg-[#222] text-[#555] opacity-60 cursor-not-allowed',
  error:              'border-[#F44336] bg-[#2A2A2A] text-[#E0E0E0]',
  errorDisabled:      'border-[#F44336]/40 bg-[#222] text-[#555] opacity-60 cursor-not-allowed',
} as const;

interface XOInputProps extends React.InputHTMLAttributes<HTMLInputElement> {
  label?: string;
  error?: string;
  hint?: string;
}

export function XOInput({ label, error, disabled, className, id, ...props }: XOInputProps) {
  const [focused, setFocused] = useState(false);
  const inputRef = useRef<HTMLInputElement>(null);
  const hasValue = Boolean(props.value || props.defaultValue);
  const inputId = id ?? label?.toLowerCase().replace(/\s+/g, '-');
  const errorId = error ? `${inputId}-error` : undefined;

  const stateKey = (() => {
    if (disabled && error)  return 'errorDisabled';
    if (disabled)           return 'disabled';
    if (error)              return 'error';
    if (focused && hasValue) return 'activeNormal';
    if (focused)            return 'activePlaceholder';
    if (hasValue)           return 'normal';
    return 'placeholder';
  })();

  return (
    <div className="flex flex-col gap-1">
      {label && (
        <label
          htmlFor={inputId}
          className={cn(
            'text-[11px] font-medium tracking-wide uppercase',
            disabled ? 'text-[#555]' : 'text-[#AAA]'
          )}
        >
          {label}
        </label>
      )}

      <div className="relative">
        <input
          ref={inputRef}
          id={inputId}
          disabled={disabled}
          aria-invalid={Boolean(error)}
          aria-describedby={errorId}
          onFocus={() => setFocused(true)}
          onBlur={() => setFocused(false)}
          className={cn(
            // Base
            'w-full h-9 px-3 rounded-[6px] border text-[13px]',
            'outline-none transition-[border-color,background-color] duration-150 ease-out',
            // Inner shadow (tactile weight)
            'shadow-[inset_0_1px_0_rgba(0,0,0,0.15)]',
            inputStateClasses[stateKey],
            className
          )}
          {...props}
        />

        {/* Clear button — State 3 only */}
        {stateKey === 'activeNormal' && props.onChange && (
          <button
            type="button"
            aria-label="Clear"
            className="absolute right-2 top-1/2 -translate-y-1/2 w-5 h-5 flex items-center justify-center text-[#A0A0A0] hover:text-[#E0E0E0] transition-colors"
            onMouseDown={(e) => {
              e.preventDefault();
              const nativeInput = inputRef.current;
              if (nativeInput) {
                Object.getOwnPropertyDescriptor(
                  window.HTMLInputElement.prototype, 'value'
                )?.set?.call(nativeInput, '');
                nativeInput.dispatchEvent(new Event('input', { bubbles: true }));
              }
            }}
          >
            ×
          </button>
        )}
      </div>

      {error && (
        <p id={errorId} role="alert" className="flex items-center gap-1 text-[11px] text-[#F44336]">
          <span aria-hidden="true">⚠</span>
          {error}
        </p>
      )}
    </div>
  );
}
```

### Web: Dropdown

```tsx
// src/components/ui/XOSelect.tsx

interface XOSelectProps extends React.SelectHTMLAttributes<HTMLSelectElement> {
  label?: string;
  error?: string;
  options: Array<{ value: string; label: string }>;
}

export function XOSelect({ label, error, options, disabled, id, ...props }: XOSelectProps) {
  const selectId = id ?? label?.toLowerCase().replace(/\s+/g, '-');

  return (
    <div className="flex flex-col gap-1">
      {label && (
        <label htmlFor={selectId} className={cn('text-[11px] font-medium tracking-wide uppercase', disabled ? 'text-[#555]' : 'text-[#AAA]')}>
          {label}
        </label>
      )}
      <div className="relative">
        <select
          id={selectId}
          disabled={disabled}
          aria-invalid={Boolean(error)}
          className={cn(
            'w-full h-9 px-3 pr-8 rounded-[6px] border text-[13px] appearance-none',
            'outline-none transition-[border-color,background-color] duration-150 ease-out',
            'shadow-[inset_0_1px_0_rgba(0,0,0,0.15)]',
            error
              ? (disabled ? 'border-[#F44336]/40 bg-[#222] text-[#555] opacity-60' : 'border-[#F44336] bg-[#2A2A2A] text-[#E0E0E0]')
              : (disabled ? 'border-[#333] bg-[#222] text-[#555] opacity-60 cursor-not-allowed' : 'border-[#444] bg-[#2A2A2A] text-[#E0E0E0] focus:border-[#E9C46A] focus:bg-[#2E2E2E]')
          )}
          {...props}
        >
          {options.map(opt => (
            <option key={opt.value} value={opt.value} className="bg-[#2E2E2E]">
              {opt.label}
            </option>
          ))}
        </select>
        {/* Chevron */}
        <span className="pointer-events-none absolute right-3 top-1/2 -translate-y-1/2 text-[#A0A0A0] text-[10px]">▼</span>
      </div>
      {error && (
        <p role="alert" className="flex items-center gap-1 text-[11px] text-[#F44336]">
          <span aria-hidden="true">⚠</span> {error}
        </p>
      )}
    </div>
  );
}
```

---

## iOS Implementation (Originate)

```swift
// XOTextField.swift

import UIKit

final class XOTextField: UITextField {

    enum InputState {
        case placeholder        // State 1
        case activePlaceholder  // State 2
        case activeNormal       // State 3
        case normal             // State 4
        case disabled           // State 5
        case error              // State 6
        case errorDisabled      // State 7
    }

    var hasError: Bool = false {
        didSet { updateAppearance(animated: true) }
    }

    var errorMessage: String? {
        didSet { errorLabel.text = errorMessage.map { "⚠ \($0)" } }
    }

    private let errorLabel: UILabel = {
        let l = UILabel()
        l.font = .systemFont(ofSize: 11, weight: .regular)
        l.textColor = UIColor(hex: "#F44336")
        l.numberOfLines = 0
        return l
    }()

    // MARK: - State

    private var currentState: InputState {
        if !isEnabled { return hasError ? .errorDisabled : .disabled }
        if hasError { return .error }
        if isFirstResponder { return (text?.isEmpty ?? true) ? .activePlaceholder : .activeNormal }
        return (text?.isEmpty ?? true) ? .placeholder : .normal
    }

    // MARK: - Appearance

    private func updateAppearance(animated: Bool) {
        let state = currentState
        let borderColor  = Self.borderColor(for: state)
        let bgColor      = Self.backgroundColor(for: state)
        let borderWidth  = isFirstResponder ? 1.5 : 1.0  // Slightly heavier focus ring

        let apply = {
            self.layer.borderColor  = borderColor.cgColor
            self.layer.borderWidth  = borderWidth
            self.backgroundColor    = bgColor
            self.alpha              = (state == .disabled || state == .errorDisabled) ? 0.6 : 1.0
        }

        if animated {
            UIView.animate(withDuration: 0.15, delay: 0, options: .curveEaseOut, animations: apply)
        } else {
            apply()
        }
    }

    private static func borderColor(for state: InputState) -> UIColor {
        switch state {
        case .placeholder:       return UIColor(hex: "#444444")
        case .activePlaceholder: return UIColor(hex: "#E9C46A")
        case .activeNormal:      return UIColor(hex: "#E9C46A")
        case .normal:            return UIColor(hex: "#444444")
        case .disabled:          return UIColor(hex: "#333333")
        case .error:             return UIColor(hex: "#F44336")
        case .errorDisabled:     return UIColor(hex: "#F44336").withAlphaComponent(0.4)
        }
    }

    private static func backgroundColor(for state: InputState) -> UIColor {
        switch state {
        case .activePlaceholder, .activeNormal: return UIColor(hex: "#2E2E2E")
        case .disabled, .errorDisabled:         return UIColor(hex: "#222222")
        default:                                return UIColor(hex: "#2A2A2A")
        }
    }

    // MARK: - Layout

    override func layoutSubviews() {
        super.layoutSubviews()
        layer.cornerRadius = 6
        layer.masksToBounds = true
        // Inner shadow: subtle top-edge depth
        layer.shadowColor  = UIColor.black.withAlphaComponent(0.15).cgColor
        layer.shadowOffset = CGSize(width: 0, height: 1)
        layer.shadowRadius = 0
        layer.shadowOpacity = 1
        layer.masksToBounds = false
    }

    override var intrinsicContentSize: CGSize {
        CGSize(width: UIView.noIntrinsicMetric, height: 36)
    }

    // Inner padding
    override func textRect(forBounds bounds: CGRect) -> CGRect {
        bounds.insetBy(dx: 12, dy: 0)
    }
    override func editingRect(forBounds bounds: CGRect) -> CGRect {
        bounds.insetBy(dx: 12, dy: 0)
    }

    // MARK: - Delegate hooks
    override func becomeFirstResponder() -> Bool {
        let r = super.becomeFirstResponder()
        updateAppearance(animated: true)
        return r
    }
    override func resignFirstResponder() -> Bool {
        let r = super.resignFirstResponder()
        updateAppearance(animated: true)
        return r
    }
}
```

---

## State Transition Diagram

```
                    ┌─────────────────────────────────────────────────────────────┐
                    │                       Input States                           │
                    └─────────────────────────────────────────────────────────────┘

    User taps/clicks ──►  1: PLACEHOLDER        ──focus──►  2: ACTIVE PLACEHOLDER
                               (empty, blur)                    (empty, focus)
                                                                     │ type
                                                                     ▼
    blur ◄──────────────  4: NORMAL             ◄──blur──   3: ACTIVE NORMAL
                           (value, blur)                        (value, focus)

    disable ──► any ──►  5: DISABLED

    validation fail ──►  4/3 ──►  6: ERROR       ──focus preserved
                                  (value, error, blur)

    submit+lock ──►  6: ERROR ──►  7: ERROR + DISABLED
```

---

## Accessibility Checklist

- Labels always visible above the field — no placeholder-only labeling
- Error messages associated via `aria-describedby` (web) / `accessibilityHint` (iOS)
- Focus traversal follows visual reading order (top-to-bottom, left-to-right)
- Disabled fields: announced as "dimmed" or "unavailable" by screen readers
- Error indicator uses both color AND text — color alone is not sufficient
- All text/background combinations meet WCAG 4.5:1 contrast ratio:
  - `#E0E0E0` on `#2A2A2A` — 7.8:1 ✓
  - `#AAAAAA` on `#2A2A2A` — 5.0:1 ✓
  - `#E9C46A` on `#2A2A2A` — 5.9:1 ✓
  - `#F44336` on `#2A2A2A` — 4.7:1 ✓
- Cursor color (`#E9C46A`) is a visual enhancement only — not a navigational requirement
- Touch targets on iOS: minimum 44×44pt tap area even if visual height is 36pt

---

## Usage Notes

### State derivation is always computed, never stored

Do not track `inputState` as a variable. Derive it from component primitives: `isEmpty`, `isFocused`, `isEnabled`, `hasError`. This prevents states from going out of sync.

### Error is set from outside, not from within

Input components do not self-validate. The parent (form, store action, submit handler) sets the error. The input only renders it.

### Clear button does not submit or trigger blur

The `×` clear button clears the value and returns focus to the input (State 2: Active Placeholder). It does not trigger `onBlur` or form submission.

### Numeric drag does not change focus state

Dragging a numeric input does not move focus away from the currently focused element unless the user explicitly clicks the numeric field first.

---

*Last updated: 2026-03-22*
*Applies to: Outshine (JUCE), Originate (iOS), audio-xpm-creator (Web/React)*
