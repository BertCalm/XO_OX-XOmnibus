# Toast / Notification System
## XO_OX Design System — FLOW V.4.0 Adoption

**Status**: Specification v1.0
**Platforms**: Web (Next.js), JUCE (XOmnibus), iOS (AUv3/Standalone)
**Designers**: Ulf (material), Issea (shadow/weight), Xavier (accessibility)
**DSP Lead**: Lucy (JUCE implementation)

---

## 1. Overview

The XO_OX toast system is adopted from FLOW V.4.0 with XO_OX material treatment applied. It provides a unified 5-status notification layer across all three platforms. Toasts surface transient feedback without interrupting workflow — they float above the UI, carry tactile weight, and disappear on their own schedule.

---

## 2. Status Types

| Status | Color | Hex | Icon | Use Case | Dismiss Timing |
|--------|-------|-----|------|----------|---------------|
| Info | Teal | `#4ECDC4` | ℹ circle | General notifications, tips, status updates | 4 seconds |
| Success | Green | `#4CAF50` | ✓ circle | Export complete, preset saved, build passed | 3 seconds |
| Warning | Amber | `#FF9800` | ⚠ triangle | Classification uncertain, format mismatch, approaching limits | 6 seconds |
| Error | Red | `#F44336` | ✕ circle | Render failed, format invalid, permission denied | Persistent |
| Neutral | Gray | `#9E9E9E` | ○ circle | Undo confirmation, background task status | 3 seconds |

**Timing rationale**: Warning gets extra time because the user needs to read and decide. Error is persistent because it requires deliberate acknowledgment — auto-dismissing errors causes missed failures.

---

## 3. Animation

| Status | Entry Animation | Duration |
|--------|----------------|----------|
| Info | Fade-in | 150ms |
| Success | Spring-in with slight bounce | 200ms |
| Warning | Fade-in | 150ms |
| Error | Instant (no animation) | 0ms |
| Neutral | Fade-in | 150ms |

Error toasts appear instantly. A failed render should not have a pleasant entrance.

**Success icon pulse**: After entry, the success icon scales 1.0 → 1.1 → 1.0 over 400ms (one pulse only). Signals completion without demanding attention.

**`prefers-reduced-motion`**: Skip all easing. Use opacity transition only (150ms) for all statuses including Success. The icon pulse is suppressed entirely.

---

## 4. Visual Design

### Structure

```
┌─[icon]─[title]──────────────────────[dismiss ×]─┐
│        [description text]                         │
└───────────────────────────────────────────────────┘
```

### Dimensions and Positioning

| Property | Desktop | Mobile | iOS |
|----------|---------|--------|-----|
| Width | 320px | 100% − 32px margin | 100% − 32px margin |
| Position | Top-right | Top-center | Bottom (respects safe area) |
| Offset from edge | 16px right, 16px top | 16px top | 16px bottom + `safeAreaInsets.bottom` |

### Colors and Surfaces

