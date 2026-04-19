# AI Subsystem — Architecture and Component Reference

> **Build gate:** The entire AI subsystem is gated behind the `XOCEANUS_BUILD_AI` CMake option
> (default `OFF`). None of these files are compiled into a standard release build. To include them:
> ```
> cmake -B build -G Ninja -DXOCEANUS_BUILD_AI=ON
> ```

---

## Purpose

The AI subsystem gives XOceanus users a natural language sound design assistant. Rather than
requiring knowledge of specific parameter IDs, users describe the sound they want in plain
English ("massive dark pad with shimmer", "more aggressive", "sounds like the color blue") and
the assistant translates that into a full `.xorecipe` configuration or iterative parameter
adjustments.

The subsystem is deliberately provider-agnostic: it supports Claude (Anthropic), ChatGPT
(OpenAI), and Gemini (Google) through a unified API surface. The caller never sees
provider-specific details.

---

## Architecture Overview

```
User text input
      │
      ▼
NaturalLanguageInterpreter           ← stateless, offline, no API key needed
  • Tier 1: technical param refs     → hasTechnicalContent = true
  • Tier 2: musical adjectives       → DNA delta adjustments
  • Tier 3: abstract/synesthetic     → DNA + mood + texture hints
  • ExpertiseLevel detection
      │
      ▼ InterpretedIntent (enrichedPrompt, dnaDelta, suggestedEngines, …)
      │
      ▼
SoundAssistant                       ← orchestrator; manages keys, rate limits, threads
  • Retrieves key from SecureKeyStore (plaintext only during HTTP call)
  • Injects AIParameterSchema into prompt (parameter semantics + constraints)
  • Fires RequestThread (JUCE Thread) → provider API over HTTPS
  • Validates/clamps AI response via AIParameterSchema
  • Returns RecipeResult / RefineResult / SoundMatchResult to message thread
      │
      ▼
RecipeEngine                         ← applies Recipe to live APVTS + coupling matrix
```

Community and telemetry run as optional side-channels, independent of the request path:

```
CommunityInsights  ← anonymous opt-in telemetry; batched, previewed before send
SharedRecipeVault  ← opt-in recipe upload/browse via Supabase PostgREST
```

---

## Component Reference

### SoundAssistant (`Source/AI/SoundAssistant.h`)

The central orchestrator. All other AI components are owned or called by this class.

**Responsibilities:**
- Provider selection and key lifecycle (key retrieved → request sent → key wiped)
- Rate limiting (10 requests per minute, ring buffer of timestamps)
- Multi-turn conversation history (`kMaxHistoryEntries = 10`)
- Seven AI capabilities exposed as async methods (callback on message thread)
- Validates every AI response through `AIParameterSchema` before returning it

**Public API:**

| Method | Input | Output |
|--------|-------|--------|
| `textToRecipe(description, context, cb)` | Natural language description + current synth state | `RecipeResult` containing a full `.xorecipe` |
| `refineRecipe(request, currentRecipe, context, cb)` | Refinement phrase ("more aggressive") + current recipe | `RefineResult` with param diffs and DNA before/after |
| `matchSound(description, context, cb)` | Reference sound description | `SoundMatchResult` with matching recipe + listening notes |
| `adviseCoupling(context, cb)` | Current engine setup | `CouplingAdvice` with ranked coupling suggestions |
| `suggestMacros(context, cb)` | Current parameter state | `MacroSuggestion` with evocative labels + target lists |
| `namePreset(context, cb)` | Parameter values + DNA | `NamingSuggestion` with name, description, mood, tags |
| `explainSound(context, question, cb)` | Current state + user question | `SoundExplanation` in plain English + improvement hints |

**Key types:**

- `SynthContext` — snapshot of active engines, all parameter values, coupling routes, master FX state, and current 6D Sonic DNA. This is everything sent to the AI (no audio, no user data).
- `RecipeResult` — wraps a `RecipeEngine::Recipe` plus the AI's reasoning text, validation warnings, and counts of corrected/dropped parameters.
- `RefineResult` — extends `RecipeResult` with a `paramChanges` diff, detected `ExpertiseLevel`, interpretation confidence, and DNA before/after.
- `SoundMatchResult` — extends `RecipeResult` with the `InterpretedIntent` and `listeningNotes`.

**Security:**
- Keys are retrieved from `SecureKeyStore` only inside `sendRequest()` and live only in `RequestThread`.
- `RequestThread` destructor calls `SecureKeyStore::secureWipePublic()` to zero key bytes after the response is received.
- The `destroyed_` atomic flag prevents callbacks from firing on the message thread after `SoundAssistant` has been destroyed.
- HTTPS enforcement is checked both at the hardcoded URL level and defensively inside `RequestThread::run()`.

