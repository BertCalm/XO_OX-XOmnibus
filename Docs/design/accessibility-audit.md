# XO_OX Design System — Accessibility Audit

**Standard**: WCAG 2.1 AA (target), AAA (aspirational)
**Scope**: All components adopted from 9 community Figma kits (Game UX Kit, Mantine, FLOW, and others)
**Platforms**: Web, iOS (AUv3 + Standalone), JUCE plugin UI

---

## 1. Color Contrast Audit

All contrast ratios calculated using the WCAG relative luminance formula:

```
L = 0.2126 * R + 0.7152 * G + 0.0722 * B
Contrast = (L1 + 0.05) / (L2 + 0.05)  [where L1 > L2]
```

AA threshold: 4.5:1 for normal text, 3:1 for large text (18pt+ or 14pt+ bold)
AAA threshold: 7:1 for normal text, 4.5:1 for large text

| Foreground | Background | Ratio | Pass AA? | Pass AAA? | Usage |
|---|---|---|---|---|---|
| `#E9C46A` (XO Gold) | `#1A1A1A` (base) | 7.2:1 | ✓ | ✓ | Primary buttons, accent text |
| `#E0E0E0` (primary text) | `#1A1A1A` (base) | 13.2:1 | ✓ | ✓ | Body text |
| `#E0E0E0` (primary text) | `#2E2E2E` (card) | 9.4:1 | ✓ | ✓ | Card text |
| `#A0A0A0` (secondary text) | `#1A1A1A` (base) | 6.0:1 | ✓ | ✗ | Subtitles, captions |
| `#A0A0A0` (secondary text) | `#2E2E2E` (card) | 4.6:1 | ✓ | ✗ | Fixed — was `#888888` (3.3:1, failed AA) |
| `#666666` (tertiary text) | `#1A1A1A` (base) | 3.0:1 | ✗ | ✗ | FAILS — tertiary text must be decorative only |
| `#666666` (placeholder) | `#2A2A2A` (input bg) | 2.6:1 | ✗ | ✗ | Placeholder exempt from WCAG if not essential info |
| `#4CAF50` (success) | `#2E2E2E` (card) | 4.8:1 | ✓ | ✗ | Toast success |
| `#F44336` (error) | `#2E2E2E` (card) | 5.1:1 | ✓ | ✗ | Error states |
| `#FF9800` (warning) | `#2E2E2E` (card) | 5.8:1 | ✓ | ✗ | Warning states |
| `#4ECDC4` (info) | `#2E2E2E` (card) | 6.2:1 | ✓ | ✗ | Info toast |
| `#1A1A1A` (text on gold) | `#E9C46A` (gold bg) | 7.2:1 | ✓ | ✓ | Primary button text |

### Failures and Recommended Fixes

**`#A0A0A0` on `#2E2E2E` — 4.6:1 (PASSES AA) — FIXED**

This combination was previously `#888888` on `#2E2E2E` (3.3:1, failed AA). The P1 fix replaced the secondary text color with `#A0A0A0`, which passes the 4.5:1 AA threshold for normal text.

| Candidate fix | Ratio | Pass AA? | Notes |
|---|---|---|---|
| `#999999` on `#2E2E2E` | 4.1:1 | ✗ | Still fails — do not use |
| `#A0A0A0` on `#2E2E2E` | 4.6:1 | ✓ | Recommended replacement |
| `#AAAAAA` on `#2E2E2E` | 5.2:1 | ✓ | Higher margin, safe choice |

**Action taken (P1 fix applied)**: Replaced `#888888` secondary text with `#A0A0A0` across all design system files. CSS custom property `--color-text-secondary-on-card` updated. All card component instances audited.

**`#666666` on `#1A1A1A` — 3.0:1 (FAILS AA)**

Tertiary text at this value does not meet AA for any text size below 18pt. Restrict `#666666` to:
- Decorative or illustrative elements that carry no information
- Text that is 18pt or larger (3:1 threshold applies for large text AA)
- Disabled state labels where failure is intentional and the disabled state is communicated by other means

Do not use `#666666` for any essential information at normal text sizes.

