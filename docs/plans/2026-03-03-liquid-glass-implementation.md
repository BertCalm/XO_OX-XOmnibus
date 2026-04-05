# Liquid Glass Material — Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add Liquid Glass material as opt-in CSS utility classes that layer over the existing 20-theme system, giving every surface glass depth, specular highlights, and backdrop blur without modifying any themes.

**Architecture:** Glass is purely additive — CSS custom properties + 3 utility classes (`.glass-panel`, `.glass-raised`, `.glass-sunken`) in `globals.css`, shadow tokens in `tailwind.config.ts`, a `glassEnabled` toggle in `layoutStore`, and `className` additions to 7 existing components. All 20 themes work automatically because glass reads `--color-surface`.

**Tech Stack:** CSS `backdrop-filter`, CSS Custom Properties (space-separated RGB), Tailwind CSS, Zustand (persist middleware), React/Next.js

**Design Document:** `docs/plans/2026-03-03-liquid-glass-design.md`

---

## Task 1: Add Glass CSS Custom Properties to globals.css

**Files:**
- Modify: `src/app/globals.css`

**Step 1: Add glass custom properties inside the `:root` block**

Open `src/app/globals.css`. Inside the `@layer base { :root { ... } }` block, after the closing `*/` of `--shadow-modal`, add these glass properties:

```css
    /* ── Liquid Glass Material ─────────────────────────────────── */
    --glass-blur: 40px;
    --glass-saturate: 180%;
    --glass-contrast: 110%;
    --glass-bg-alpha: 0.6;
    --glass-specular-alpha: 0.1;
    --glass-depth-alpha: 0.4;
```

The existing `:root` block currently ends after `--shadow-modal`. Insert the glass block right before the closing `}` of `:root`.

**Step 2: Add dark-mode glass alpha override**

After the `:root` closing `}` (still inside `@layer base`), before the `body` rule, add:

```css
  [data-theme-mode='dark'] {
    --glass-bg-alpha: 0.4;
  }
```

This makes dark themes more transparent (0.4 vs 0.6) for better glass depth.

**Step 3: Verify**

Run: `npx tsc --noEmit`
Expected: PASS — CSS-only change, no type impact.

**Step 4: Commit**

```bash
git add src/app/globals.css
git commit -m "feat(glass): add glass CSS custom properties to :root"
```

---

## Task 2: Add Glass Utility Classes to globals.css

**Files:**
- Modify: `src/app/globals.css`

**Step 1: Add glass utility classes in the `@layer components` block**

In `src/app/globals.css`, inside `@layer components { ... }`, after the existing `.pad-cell-celebrate` class (around line 137), add these glass utilities:

```css
  /* ── Liquid Glass Utilities ────────────────────────────────── */

  .glass-panel {
    backdrop-filter: blur(var(--glass-blur)) saturate(var(--glass-saturate)) contrast(var(--glass-contrast));
    -webkit-backdrop-filter: blur(var(--glass-blur)) saturate(var(--glass-saturate)) contrast(var(--glass-contrast));
    background: rgb(var(--color-surface) / var(--glass-bg-alpha));
    border: 1px solid rgb(255 255 255 / 0.06);
    box-shadow: inset 0 1px 1px rgb(255 255 255 / var(--glass-specular-alpha)),
                inset 0 -1px 1px rgb(0 0 0 / var(--glass-depth-alpha));
    will-change: backdrop-filter;
  }

  /* Nesting: inner glass panels reduce blur + transparency */
  .glass-panel .glass-panel {
    --glass-blur: 20px;
    --glass-bg-alpha: 0.42;
  }

  .glass-panel .glass-panel .glass-panel {
    --glass-blur: 10px;
    --glass-bg-alpha: 0.3;
  }

  .glass-raised {
    box-shadow: 0 8px 32px rgba(0, 0, 0, 0.5),
                inset 0 1px 1px rgba(255, 255, 255, 0.08);
  }

  .glass-sunken {
    box-shadow: inset 0 2px 8px rgba(0, 0, 0, 0.6),
                inset 0 1px 2px rgba(0, 0, 0, 0.4);
  }
```