---

### AIParameterSchema (`Source/AI/AIParameterSchema.h`)

A structured, immutable knowledge base of every DSP parameter in the fleet. Built once at startup
from each engine's `createParameterLayout()`, then augmented with hand-tuned musical metadata.

**Three purposes:**
1. **Prompt enrichment** — serialized to JSON and injected into the AI prompt so the model knows
   exactly what parameters exist, their ranges, and their musical meaning.
2. **Output validation** — clamps, quantizes, and safety-checks every AI-suggested value before
   it can reach a `juce::AudioProcessorValueTreeState`.
3. **Safety constraints** — cross-parameter rules (e.g. "if delay feedback > 0.9 and delay time
   < 10ms, clamp delay time to prevent runaway") applied as a post-pass after per-parameter clamping.

**Key types:**

- `AIParamType` — enum classifying a parameter as `Continuous`, `Frequency`, `Time`, `Semitones`,
  `Percentage`, `Choice`, `Toggle`, `Unipolar`, `Bipolar`, or `Integer`.
- `AIParamRole` — enum encoding the musical function: oscillator pitch/shape, filter cutoff/resonance,
  ADSR stages, LFO rate/depth, delay/reverb controls, drive, grain parameters, coupling amount, etc.
- `AIParamDef` — a single parameter definition: `paramId`, display name, engine ID, type, role,
  min/max/default/step, skew factor, sweet-spot range, musical description, and safety constraints
  (`canCauseRunaway`, `safeMax`, `safetyNote`). The `validate()` method runs the full
  clamp → safety-clamp → quantize pipeline.
- `AIEngineProfile` — per-engine metadata: character string, strengths, weaknesses, best
  send/receive coupling types, recommended pairings, and all `AIParamDef`s for that engine.
- `AICouplingConstraint` — max safe intensity and incompatibility notes for each coupling type.
- `AISafetyRule` — condition + fix pair for cross-parameter enforcement.
- `ValidationResult` — returned by `validateParameters()` and `validateCoupling()`; contains
  corrected values, warnings, and errors.

**Registration:**
```cpp
schema.registerEngine (profile);                 // called per engine at startup
schema.registerCouplingConstraint (constraint);
schema.addSafetyRule (rule);
```

**Validation pipeline:**
```cpp
auto vr = schema.validateParameters (aiSuggestedParams);
// vr.correctedParams contains safe values; vr.warnings describes each correction
```

---

### NaturalLanguageInterpreter (`Source/AI/NaturalLanguageInterpreter.h`)

A stateless, offline, deterministic pre-processor. Runs before the AI API call to convert
unstructured user text into structured guidance the AI prompt can incorporate.

**Three input tiers:**

| Tier | Example input | What it produces |
|------|---------------|-----------------|
| 1 — Technical | "reduce snap_filterCutoff to 0.3, increase FM depth" | `hasTechnicalContent = true`; prompt passed through unchanged |
| 2 — Musical | "warmer, more movement, add shimmer" | `DNADelta` adjustments on specific axes (warmth +0.5, movement +0.4, brightness +0.4, …) |
| 3 — Abstract/synesthetic | "the color blue", "underwater cathedral", "sounds like autumn" | Mapped to DNA deltas + `suggestedMood` + `textureHints` via Bouba/Kiki-inspired tables |

**Data flow into 6D Sonic DNA mapping:**

1. `interpret(userInput)` runs a 9-step pipeline on the lowercased input:
   - `detectExpertise()` — scores expert/intermediate/novice by matching against ~55 expert terms (cutoff, LFO, FM, coupling matrix, etc.) and ~50 intermediate terms (reverb, delay, compression, etc.)
   - `containsTechnicalContent()` — checks for engine parameter prefixes (`snap_`, `dub_`, `perc_`, etc.) and coupling type keywords
   - `applyModifierMappings()` — maps ~80 musical adjectives to multi-dimensional DNA deltas (each word maps to fractional adjustments across all 6 DNA axes simultaneously, e.g. "lush" → warmth +0.4, movement +0.2, density +0.2)
   - `applySynestheticMappings()` — maps sensory/abstract words (colors, textures, environments, weather) to DNA + mood
   - `detectMood()` — returns one of the 15 XOceanus moods if a clear match is found
   - `suggestEngines()` — recommends specific engine IDs based on texture keywords
   - `suggestFX()` — recommends FX stages based on effect-class keywords
   - `buildEnrichedPrompt()` — constructs a structured prompt section embedding all the above
   - `calculateConfidence()` — 0–1 score based on how many mappings fired

2. For refinement requests, `parseRefinement()` parses polarity ("less", "reduce") and magnitude
   ("much", "slightly", "extremely") modifiers, then maps adjectives to `RefinementDirection` structs.
   Multiple words mapping to the same DNA axis are averaged.

3. `refinementsToDNADelta()` and `applyDNADelta()` convert directions into a target DNA state
   clamped to [0, 1] per axis.

**Key types:**

- `ExpertiseLevel` — `Novice`, `Intermediate`, `Expert` (adapts AI response verbosity)
- `InterpretedIntent` — the full result: expertise, `DNADelta`, suggested mood, texture hints,
  suggested engine IDs, suggested FX, technical content flag, normalized input,
  enriched prompt, and confidence score
- `RefinementDirection` — a (dimension, magnitude, explanation) triple; magnitude is −1 to +1

**Research basis cited in the header:** Bouba/Kiki crossmodal effect, MIR mood/valence/arousal
models, Sony LLM2Fx (natural language → effect chain parameters).

---

### SecureKeyStore (`Source/AI/SecureKeyStore.h`)

Encrypted storage for the user's AI provider API keys. The sole gateway to key material;
no other module handles keys.

**Supported providers:** Anthropic (`sk-ant-api03-…`), OpenAI (`sk-…`), Google (`AIza…`).

**Security architecture:**
- macOS/iOS (production): keys stored exclusively in the OS Keychain via `SecItem` API.
  AES encryption is OS-managed, hardware-backed on Apple Silicon / Secure Enclave devices.
- Other platforms (deprecated fallback): BlowFish via `juce::BlowFish` using a device-derived
  SHA-256 key (computer name + unique device ID + app salt).
- Keys exist in plaintext only in memory, only during an API call.
- `secureWipe()` zeroes the string's internal UTF-8 buffer before releasing.
- No key material is ever logged, transmitted outside TLS, or written to disk in plaintext.

**Public API:**

| Method | Description |
|--------|-------------|
| `storeKey(provider, plaintextKey)` | Validate format, commit to Keychain, wipe plaintext |
| `retrieveKey(provider)` | Decrypt and return key (session cache avoids repeated Keychain reads) |
| `releaseKey(provider)` | Wipe the session-cached decrypted copy |
| `releaseAllKeys()` | Wipe all session-cached copies (call on app quit or background) |
| `hasKey(provider)` | Check existence without decrypting |
| `deleteKey(provider)` | Permanently remove from Keychain/disk |
| `validateKeyFormat(provider, key)` | Prefix/length check only — no network call |

---

### SharedRecipeVault (`Source/AI/SharedRecipeVault.h`)

Opt-in community recipe sharing backed by Supabase PostgREST + Edge Functions.

**Privacy model:** 100% opt-in (sharing is a deliberate user action). No personal data attached
to shared recipes. Users choose a display name (anonymous is the default). TLS only. Content
is pure JSON config — no executable code.

**Backend endpoints used:**
- `POST /rest/v1/shared_recipes` — share a recipe
- `GET /functions/v1/vault-search` — browse with full-text search and pagination
- `GET /rest/v1/shared_recipes?recipe_id=eq.{id}` — download a specific recipe
- `POST /functions/v1/vault-thumbsup` — rate a recipe (dedup by voter hash)
- `DELETE /rest/v1/shared_recipes?recipe_id=eq.{id}` — delete own recipe (RLS via author token)

**Public API (all async wrappers dispatch `juce::Thread` internally):**

| Method | Description |
|--------|-------------|
| `configure(VaultConfig)` | Set Supabase URL, anon key, display name, author token |
| `browseAsync(BrowseFilter)` | Filter/search community recipes; fires `onBrowseComplete` |
| `shareAsync(json, description, tags)` | Upload a recipe; fires `onShareComplete` |
| `downloadAsync(recipeId)` | Download full recipe JSON; fires `onDownloadComplete` |
| `thumbsUp(recipeId, voterHash)` | Increment thumbs-up count |
| `deleteOwnRecipe(recipeId)` | Remove own recipe (author token required) |

**Key types:** `RecipeMetadata`, `BrowseFilter` (mood/engine/text/staffPicks/sort/pagination),
`BrowseResult`, `ShareResult`, `DownloadResult`.

**Integration with CommunityInsights and SoundAssistant:**
- Popular shared recipes inform factory preset priorities.
- Engine and coupling patterns from shared recipes feed aggregate stats.
- The AI assistant can reference community favorites when generating suggestions.

---

### CommunityInsights (`Source/AI/CommunityInsights.h`)

Anonymous, opt-in telemetry for product development intelligence.

**Privacy model:** Disabled by default. Explicit `ConsentLevel` required. No PII, no audio,
no parameter values — only aggregate counts and histograms. User can preview the exact JSON
batch before it is transmitted. User can delete all collected data at any time. TLS only.

**What is collected (at `ConsentLevel::Full`):**
- Engine slot usage counts (which engines are loaded and in which slot)
- Coupling type usage counts (12 types)
- 6D Sonic DNA histograms (5-bin distribution per axis) from saved presets
- Recipe mood/load/create/modify counts
- AI query theme counts (8 categories: bass, pad, lead, FX, percussion, ambient, aggressive, experimental) — **never** the actual query text
- FX stage active counts (18 stages) and Vibe knob direction (sweet vs. grit)

**At `ConsentLevel::BasicUsage`:** engine and coupling counts only.

**Public API:**

| Method | Description |
|--------|-------------|
| `setPrivacyConfig(config)` | Set consent level and batch interval |
| `recordEngineLoaded(index, slot)` | Called when user loads an engine |
| `recordCouplingRoute(type)` | Called when a coupling route is created |
| `recordPresetSaved(br, wa, mv, de, sp, ag)` | Called when user saves a preset |
| `recordRecipeLoaded(moodIndex, isFactory)` | Called when a recipe is loaded |
| `recordAIQuery(category)` | Called when AI assistant is used (category string only) |
| `recordFXStageActive(stage)` | Called when an FX stage is activated |
| `recordVibeKnobUsage(value)` | Called when Vibe knob moves |
| `getPreviewJSON()` | Returns the current batch as human-readable JSON for user review |
| `prepareBatch()` | Seals the batch and resets for the next collection period |
| `clearAllData()` | Discards all collected data without transmitting |
| `transmitBatch(batch)` | Sends to Supabase `insight_batches` table (background thread only) |
| `savePendingToDisk(dir)` / `loadPendingFromDisk(dir)` | Persist batch across app restarts |

**Key type:** `InsightsBatch` — contains a random UUID batch ID (not device-linked), app version,
platform string, and all the data sub-structs above.

---

### RecipeEngine (`Source/AI/RecipeEngine.h`)

The file system and application layer for `.xorecipe` files. Moved from `Source/Core/` to
`Source/AI/` on 2026-04-01 (issue #575).

**What a Recipe is:** A "Scene Architect" that specifies engine slot assignments, per-engine
preset references or parameter overrides, the full coupling matrix, all 18 master FX parameter
values, 4 macro assignments with target lists and range amounts, an optional variation axis
(a single sweep dimension for exploration), the 6D Sonic DNA fingerprint, and a `suggestedBPM`.

**Format design:** Forward-compatible JSON. Unknown engine IDs, FX stages, and coupling types are
silently ignored so recipes created with future engine versions remain applicable.

**Public API:**

| Method | Description |
|--------|-------------|
| `scanRecipeDirectory(dir)` | Recursively scan for `*.xorecipe` files and build the in-memory catalog |
| `getCatalog()` | Return all `CatalogEntry` items (sorted by mood, then name) |
| `getByMood(mood)` | Filter catalog by mood string |
| `getByTag(tag)` | Filter catalog by tag |
| `getByEngine(engineId)` | Filter catalog by required engine |
| `loadRecipe(file)` | Parse a `.xorecipe` file into a full `Recipe` struct |
| `applyRecipe(recipe, apvts, callbacks)` | Apply a recipe to live synth state via callback injection |
| `saveRecipe(recipe, file)` (static) | Serialize a `Recipe` to a `.xorecipe` file |

**Key types:**

- `Recipe` — the complete recipe struct: identity fields, `vector<EngineSlot>`, `vector<CouplingRoute>`,
  `map<String, float>` for master FX, `array<MacroAssignment, 4>`, optional `VariationAxis`,
  `SonicDNA`, `suggestedBPM`, and `extensions` for future-proofing.
- `CatalogEntry` — lightweight struct for browsing: name, mood, author, description, tags,
  engine names, file path, and DNA.
- `ApplyCallbacks` — `loadEngine`, `loadEnginePreset`, `setCoupling`, and `setMacro` function
  objects injected by the caller for integration with `EngineRegistry` and `MegaCouplingMatrix`.

---

## Data Flow: NaturalLanguageInterpreter to 6D Sonic DNA

The following describes the complete path from a user's text to a live parameter change:

```
User: "dark evolving pad with shimmer, massive and spacious"
  │
  ▼ NaturalLanguageInterpreter::interpret()
  │
  ├─ detectExpertise()     → Novice (no technical terms)
  ├─ applyModifierMappings():
  │    "dark"     → brightness −0.4, warmth +0.2, density +0.1
  │    "evolving" → movement +0.5, space +0.1
  │    "shimmer"  → brightness +0.4, movement +0.3, space +0.2
  │    "massive"  → density +0.5, space +0.1, aggression +0.2
  │    "spacious" → space +0.5
  │    (averaged per axis)
  │    → DNADelta { brightness: 0.0, warmth: 0.2, movement: 0.4,
  │                 density: 0.6, space: 0.9, aggression: 0.2 }
  ├─ detectMood()          → "Atmosphere"
  ├─ suggestEngines()      → ["Opal", "OpenSky"] (pad + shimmer keywords)
  ├─ suggestFX()           → ["reverb", "shimmerReverb"]
  └─ buildEnrichedPrompt() → injects DNA targets, mood, engine hints into prompt
  │
  ▼ SoundAssistant::textToRecipe()
  │
  ├─ Injects AIParameterSchema JSON (all engine parameters, ranges, sweet spots)
  ├─ Fires RequestThread → Anthropic / OpenAI / Google API (HTTPS)
  ├─ AI returns Recipe JSON with specific parameter values
  │
  ▼ AIParameterSchema::validateParameters()
  │
  ├─ Per-parameter: clamp to safe range, check sweet spot, quantize to step size
  ├─ Cross-parameter safety rules (feedback + delay time, resonance extremes, etc.)
  └─ Returns corrected params + validation warnings
  │
  ▼ RecipeEngine::applyRecipe()
  │
  ├─ LoadEngine callbacks → EngineRegistry
  ├─ Parameter writes → juce::AudioProcessorValueTreeState
  ├─ SetCoupling callbacks → MegaCouplingMatrix
  └─ SetMacro callbacks → macro assignment system
```

---

## Current Status

- **Build gate:** `XOCEANUS_BUILD_AI=OFF` (default). None of these files compile in standard builds.
- **RecipeEngine.h** was moved from `Source/Core/` to `Source/AI/` on 2026-04-01 (closes #575).
  The CMakeLists.txt comment on line 463 documents this move.
- All seven AI capabilities (`textToRecipe`, `refineRecipe`, `matchSound`, `adviseCoupling`,
  `suggestMacros`, `namePreset`, `explainSound`) are fully implemented in `SoundAssistant.h`.
- `AIParameterSchema::buildDefaultSchema()` (declared in `SoundAssistant.h`) still needs
  completion — it must be populated with `AIEngineProfile` entries for every engine.
- `SharedRecipeVault` and `CommunityInsights` both require Supabase project credentials to
  be set at runtime; they are inert without configuration.

---

## Dependencies and Integration Points

| AI Component | Depends on | Integrated via |
|--------------|-----------|----------------|
| `SoundAssistant` | `SecureKeyStore`, `AIParameterSchema`, `NaturalLanguageInterpreter`, `RecipeEngine` | Owned members; `SynthContext` passed by caller |
| `NaturalLanguageInterpreter` | `RecipeEngine::Recipe::SonicDNA` | Static methods only; no JUCE modules beyond `juce_core` |
| `AIParameterSchema` | `juce_core` | Populated at startup by iterating `createParameterLayout()` per engine |
| `SecureKeyStore` | `juce_core`; `Security.framework` (macOS/iOS); `juce_cryptography` (other platforms) | Singleton owned by `SoundAssistant` |
| `SharedRecipeVault` | `juce_core` (URL, JSON, Thread) | Configured with Supabase credentials at app startup; callbacks wired to UI |
| `CommunityInsights` | `juce_core` | `recordXxx()` call sites scattered through UI/engine event handlers |
| `RecipeEngine` | `juce_audio_processors` (APVTS), `juce_core` | `ApplyCallbacks` injected by `XOceanusProcessor` or equivalent host |

**External services:**
- AI inference: Anthropic Messages API, OpenAI Chat Completions, Google Generative Language API
- Community vault: Supabase PostgREST + Edge Functions (`vault-search`, `vault-thumbsup`)
- Telemetry: Supabase PostgREST (`insight_batches` table)