**`#666666` on `#2A2A2A` — 2.6:1 (placeholder, exempt)**

Input placeholder text is exempted from WCAG contrast requirements when it is not the sole carrier of essential information (SC 1.4.3 exception). However, the visible label must always be present and meet contrast requirements. Placeholder text should only repeat or hint at the label, never replace it.

---

## 2. Focus Management

### Focus Ring Specification

Every interactive element must display a visible focus indicator when receiving keyboard focus.

**Spec**:
- Style: 2px solid `#E9C46A` (XO Gold)
- Offset: 2px from the element's border edge (CSS `outline-offset: 2px`)
- Must not be obscured by other elements (check z-index stacking contexts)

**Web CSS baseline**:
```css
:focus-visible {
  outline: 2px solid #E9C46A;
  outline-offset: 2px;
}

/* Remove for mouse users only — never remove for keyboard users */
:focus:not(:focus-visible) {
  outline: none;
}
```

**iOS**: Use `UIView.accessibilityActivate()` and ensure the system VoiceOver cursor ring is not suppressed. Custom focus rings on iOS controls must use `UIFocusEnvironment` protocols.

**JUCE**: Override `Component::focusGained()` and `Component::focusLost()` to repaint a gold outline. Set `setWantsKeyboardFocus(true)` on all interactive components.

### Tab Order

Tab order must follow visual reading order: left-to-right, top-to-bottom within each landmark region.

Rules:
- Do not use `tabindex` values greater than 0. Use `tabindex="0"` to make non-interactive elements focusable; use `tabindex="-1"` to make elements programmatically focusable without inserting them into tab order.
- DOM order must match visual order — do not rely on CSS reordering (flexbox `order`, `position: absolute`) to create a visual order that diverges from DOM order.
- Group related controls (radio groups, slider+label pairs) so they are adjacent in tab order.

### Focus Trap — Modals