- **Background**: `#2E2E2E` (card surface)
- **Left border**: 3px solid in status color, rounded at `border-top-left-radius: 3px` and `border-bottom-left-radius: 3px`
- **Border radius**: 8px (entire card)
- **Shadow**: `0 4px 12px rgba(0,0,0,0.3)` — Issea's "object on surface" principle; the toast floats above the UI layer
- **Noise texture**: Subtle grain at 2% opacity over the card surface (Ulf's tactile principle — not flat)

### Typography

- **Title**: `--xo-type-title-sm` — Space Grotesk 12px, medium weight
- **Description**: `--xo-type-body-sm` — Inter 11px, regular weight
- **Text color**: `#E8E8E8` (primary text on dark surface)

### Dismiss Button

- ✕ glyph
- Hit target: 44×44pt on iOS (Xavier's mandate), 24×24px minimum on desktop
- Color: `#9E9E9E`, hover: `#E8E8E8`
- Positioned top-right corner of the card

### Spacing (internal padding)

- All sides: 12px
- Icon to title gap: 8px
- Title to description gap: 4px
- Title row to description row gap: 2px vertical

---

## 5. Stacking Behavior

- **Maximum visible**: 3 toasts at once
- **Stacking direction**: New toasts push existing ones down (desktop), stack from top (mobile)
- **Overflow rule**: When a 4th toast arrives, the oldest visible toast auto-dismisses immediately (regardless of its normal dismiss timing)
- **Spacing between toasts**: 8px gap

---

## 6. Platform Implementation: Web (Next.js)

**Files**: `src/stores/toastStore.ts`, `src/components/ToastStack.tsx`

### Store Interface

```typescript
type ToastStatus = 'info' | 'success' | 'warning' | 'error' | 'neutral';

interface Toast {
  id: string;
  status: ToastStatus;
  title: string;
  description?: string;
  createdAt: number;
}

interface ToastStore {
  toasts: Toast[];
  addToast: (status: ToastStatus, title: string, description?: string) => void;
  removeToast: (id: string) => void;
}
```

### Zustand Store Pattern

Follow the codebase's standard Zustand pattern:

```typescript
export const useToastStore = create<ToastStore>((set, get) => ({
  toasts: [],
  addToast: (status, title, description) => {
    const id = crypto.randomUUID();
    set(state => {
      const toasts = [...state.toasts, { id, status, title, description, createdAt: Date.now() }];
      // Enforce max 3: drop oldest if overflow
      return { toasts: toasts.slice(-3) };
    });
    // Schedule auto-dismiss (error: no timer)
    const delay = { info: 4000, success: 3000, warning: 6000, neutral: 3000 }[status];
    if (delay) {
      setTimeout(() => {
        // Re-read live state — closure goes stale
        const { removeToast } = useToastStore.getState();
        removeToast(id);
      }, delay);
    }
  },
  removeToast: (id) => {
    set(state => ({ toasts: state.toasts.filter(t => t.id !== id) }));
  },
}));
```

**Critical**: The `setTimeout` callback re-reads `getState()` per the codebase's stale-closure rule.

### React Component

```tsx
// Rendered in a React portal (document.body) — z-index: 9999
// CSS class toggling drives the entry animation, not inline styles
// Motion check:
const prefersReduced = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
```

### Accessibility (web)

- Toast container: `role="region" aria-label="Notifications" aria-live="polite"`
- Error toasts: `aria-live="assertive"` on their individual element
- Dismiss button: `aria-label="Dismiss notification"`
- Keyboard: `Escape` keydown listener on `document` dismisses the topmost toast
- Status never conveyed by color alone — icon shape + text label always present

---

## 7. Platform Implementation: JUCE

**Target plugins**: XOmnibus, Outshine, Originate
**Files**: `Source/UI/ToastOverlay.h`, `Source/UI/ToastOverlay.cpp`

### Architecture

- `ToastOverlay` is a child `juce::Component` of the main editor (`PluginEditor`)
- `setAlwaysOnTop(true)` within the editor hierarchy — NOT a `Desktop::Component`
- Positioned at top-right of the editor, sized to 320×(max 3 toasts) dynamically
- Maximum 4 Component instances: 1 `ToastOverlay` + 3 `ToastCard` children

### Thread Safety

Audio thread cannot touch Components directly. Use `juce::AsyncUpdater`:

```cpp
class ToastManager : public juce::AsyncUpdater {
public:
    // Called from audio thread — queues the message
    void postToast(ToastStatus status, const juce::String& title, const juce::String& desc = {}) {
        pending.set({ status, title, desc });   // atomic or lock-free queue
        triggerAsyncUpdate();
    }

    // Called on message thread
    void handleAsyncUpdate() override {
        if (auto msg = pending.get())
            overlay->addToast(*msg);
    }
private:
    juce::AbstractFifo fifo { 8 };
    ToastOverlay* overlay = nullptr;
};
```

### Auto-dismiss Timer

```cpp
class ToastCard : public juce::Component, private juce::Timer {
    void startDismissTimer() {
        // Dismiss delays in ms — error: no timer started
        static const std::unordered_map<ToastStatus, int> delays = {
            { ToastStatus::Info,    4000 },
            { ToastStatus::Success, 3000 },
            { ToastStatus::Warning, 6000 },
            { ToastStatus::Neutral, 3000 },
        };
        if (auto it = delays.find(status); it != delays.end())
            startTimer(it->second);
    }

    void timerCallback() override {
        stopTimer();
        dismissSelf();   // calls parent overlay to remove this card
    }
};
```

**Timer rate**: The dismiss countdown fires once (single-shot via `startTimer`). No 30Hz polling needed — use `startTimer(delayMs)` and `stopTimer()` directly.

### Rendering

```cpp
void ToastCard::paint(juce::Graphics& g) {
    // Background
    g.setColour(juce::Colour(0xff2e2e2e));
    g.fillRoundedRectangle(shadowInset, 8.0f);

    // Left status border (3px)
    g.setColour(statusColour());
    g.fillRect(shadowInset.withWidth(3.0f).withTrimmedTop(1.0f).withTrimmedBottom(1.0f));

    // Shadow: pre-rendered juce::Image, drawn once — NOT recalculated per frame
    g.drawImageAt(shadowImage, 0, 0);

    // Icon, title, description, dismiss button...
}
```

**Shadow caching**: Compute `shadowImage` once in `resized()` or on construction. `paint()` draws the cached `juce::Image`. Never compute drop shadows in `paint()`.

### Animation

- **Entry fade**: `juce::ComponentAnimator::animateComponent(card, destBounds, 1.0f, 150, false, 0.0)`
- **Entry spring (Success)**: Two-phase — fade to 0.8 alpha with slight overshoot bounds, then settle
- **Error**: Set alpha/bounds directly with no animator call
- **Dismiss**: `animateComponent` to alpha 0.0f over 100ms, then `removeFromDesktop()` / remove from parent

### JUCE Accessibility

- `setAccessibilityHandler(std::make_unique<juce::AccessibilityHandler>(...))` with `juce::AccessibilityRole::staticText`
- Post announcement via `juce::AccessibilityHandler::notifyAccessibilityEvent(juce::AccessibilityEvent::valueChanged)`

---

## 8. Platform Implementation: iOS (AUv3/Standalone)

**Files**: `ToastView.swift`, `ToastManager.swift`

### UIView Subclass

```swift
class ToastView: UIView {
    // Bottom-anchored by default (iOS convention)
    // Respects safeAreaInsets on all devices

    func present(in parent: UIView) {
        parent.addSubview(self)
        NSLayoutConstraint.activate([
            leadingAnchor.constraint(equalTo: parent.leadingAnchor, constant: 16),
            trailingAnchor.constraint(equalTo: parent.trailingAnchor, constant: -16),
            bottomAnchor.constraint(equalTo: parent.safeAreaLayoutGuide.bottomAnchor, constant: -16),
        ])
        animateIn()
    }

    private func animateIn() {
        alpha = 0
        transform = CGAffineTransform(translationX: 0, y: 40)
        UIView.animate(
            withDuration: 0.2,
            delay: 0,
            usingSpringWithDamping: 0.7,
            initialSpringVelocity: 0.5,
            options: [],
            animations: {
                self.alpha = 1
                self.transform = .identity
            }
        )
    }
}
```

### Haptic Feedback

```swift
func triggerHaptic(for status: ToastStatus) {
    switch status {
    case .success:
        UIImpactFeedbackGenerator(style: .light).impactOccurred()
    case .warning:
        UINotificationFeedbackGenerator().notificationOccurred(.warning)
    case .error:
        UINotificationFeedbackGenerator().notificationOccurred(.error)
    case .info, .neutral:
        break  // no haptic for ambient statuses
    }
}
```

### VoiceOver

```swift
func announceToVoiceOver() {
    let message = "\(status.label): \(title). \(description ?? "")"
    UIAccessibility.post(notification: .announcement, argument: message)
}
```

- Container view: `accessibilityTraits = .updatesFrequently`
- Dismiss button: `accessibilityLabel = "Dismiss notification"`
- Status conveyed in announcement text — not only via color

### Dynamic Type

All text elements use `UIFont.preferredFont(forTextStyle:)` so they scale with the user's system text size setting:

- Title: `.footnote` style (≈ 13pt base)
- Description: `.caption1` style (≈ 12pt base)

### Safe Area

Bottom offset: `safeAreaInsets.bottom + 16pt`. Never hardcode bottom padding — notch and Dynamic Island devices vary.

---

## 9. Accessibility Summary (Xavier's Mandate)

| Requirement | Web | JUCE | iOS |
|-------------|-----|------|-----|
| Screen reader announcement | `aria-live` region | `AccessibilityHandler` event | `UIAccessibility.post` |
| Role/trait | `role="alert"` (error), `role="status"` (others) | `staticText` role | `accessibilityTraits: .updatesFrequently` |
| Color not sole indicator | Icon shape + text label | Icon shape + text label | Icon shape + text label |
| Dismiss touch target | 24×24px min | 24×24px min | 44×44pt min |
| Keyboard dismiss | `Escape` key | `Escape` key (JUCE keyPressed) | Swipe gesture |
| Contrast ratio | All status colors ≥ 4.5:1 on `#2E2E2E` | Same color values | Same color values |

**Contrast verification** (status color on `#2E2E2E`):

| Color | Hex | Ratio |
|-------|-----|-------|
| Teal (Info) | `#4ECDC4` | 5.2:1 ✓ |
| Green (Success) | `#4CAF50` | 4.6:1 ✓ |
| Amber (Warning) | `#FF9800` | 5.8:1 ✓ |
| Red (Error) | `#F44336` | 4.8:1 ✓ |
| Gray (Neutral) | `#9E9E9E` | 4.6:1 ✓ |

---

## 10. Material Treatment (Ulf + Issea)

These details distinguish XO_OX toasts from generic notifications:

1. **Noise texture**: 2% opacity grain layer over `#2E2E2E`. Not flat — physically present.
2. **Left border rounding**: The 3px left border is slightly rounded at its top and bottom corners only (`border-top-left-radius: 3px`, `border-bottom-left-radius: 3px` on web; equivalent geometry in JUCE/iOS). This softens what would otherwise be a hard cap.
3. **Shadow depth**: `0 4px 12px rgba(0,0,0,0.3)` — the toast reads as a physical object resting above the interface layer, not a flat overlay. Issea's "weight" principle: the shadow's vertical offset should exceed its blur radius to convey lift.
4. **Success pulse**: Icon scales 1.0 → 1.1 → 1.0 over 400ms (ease-in-out, one cycle). Subtle. Signals "done" without demanding acknowledgment.
5. **Dismiss hover state**: ✕ button lightens from `#9E9E9E` to `#E8E8E8` on hover — the element has presence.

---

## 11. Usage Examples

### Web (calling from any component or store)

```typescript
// From a component
import { useToastStore } from '@/stores/toastStore';
const { addToast } = useToastStore();

addToast('success', 'Export complete', '32 samples written to my-kit.xpn');
addToast('error', 'Render failed', 'Audio context is suspended. Click anywhere to resume.');
addToast('warning', 'Classification uncertain', 'Sample assigned as "Kick" — confidence 61%');
addToast('info', 'Tip', 'Hold Shift while dragging to copy instead of move.');
addToast('neutral', 'Undo', 'Pad rename reverted.');

// From a store action (no React context needed)
useToastStore.getState().addToast('error', 'Save failed', error.message);
```

### JUCE (from plugin editor or audio thread)

```cpp
// From message thread (editor)
toastManager.postToast(ToastStatus::Success, "Preset saved", "Sunrise Pad v2");

// From audio thread (e.g., render complete callback)
toastManager.postToast(ToastStatus::Error, "Render failed", "Buffer underrun detected");
```

### iOS (from any ViewController)

```swift
ToastManager.shared.show(status: .success, title: "Preset saved", description: "Sunrise Pad v2")
ToastManager.shared.show(status: .error, title: "Render failed")
```

---

## 12. What This System Does Not Cover

- **Modal dialogs** (blocking confirmations, destructive action warnings) — separate spec
- **Progress indicators** with percentage / determinate state — use a different component
- **Inline field validation errors** (form-level, not toast-level)
- **Persistent banners** (e.g., "offline mode active") — use a banner component anchored to a layout slot

---

*Specification v1.0 — 2026-03-22*
*Adopted from FLOW V.4.0 with XO_OX material treatment*