**Step 2: Add reduced-motion and browser fallbacks**

At the end of `globals.css` (after the scrollbar utilities `@layer utilities` block), add:

```css
/* ------------------------------------------------------------------ */
/* Liquid Glass — Accessibility & Browser Fallbacks                     */
/* ------------------------------------------------------------------ */

@media (prefers-reduced-motion: reduce) {
  .glass-panel {
    backdrop-filter: none;
    -webkit-backdrop-filter: none;
    background: rgb(var(--color-surface) / 0.95);
  }
}

@supports not (backdrop-filter: blur(1px)) {
  .glass-panel {
    background: rgb(var(--color-surface) / 0.95);
  }
}

/* Glass opt-out — applied when layoutStore.glassEnabled is false */
[data-glass-disabled='true'] .glass-panel {
  backdrop-filter: none;
  -webkit-backdrop-filter: none;
  background: rgb(var(--color-surface) / 0.95);
  box-shadow: var(--shadow-card);
}
```

**Step 3: Verify**

Run: `npx tsc --noEmit`
Expected: PASS — CSS-only change.

**Step 4: Commit**

```bash
git add src/app/globals.css
git commit -m "feat(glass): add glass-panel, glass-raised, glass-sunken utility classes"
```

---

## Task 3: Add Glass Shadow Tokens to Tailwind Config

**Files:**
- Modify: `tailwind.config.ts`

**Step 1: Add glass shadow tokens to the `boxShadow` extend object**

In `tailwind.config.ts`, locate the `boxShadow` object inside `theme.extend` (currently lines 53-57). Add the three glass tokens:

Change from:
```ts
      boxShadow: {
        card: 'var(--shadow-card)',
        elevated: 'var(--shadow-elevated)',
        modal: 'var(--shadow-modal)',
      },
```

To:
```ts
      boxShadow: {
        card: 'var(--shadow-card)',
        elevated: 'var(--shadow-elevated)',
        modal: 'var(--shadow-modal)',
        'glass-inset': 'inset 0 1px 1px rgba(255,255,255,0.1), inset 0 -1px 1px rgba(0,0,0,0.4)',
        'glass-raised': '0 8px 32px rgba(0,0,0,0.5), inset 0 1px 1px rgba(255,255,255,0.08)',
        'glass-sunken': 'inset 0 2px 8px rgba(0,0,0,0.6), inset 0 1px 2px rgba(0,0,0,0.4)',
      },
```

This lets components use `shadow-glass-raised` etc. via Tailwind when needed alongside the CSS utility classes.

**Step 2: Verify**

Run: `npx tsc --noEmit`
Expected: PASS — config is JS/TS, no import changes.

**Step 3: Commit**

```bash
git add tailwind.config.ts
git commit -m "feat(glass): add glass shadow tokens to tailwind config"
```

---

## Task 4: Add `glassEnabled` Toggle to layoutStore

**Files:**
- Modify: `src/stores/layoutStore.ts`

**Step 1: Add `glassEnabled` to the interface and store**

In `src/stores/layoutStore.ts`, update the `LayoutState` interface to add:

```ts
  /** Whether Liquid Glass material effects are enabled (default true) */
  glassEnabled: boolean;
  setGlassEnabled: (enabled: boolean) => void;
```

Add after the `panelWidth` and `setPanelWidth` lines.

Then in the `create` initializer, add the default + setter:

```ts
      glassEnabled: true,

      setGlassEnabled: (enabled) => set({ glassEnabled: enabled }),
```

Add these after the `setPanelWidth` implementation.

Also update `resetLayout` to include resetting glass:

Change from:
```ts
      resetLayout: () =>
        set({ sidebarWidth: DEFAULT_SIDEBAR_WIDTH, panelWidth: DEFAULT_PANEL_WIDTH }),
```

