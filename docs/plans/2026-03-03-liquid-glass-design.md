# Liquid Glass Material — Design Document

**Date:** 2026-03-03
**Status:** Approved
**Scope:** Cherry-pick Core Glass from the XO_OX 2026 Manifesto into the existing 20-theme system

## Context

The XO_OX 2026 System Manifesto proposes a complete Liquid Glass visual identity. After evaluating the full spec against 6 phases of existing work (20 themes, 70+ components, Zustand stores, canvas waveforms), we chose to **selectively cherry-pick** the Core Glass material (Levels 1+3+4) as opt-in CSS utilities that layer over the existing theme system.

**What we adopt:** `backdrop-filter` blur, specular highlights, depth shadows.
**What we skip (for now):** Environment tint layer, spring physics, 5-accent color system, dark-only mode, new hardware components (Knob, Fader, TransportBar, etc.).

## Design Decisions

### 1. Glass as Opt-In CSS Utilities (Not Theme Replacement)

Glass material is delivered as CSS utility classes (`.glass-panel`, `.glass-raised`, `.glass-sunken`) that can be added to any component without modifying the theme system. The glass tints itself from the current theme's `--color-surface` variable, so all 20 themes gain glass depth automatically.

### 2. New CSS Custom Properties

```css
:root {
  --glass-blur: 40px;
  --glass-saturate: 180%;
  --glass-contrast: 110%;
  --glass-bg-alpha: 0.6;
  --glass-specular-alpha: 0.1;
  --glass-depth-alpha: 0.4;
}

[data-theme-mode='dark'] {
  --glass-bg-alpha: 0.4;
}
```

Dark themes get more transparency (alpha 0.4 vs 0.6) because glass looks best when there's contrast behind it.

### 3. Utility Classes

```css
.glass-panel {
  backdrop-filter: blur(var(--glass-blur)) saturate(var(--glass-saturate)) contrast(var(--glass-contrast));
  -webkit-backdrop-filter: blur(var(--glass-blur)) saturate(var(--glass-saturate)) contrast(var(--glass-contrast));
  background: rgb(var(--color-surface) / var(--glass-bg-alpha));
  border: 1px solid rgb(255 255 255 / 0.06);
  box-shadow: inset 0 1px 1px rgb(255 255 255 / var(--glass-specular-alpha)),
              inset 0 -1px 1px rgb(0 0 0 / var(--glass-depth-alpha));
}

.glass-panel .glass-panel {
  --glass-blur: 20px;
  --glass-bg-alpha: calc(var(--glass-bg-alpha) * 0.7);
}

.glass-panel .glass-panel .glass-panel {
  --glass-blur: 10px;
  --glass-bg-alpha: calc(var(--glass-bg-alpha) * 0.5);
}

.glass-raised {
  box-shadow: 0 8px 32px rgba(0,0,0,0.5),
              inset 0 1px 1px rgba(255,255,255,0.08);
}

.glass-sunken {
  box-shadow: inset 0 2px 8px rgba(0,0,0,0.6),
              inset 0 1px 2px rgba(0,0,0,0.4);
}
```

### 4. Tailwind Tokens

Add to `tailwind.config.ts`:
```ts
boxShadow: {
  'glass-inset':  'inset 0 1px 1px rgba(255,255,255,0.1), inset 0 -1px 1px rgba(0,0,0,0.4)',
  'glass-raised': '0 8px 32px rgba(0,0,0,0.5), inset 0 1px 1px rgba(255,255,255,0.08)',
  'glass-sunken': 'inset 0 2px 8px rgba(0,0,0,0.6), inset 0 1px 2px rgba(0,0,0,0.4)',
}
```

### 5. Component Application Map

**Tier 1 — Maximum Impact (implement first):**
- Modal dialog panel
- CommandPalette panel
- ContextMenu panel

**Tier 2 — Structural Depth:**
- Sidebar navigation
- Detail Panel (right aside)
- Header bar
- StatusBar

**Tier 3 — Selective Accent (evaluate after T1+T2):**
- Card (elevated variant only)
- Toast notifications
- Tooltips (reduced blur: 10px)

**Stays Solid (no glass):**
- PadCell — tactile hardware feel requires opaque surfaces
- Waveform canvas — canvas rendering incompatible with backdrop-filter
- Input fields — readability over aesthetics
- Buttons — solid fills for clear affordance

### 6. Performance Safeguards

1. `will-change: backdrop-filter` on active glass surfaces
2. Maximum 3 simultaneous glass surfaces (architectural constraint)
3. Performance opt-out: `layoutStore.glassEnabled` boolean (default true)
4. Auto-detection on first load: if frame time > 20ms with glass, auto-disable

### 7. Accessibility

**Reduced motion:**
```css
@media (prefers-reduced-motion: reduce) {
  .glass-panel {
    backdrop-filter: none;
    -webkit-backdrop-filter: none;
    background: rgb(var(--color-surface) / 0.95);
  }
}
```

**Browser compatibility:**
```css
@supports not (backdrop-filter: blur(1px)) {
  .glass-panel {
    background: rgb(var(--color-surface) / 0.95);
  }
}
```

### 8. Integration with Existing Systems

- **Theme store:** No changes needed. Glass reads from existing `--color-surface` CSS variable.
- **`applyThemeToDOM()`:** Already sets `data-theme-mode` attribute, which the glass alpha uses.
- **TextureOverlay:** Texture renders above glass (z-9999), creating a dust-on-glass effect.
- **20 themes:** All work automatically. Each theme's surface color tints the glass uniquely.

## Implementation Order

1. Add glass CSS custom properties and utility classes to `globals.css`
2. Add glass shadow tokens to `tailwind.config.ts`
3. Add `glassEnabled` toggle to `layoutStore.ts`
4. Apply glass to Tier 1 components (Modal, CommandPalette, ContextMenu)
5. Apply glass to Tier 2 components (Sidebar, Panel, Header, StatusBar)
6. Test across all 20 themes
7. Add performance auto-detection
8. Evaluate Tier 3 components

## Files Modified

- `src/app/globals.css` — glass custom properties + utility classes
- `tailwind.config.ts` — glass shadow tokens
- `src/stores/layoutStore.ts` — `glassEnabled` toggle
- `src/components/ui/Modal.tsx` — glass panel class
- `src/components/ui/CommandPalette.tsx` — glass panel class
- `src/components/ui/ContextMenu.tsx` — glass panel class
- `src/components/layout/Sidebar.tsx` — glass panel class
- `src/components/layout/WorkspaceLayout.tsx` — glass panel on aside
- `src/components/layout/Header.tsx` — glass panel class
- `src/components/layout/StatusBar.tsx` — glass panel class