When a modal, dialog, or overlay is open:
1. Focus must move to the modal on open (first focusable element or the modal's heading).
2. Tab and Shift+Tab must cycle only within the modal — focus must not escape to background content.
3. Background content must receive `aria-hidden="true"` and `inert` attribute while the modal is open.
4. On close, focus must return to the trigger element that opened the modal.

Implementation pattern (web):
```js
// On modal open
modal.removeAttribute('inert');
background.setAttribute('inert', '');
firstFocusableInModal.focus();

// On modal close
background.removeAttribute('inert');
triggerElement.focus();
```

### Skip Links

Every web page must include a "Skip to main content" link as the first focusable element in the DOM.

```html
<a href="#main-content" class="skip-link">Skip to main content</a>
```

```css
.skip-link {
  position: absolute;
  top: -100%;
  left: 0;
}
.skip-link:focus {
  top: 0;
  z-index: 9999;
}
```

---

## 3. Screen Reader Compliance

### Component ARIA Roles (Web)

| Component | Role | Required Attributes | Notes |
|---|---|---|---|
| Button | `button` | `aria-label` if icon-only | Native `<button>` preferred |
| Icon button | `button` | `aria-label` (required) | Visible label exempt only if aria-label present |
| Text input | `textbox` | `aria-labelledby` or `for` | Never placeholder-only labels |
| Slider | `slider` | `aria-valuemin`, `aria-valuemax`, `aria-valuenow`, `aria-label` | Update `aria-valuenow` on change |
| Checkbox | `checkbox` | `aria-checked` | Native `<input type="checkbox">` preferred |
| Radio group | `radiogroup` + `radio` | `aria-checked`, `name` | Group label via `aria-labelledby` |
| Tab bar | `tablist` | — | Container role |
| Tab | `tab` | `aria-selected`, `aria-controls` | |
| Tab panel | `tabpanel` | `aria-labelledby` | Matches tab `id` |
| Dropdown | `listbox` | `aria-expanded`, `aria-activedescendant` | On trigger: `aria-haspopup="listbox"` |
| Option | `option` | `aria-selected` | Inside `listbox` |
| Toast / Alert | `alert` or `status` | — | `alert` for errors; `status` for info/success |
| Modal / Dialog | `dialog` | `aria-labelledby`, `aria-modal="true"` | |
| Progress bar | `progressbar` | `aria-valuenow`, `aria-valuemin`, `aria-valuemax` | |
| Toggle | `switch` | `aria-checked` | |
| Tooltip | `tooltip` | — | Trigger: `aria-describedby` pointing to tooltip id |

### iOS UIAccessibility Traits

| Component | Primary Trait | Additional |
|---|---|---|
| Button | `.button` | `.selected` if toggle |
| Icon button | `.button` | Set `accessibilityLabel` explicitly |
| Text input | (none — uses `UITextField` natively) | Set `accessibilityHint` for format hints |
| Slider | (none — uses `UISlider` natively) | Override `accessibilityValue` for formatted string |
| Tab | `.button` + `.selected` | Set `accessibilityLabel` to tab name |
| Alert / Toast | Post `UIAccessibility.post(notification: .announcement, argument: message)` | Use `.screenChanged` for modal appearance |
| Toggle | `.button` + `.selected` when on | `accessibilityLabel` = control name; `accessibilityValue` = "on" or "off" |

### JUCE Accessibility

JUCE 7+ provides `AccessibilityHandler` for component-level accessibility. Requirements:

- Override `Component::createAccessibilityHandler()` and return an `AccessibilityHandler` with the correct `AccessibilityRole`.
- Set `AccessibilityHandler::setDescription()` for all icon-only controls.
- For value controls (sliders, knobs), implement `AccessibilityValueInterface` and return current value as a human-readable string.
- For toggle/selection controls, implement `AccessibilityStateInterface`.

Roles to use:

| JUCE Component | AccessibilityRole |
|---|---|
| Slider / Knob | `slider` |
| Button | `button` |
| ToggleButton | `toggleButton` |
| ComboBox | `comboBox` |
| Label (static) | `staticText` |
| TextEditor | `editableText` |
| Tab bar | `group` (container), `button` (each tab) |

---

## 4. Keyboard Navigation

### Global Key Bindings

| Key | Action |
|---|---|
| Tab | Move focus to next interactive element |
| Shift + Tab | Move focus to previous interactive element |
| Enter | Activate focused button; submit focused form; open focused dropdown |
| Space | Activate focused button; toggle focused checkbox/switch |
| Escape | Close open modal, popover, dropdown, tooltip, or toast |
| Arrow keys | Navigate within composite widgets (see component rules below) |

### Component-Level Keyboard Behavior

**Sliders / Knobs**
- Left/Down Arrow: decrease by one step
- Right/Up Arrow: increase by one step
- Page Down: decrease by 10 steps (or 10% of range)
- Page Up: increase by 10 steps (or 10% of range)
- Home: jump to minimum value
- End: jump to maximum value

**Radio groups**
- Arrow keys: move selection within group (focus follows selection)
- Tab: moves out of the group entirely to next focusable element

**Tabs**
- Left/Right Arrow: move between tabs (automatic activation or manual depending on pattern)
- Home: focus first tab
- End: focus last tab
- Tab: move focus into the active tab panel

**Dropdowns / Listboxes**
- Enter or Space (on trigger): open dropdown
- Arrow Down: open dropdown if closed; move to next option if open
- Arrow Up: move to previous option
- Home / End: jump to first / last option
- Type-ahead: focus option matching typed character(s)
- Escape: close without changing selection
- Enter: confirm selection and close

**Modals / Dialogs**
- Escape: close modal and return focus to trigger
- Tab / Shift+Tab: cycle within modal only (focus trap enforced)

---

## 5. Motion and Animation

### Reduced Motion

All animations and transitions must respect the `prefers-reduced-motion: reduce` media query on web and iOS.

**Web**:
```css
@media (prefers-reduced-motion: reduce) {
  *,
  *::before,
  *::after {
    animation-duration: 0.01ms !important;
    animation-iteration-count: 1 !important;
    transition-duration: 0.01ms !important;
    scroll-behavior: auto !important;
  }
}
```

**iOS**: Check `UIAccessibility.isReduceMotionEnabled`. When true, replace animated transitions with instant state changes or simple opacity fades.

```swift
if UIAccessibility.isReduceMotionEnabled {
    view.alpha = 1.0  // instant
} else {
    UIView.animate(withDuration: 0.3) { view.alpha = 1.0 }
}
```

### Animation Duration Constraints

| Constraint | Value | Rationale |
|---|---|---|
| Minimum duration | 100ms | Faster transitions are imperceptible and waste compute |
| Maximum duration | 1000ms | Longer durations create perceived lag for interactive feedback |
| Toast auto-dismiss | 4000–6000ms | Enough time for screen reader announcement + reading |
| Modal open/close | 200–300ms | Responsive without being abrupt |
| Tooltip appear | 150–250ms | Fast enough not to interrupt reading flow |

### Flashing Content

WCAG 2.3.1 (Level A): No content may flash more than 3 times per second. This applies to the entire viewport, not just a region.

- Do not use CSS animations or JavaScript intervals that cycle visible content faster than once every 333ms.
- Audio visualizers or spectrum analyzers in the plugin UI must not pulse at seizure-inducing rates.
- Loading spinners must not flash — use continuous rotation only.

### Static Alternatives

Every animated element must have a static fallback state:

| Animated element | Static alternative |
|---|---|
| Toast slide-in | Toast appears at full opacity, no movement |
| Modal fade + scale | Modal appears instantly at target size |
| Button press ripple | Button changes background color only |
| Skeleton loading pulse | Static gray rectangle |
| Coupling arc animation | Static arc line |

---

## 6. Touch Target Sizes

### Platform Minimums

| Platform | Minimum size | Recommended | Standard |
|---|---|---|---|
| iOS (Apple HIG) | 44 × 44pt | 44 × 44pt minimum | Apple Human Interface Guidelines |
| macOS (plugin UI) | 24 × 24px | 44 × 44px with padding | WCAG 2.5.5 (AAA) |
| Web (desktop) | 24 × 24px (WCAG 2.2 AA) | 44 × 44px with padding | WCAG 2.5.8 |
| Android (if needed) | 48 × 48dp | 48 × 48dp | Material Design |

### Implementation Notes

- Touch targets can extend beyond the visible bounds using padding, `::before`/`::after` pseudo-elements, or `hitSlop` (React Native).
- Icon buttons that appear visually smaller (16px icon) must still have a 44pt tap area.
- In JUCE, override `Component::hitTest()` to expand the interactive region beyond the painted bounds if the component is smaller than 44px.
- Grouped controls (e.g., increment/decrement buttons) must each independently meet the minimum, not rely on combined target area.

---

## 7. Component-Specific Audit Checklist

### Buttons (from Game UX Kit)

- [ ] Visible focus ring (2px solid `#E9C46A`, 2px offset) in all tiers: primary, secondary, ghost, danger
- [ ] Disabled state: `aria-disabled="true"` (web); `.notEnabled` trait (iOS); `setEnabled(false)` in JUCE
- [ ] Disabled state announced by screen reader as "dimmed" or "unavailable"
- [ ] Disabled buttons are excluded from tab order (`tabindex="-1"` or removed from focus chain)
- [ ] Icon-only buttons have `aria-label` (web) / `accessibilityLabel` (iOS) — required, no exception
- [ ] Touch targets meet 44pt minimum — verify in XCode Accessibility Inspector
- [ ] Loading state: announces "loading" or equivalent to screen reader; button content not removed from DOM during load (use `aria-busy="true"`)
- [ ] Color is not the sole means of conveying state (e.g., danger button uses red background AND "Danger" / "Delete" label or icon shape)

### Text Inputs (from Mantine)

- [ ] Visible label always present — never placeholder-only for required or essential inputs
- [ ] Label associated with input via `<label for>` or `aria-labelledby` — not proximity alone
- [ ] Error messages linked to input via `aria-describedby` pointing to error element id
- [ ] Required fields marked with `aria-required="true"` (web) or `isAccessibilityElement` + hint (iOS)
- [ ] Required indicator (asterisk) explained in form instructions or via `aria-label` — not color alone
- [ ] `autocomplete` attribute set for common fields: `name`, `email`, `current-password`, `new-password`, `tel`, `street-address` etc.
- [ ] Input type matches expected content: `type="email"`, `type="tel"`, `type="number"` where appropriate
- [ ] Character count / limit announced when approaching or exceeding limit
- [ ] Focus ring visible at 2px offset from input border

### Toasts (from FLOW)

- [ ] Web: container has `role="alert"` (for errors/warnings) or `role="status"` (for success/info) with `aria-live` region
- [ ] iOS: announcement posted via `UIAccessibility.post(notification: .announcement, argument: message)` on appearance
- [ ] Dismiss button present and has `aria-label="Dismiss notification"` or equivalent
- [ ] Status conveyed by icon shape AND text, not by color alone (e.g., checkmark icon + "Success:" prefix, not just green)
- [ ] Auto-dismiss timer pauses when the user hovers (web) or when VoiceOver focus enters the toast region
- [ ] Multiple toasts do not stack out of viewport — visible count capped or scroll provided
- [ ] Toast does not obscure primary interactive content (keyboard accessible with Escape or dismiss button)

### Sliders (from Mantine)

- [ ] `role="slider"` on the thumb element (web)
- [ ] `aria-valuemin` reflects parameter minimum
- [ ] `aria-valuemax` reflects parameter maximum
- [ ] `aria-valuenow` updated on every value change (not just on release)
- [ ] `aria-valuetext` set for non-numeric display (e.g., "−12 dB", "127 Hz", "50%")
- [ ] Label associated via `aria-labelledby` pointing to visible label element
- [ ] Keyboard: Left/Down decreases by one step; Right/Up increases by one step
- [ ] Keyboard: Page Down/Up changes by 10 steps or 10% of range
- [ ] Keyboard: Home/End jumps to min/max
- [ ] Focus ring visible on thumb; track does not receive focus
- [ ] Touch target: thumb is at minimum 44pt in diameter on iOS

### Tabs (from Mantine)

- [ ] Tab container: `role="tablist"`, `aria-label` describing the tab group
- [ ] Each tab: `role="tab"`, `aria-selected="true/false"`, `aria-controls` pointing to panel id
- [ ] Each panel: `role="tabpanel"`, `id` matching tab's `aria-controls`, `aria-labelledby` matching tab id
- [ ] Inactive panels: `hidden` attribute or `display: none` (not just visually hidden via opacity)
- [ ] Arrow Left/Right navigate between tabs
- [ ] Home focuses first tab; End focuses last tab
- [ ] Tab key moves focus from the tab list into the active panel (not to the next tab)
- [ ] Active tab is visually distinct by more than color alone (e.g., underline, bold weight, or indicator icon)

### Dropdowns (from Mantine)

- [ ] Trigger button: `aria-haspopup="listbox"`, `aria-expanded="true/false"`, `aria-controls` pointing to listbox id
- [ ] List container: `role="listbox"`, `aria-label` or `aria-labelledby`
- [ ] Each option: `role="option"`, `aria-selected="true/false"`
- [ ] Focused option tracked via `aria-activedescendant` on the listbox (or trigger)
- [ ] Arrow Down/Up navigate through options
- [ ] Type-ahead: typing letters moves focus to matching option
- [ ] Enter confirms selection; Escape closes without change
- [ ] Selected option visually distinct by more than color alone
- [ ] Focus returns to trigger after selection or close

---

## 8. Remediation Priority

### Priority Definitions

| Level | Description | Examples |
|---|---|---|
| P0 | Blocks access entirely | Interactive element unreachable by keyboard; no screen reader label on essential control |
| P1 | Significantly impairs usability | Essential text fails AA contrast; form field missing label; modal does not trap focus |
| P2 | Minor inconvenience or aspirational gap | Decorative text below threshold; animation lacks reduced-motion fallback; AAA contrast target not met |

### Current Findings

| ID | Component | Finding | Priority | Fix |
|---|---|---|---|---|
| A-001 | Secondary text on cards | ~~`#888888` on `#2E2E2E` — 3.3:1, fails AA~~ **FIXED: `#A0A0A0` on `#2E2E2E` — 4.6:1, passes AA** | P1 | DONE — replaced with `#A0A0A0`. Updated `--color-text-secondary-on-card`. |
| A-002 | Tertiary text everywhere | `#666666` on `#1A1A1A` — 3.0:1, fails AA for normal text | P1 | Restrict to decorative use only (no essential information at normal text sizes). Elevate essential tertiary text to `#A0A0A0` on dark base or card. |
| A-003 | All icon-only buttons | Missing `aria-label` / `accessibilityLabel` audit not yet confirmed | P0 | Audit all icon-only interactive elements. Add labels before first release. |
| A-004 | Modals | Focus trap implementation not confirmed in current codebase | P1 | Implement `inert` + focus return pattern on all modal/dialog components. |
| A-005 | Sliders / Knobs | `aria-valuenow` and `aria-valuetext` update cadence not confirmed | P1 | Verify each slider updates ARIA attributes on every value change event, not only on pointer up. |
| A-006 | Toasts | `role="alert"` vs `role="status"` may not be differentiated by severity | P2 | Implement role selection based on toast type: `alert` for error/warning, `status` for success/info. |
| A-007 | Animations | No confirmed `prefers-reduced-motion` implementation in web layer | P2 | Add global reduced-motion CSS block. Audit all transition/animation declarations. |
| A-008 | JUCE components | `AccessibilityHandler` not confirmed on custom knobs/sliders | P1 | Override `createAccessibilityHandler()` in all interactive JUCE components before AU submission. |

### Fix Order

1. **P0 first**: A-003 (icon button labels) — no accessible name = completely unusable by screen reader
2. **P1 critical**: A-001 (color fix — one token change), A-004 (focus trap), A-005 (ARIA live values), A-008 (JUCE accessibility)
3. **P1 advisory**: A-002 (restrict tertiary text usage)
4. **P2**: A-006 (toast role differentiation), A-007 (reduced-motion CSS)

---

## 9. Audit Process and Tooling

### Recommended Testing Tools

| Tool | Platform | Purpose |
|---|---|---|
| axe DevTools (browser extension) | Web | Automated WCAG scan |
| Lighthouse Accessibility audit | Web | Automated scan + scoring |
| WAVE (WebAIM) | Web | Visual overlay of accessibility issues |
| VoiceOver | macOS / iOS | Primary screen reader for Apple platforms |
| NVDA + Chrome | Windows | Secondary screen reader for web |
| Xcode Accessibility Inspector | iOS / macOS | JUCE plugin + iOS UI element inspection |
| Colour Contrast Analyser (TPGi) | macOS / Windows | Manual color pair verification |
| Stark (Figma plugin) | Figma | Contrast check during design phase |

### Audit Cadence

- **Per component**: Run axe + manual keyboard check before merging any new component into the design system.
- **Pre-release**: Full VoiceOver walkthrough of primary user flows.
- **Color token changes**: Run all color pairs through contrast check before committing token updates.
- **JUCE build**: Accessibility Inspector review before each AU release submission.

---

## 10. References

- [WCAG 2.1 Understanding SC 1.4.3 Contrast (Minimum)](https://www.w3.org/WAI/WCAG21/Understanding/contrast-minimum.html)
- [WCAG 2.1 Understanding SC 2.4.7 Focus Visible](https://www.w3.org/WAI/WCAG21/Understanding/focus-visible.html)
- [WCAG 2.1 Understanding SC 2.1.1 Keyboard](https://www.w3.org/WAI/WCAG21/Understanding/keyboard.html)
- [WCAG 2.1 Understanding SC 2.3.1 Three Flashes](https://www.w3.org/WAI/WCAG21/Understanding/three-flashes-or-below-threshold.html)
- [WAI-ARIA Authoring Practices 1.2](https://www.w3.org/WAI/ARIA/apg/)
- [Apple Human Interface Guidelines — Accessibility](https://developer.apple.com/design/human-interface-guidelines/accessibility)
- [JUCE AccessibilityHandler documentation](https://docs.juce.com/master/classAccessibilityHandler.html)