To:
```ts
      resetLayout: () =>
        set({ sidebarWidth: DEFAULT_SIDEBAR_WIDTH, panelWidth: DEFAULT_PANEL_WIDTH, glassEnabled: true }),
```

**Step 2: Verify**

Run: `npx tsc --noEmit`
Expected: PASS — new fields have defaults, no consumers break.

**Step 3: Commit**

```bash
git add src/stores/layoutStore.ts
git commit -m "feat(glass): add glassEnabled toggle to layoutStore"
```

---

## Task 5: Wire `data-glass-disabled` Attribute on Document Root

**Files:**
- Modify: `src/app/page.tsx`

**Step 1: Add glass-disabled attribute to the root layout**

In `src/app/page.tsx`, import `useLayoutStore`:

```ts
import { useLayoutStore } from '@/stores/layoutStore';
```

Then inside the main component function, read the glass toggle:

```ts
const glassEnabled = useLayoutStore((s) => s.glassEnabled);
```

And apply a `data-glass-disabled` attribute to the outermost wrapping `<div>` of the page:

Find the outermost `<div>` returned by the component and add:

```tsx
data-glass-disabled={!glassEnabled ? 'true' : undefined}
```

This activates the CSS fallback from Task 2 (`[data-glass-disabled='true'] .glass-panel`).

**Step 2: Verify**

Run: `npx tsc --noEmit`
Expected: PASS.

**Step 3: Commit**

```bash
git add src/app/page.tsx
git commit -m "feat(glass): wire data-glass-disabled attribute from layoutStore"
```

---

## Task 6: Apply Glass to Modal (Tier 1)

**Files:**
- Modify: `src/components/ui/Modal.tsx`

**Step 1: Add `glass-panel glass-raised` classes to the dialog**

In `src/components/ui/Modal.tsx`, locate the dialog `<div>` (line 106). It currently has:

```tsx
className={`${sizeClasses[size]} w-full bg-surface rounded-2xl border border-border
  shadow-modal p-6 animate-in zoom-in-95 duration-200`}
```

Change to:

```tsx
className={`${sizeClasses[size]} w-full glass-panel glass-raised rounded-2xl
  p-6 animate-in zoom-in-95 duration-200`}
```

The `glass-panel` class replaces `bg-surface border border-border shadow-modal` because it provides its own background (with alpha), border (white 6% edge), and inset shadow (specular + depth). The `glass-raised` adds the elevated outer shadow.

**Step 2: Verify**

Run: `npx tsc --noEmit`
Expected: PASS — className change only.

**Step 3: Commit**

```bash
git add src/components/ui/Modal.tsx
git commit -m "feat(glass): apply glass material to Modal dialog"
```

---

## Task 7: Apply Glass to CommandPalette (Tier 1)

**Files:**
- Modify: `src/components/ui/CommandPalette.tsx`

**Step 1: Add glass to the palette modal panel**

In `src/components/ui/CommandPalette.tsx`, locate the palette modal `<div>` (line 196). It currently has:

```tsx
className="relative w-full max-w-lg mx-4 bg-surface rounded-2xl border border-border
  shadow-modal overflow-hidden animate-cmdPaletteScaleIn"
```

Change to:

```tsx
className="relative w-full max-w-lg mx-4 glass-panel glass-raised rounded-2xl
  overflow-hidden animate-cmdPaletteScaleIn"
```

**Step 2: Verify**

Run: `npx tsc --noEmit`
Expected: PASS.

**Step 3: Commit**

```bash
git add src/components/ui/CommandPalette.tsx
git commit -m "feat(glass): apply glass material to CommandPalette"
```

---

## Task 8: Apply Glass to ContextMenu (Tier 1)

**Files:**
- Modify: `src/components/ui/ContextMenu.tsx`

**Step 1: Add glass to the context menu panel**

In `src/components/ui/ContextMenu.tsx`, locate the menu `<div>` (line 71). It currently has:

```tsx
className="fixed z-50 min-w-[160px] py-1 bg-surface border border-border rounded-lg shadow-modal
  animate-cmdPaletteScaleIn"
```

Change to:

```tsx
className="fixed z-50 min-w-[160px] py-1 glass-panel glass-raised rounded-lg
  animate-cmdPaletteScaleIn"
```

**Step 2: Verify**

Run: `npx tsc --noEmit`
Expected: PASS.

**Step 3: Commit**

```bash
git add src/components/ui/ContextMenu.tsx
git commit -m "feat(glass): apply glass material to ContextMenu"
```

---

## Task 9: Apply Glass to Sidebar (Tier 2)

**Files:**
- Modify: `src/components/layout/Sidebar.tsx`

**Step 1: Add glass to the sidebar aside**

In `src/components/layout/Sidebar.tsx`, locate the `<aside>` (line 65). It currently has:

```tsx
className="w-16 lg:w-56 bg-surface border-r border-border flex flex-col py-3"
```

Change to:

```tsx
className="w-16 lg:w-56 glass-panel flex flex-col py-3"
```

The `glass-panel` class provides its own background, border, and shadow, replacing `bg-surface border-r border-border`. The sidebar's right border is handled by the `glass-panel`'s `border: 1px solid rgb(255 255 255 / 0.06)` on all sides. If a distinct right-edge is needed, the `border-r` can be kept — but for consistency with glass design, the subtle glass border is preferred.

However, the layout depends on `border-r` to visually separate sidebar from main. Let's keep it as a refinement — add `border-r border-border` back:

```tsx
className="w-16 lg:w-56 glass-panel border-r border-border flex flex-col py-3"
```

**Step 2: Verify**

Run: `npx tsc --noEmit`
Expected: PASS.

**Step 3: Commit**

```bash
git add src/components/layout/Sidebar.tsx
git commit -m "feat(glass): apply glass material to Sidebar"
```

---

## Task 10: Apply Glass to Right Panel (Tier 2)

**Files:**
- Modify: `src/components/layout/WorkspaceLayout.tsx`

**Step 1: Add glass to the aside (right panel)**

In `src/components/layout/WorkspaceLayout.tsx`, locate the `<aside>` (line 71). It currently has:

```tsx
className="border-l border-border bg-surface overflow-auto scrollbar-thin flex-shrink-0"
```

Change to:

```tsx
className="border-l border-border glass-panel overflow-auto scrollbar-thin flex-shrink-0"
```

The `glass-panel` replaces `bg-surface` with the semi-transparent blurred background. The `border-l border-border` is kept for the left-edge divider.

**Step 2: Verify**

Run: `npx tsc --noEmit`
Expected: PASS.

**Step 3: Commit**

```bash
git add src/components/layout/WorkspaceLayout.tsx
git commit -m "feat(glass): apply glass material to right panel aside"
```

---

## Task 11: Apply Glass to Header (Tier 2)

**Files:**
- Modify: `src/components/layout/Header.tsx`

**Step 1: Add glass to the header bar**

In `src/components/layout/Header.tsx`, locate the `<header>` (line 421). It currently has:

```tsx
className="h-14 bg-surface border-b border-border flex items-center justify-between px-4"
```

Change to:

```tsx
className="h-14 glass-panel border-b border-border flex items-center justify-between px-4"
```

The `glass-panel` replaces `bg-surface` with the blurred semi-transparent background. `border-b border-border` is kept for the bottom divider.

**Step 2: Verify**

Run: `npx tsc --noEmit`
Expected: PASS.

**Step 3: Commit**

```bash
git add src/components/layout/Header.tsx
git commit -m "feat(glass): apply glass material to Header"
```

---

## Task 12: Apply Glass to StatusBar (Tier 2)

**Files:**
- Modify: `src/components/layout/StatusBar.tsx`

**Step 1: Add glass to the status bar**

In `src/components/layout/StatusBar.tsx`, locate the root `<div>` (line 34). It currently has:

```tsx
className="h-7 bg-surface border-t border-border flex items-center px-4 gap-4 text-[10px] text-text-muted select-none shrink-0"
```

Change to:

```tsx
className="h-7 glass-panel border-t border-border flex items-center px-4 gap-4 text-[10px] text-text-muted select-none shrink-0"
```

The `glass-panel` replaces `bg-surface` with the blurred background.

**Step 2: Verify**

Run: `npx tsc --noEmit`
Expected: PASS.

**Step 3: Commit**

```bash
git add src/components/layout/StatusBar.tsx
git commit -m "feat(glass): apply glass material to StatusBar"
```

---

## Task 13: Final Verification — Full TypeScript Check + Visual Audit

**Files:**
- None modified — verification only.

**Step 1: Run full TypeScript check**

Run: `npx tsc --noEmit`
Expected: PASS with zero errors.

**Step 2: Run development server and verify across themes**

Run: `npm run dev`

Manual visual checks:
1. Open the app in Chrome
2. Switch through at least 5 themes (1 light, 1 dark, 1 vintage, 1 neon, and Studio Clean)
3. For each theme, verify:
   - Header bar shows frosted glass depth (blurred background through header)
   - Sidebar shows frosted glass (content behind sidebar is blurred through)
   - Right panel aside shows glass material
   - StatusBar shows glass material
   - Open a Modal — it should have glass + elevated shadow
   - Open CommandPalette (Cmd+K) — glass + elevated shadow
   - Right-click on a pad/waveform — ContextMenu shows glass
4. Verify glass nesting: the ContextMenu (glass) inside the main workspace should have reduced blur
5. Verify accessibility:
   - Set `prefers-reduced-motion: reduce` in browser DevTools — glass panels should fall back to 95% opaque with no blur
6. Verify the opt-out:
   - In browser console: set `data-glass-disabled="true"` on the root div — all glass should fall back to solid surfaces

**Step 3: Commit any fixups**

If any visual adjustments are needed, make them and commit:
```bash
git add -A
git commit -m "fix(glass): visual adjustments from cross-theme audit"
```

---

## Summary

| Task | File | Change | Effort |
|------|------|--------|--------|
| 1 | `globals.css` | Glass CSS custom properties | 2 min |
| 2 | `globals.css` | Glass utility classes + fallbacks | 3 min |
| 3 | `tailwind.config.ts` | Glass shadow tokens | 2 min |
| 4 | `layoutStore.ts` | `glassEnabled` toggle | 2 min |
| 5 | `page.tsx` | Wire `data-glass-disabled` attr | 2 min |
| 6 | `Modal.tsx` | Add `glass-panel glass-raised` | 2 min |
| 7 | `CommandPalette.tsx` | Add `glass-panel glass-raised` | 2 min |
| 8 | `ContextMenu.tsx` | Add `glass-panel glass-raised` | 2 min |
| 9 | `Sidebar.tsx` | Add `glass-panel` | 2 min |
| 10 | `WorkspaceLayout.tsx` | Add `glass-panel` to aside | 2 min |
| 11 | `Header.tsx` | Add `glass-panel` | 2 min |
| 12 | `StatusBar.tsx` | Add `glass-panel` | 2 min |
| 13 | — | TypeScript check + visual audit | 5 min |
| **Total** | **10 files** | **13 tasks** | **~30 min** |

## Architectural Constraints

1. **Maximum 3 simultaneous glass surfaces** — In the default layout, the user sees Header + Sidebar + Panel = 3 glass surfaces. Modals/ContextMenu replace the panel as the top-level glass when open, staying within the 3-surface budget.
2. **No theme changes** — Glass reads `--color-surface` which every theme already sets via `applyThemeToDOM()`.
3. **Texture overlay unaffected** — The `.texture-overlay` is at z-9999, above all glass surfaces, creating a natural dust-on-glass effect.
4. **Canvas elements untouched** — WaveformEditor canvas rendering is incompatible with backdrop-filter, so it stays solid per the design doc.
