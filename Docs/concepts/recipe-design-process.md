# The Recipe Design Process

*How XO_OX designs food recipes that honor cultures, teach synthesis, and feed people.*

**Rule Zero: These are real recipes for real people. If someone cooks this, it must taste good, nourish them, and teach them something about both a food tradition and a synthesis concept. Everything else serves that.**

---

## The Five Commitments

Every recipe in the Culinary Collection must satisfy all five:

### 1. IT MUST TASTE GOOD
Not conceptually interesting. Not "artistically bold." Actually delicious. A recipe that maps beautifully to a synthesis concept but doesn't taste good is a failed recipe. Taste comes first.

**Test**: Would you cook this for a friend who doesn't care about synthesizers? If not, redesign.

### 2. IT MUST BE ACCESSIBLE
- **Ingredients**: Available at a well-stocked grocery store. No high-end specialty shops required. If an ingredient is uncommon, provide a substitution AND explain what makes the original special (so they seek it out when they can).
- **Equipment**: Standard home kitchen. No sous vide, no blast chiller, no specialty tools. A pot, a pan, a knife, an oven, a blender — that's the kitchen.
- **Skill level**: Achievable by someone who cooks occasionally, not professionally. Include timing cues based on observation ("until the onions are translucent and fragrant") not just clock time.
- **Cost**: A meal for 2-4 people should cost under $25-30 in ingredients. This is a tool for the people.
- **Dietary notes**: Every recipe includes modification notes for vegetarian, vegan, dairy-free, and gluten-free where possible. Don't treat dietary needs as afterthoughts.

### 3. IT MUST HONOR THE CULTURE
- **Research the tradition**: Every dish comes from somewhere. Name that somewhere. Credit the people, the region, the history. Not "Asian-inspired" — "this technique comes from Cantonese wok hei, the breath of the wok."
- **Don't flatten**: Japanese, Chinese, Thai, Korean, and Vietnamese cooking are not interchangeable. Baltic, Russian, Georgian, and Polish cooking are not interchangeable. Respect specificity.
- **Permission, not appropriation**: Present recipes as an invitation to learn, not a claim of ownership. "This is my version, inspired by [tradition]. The original is worth seeking out from [tradition's practitioners]."
- **Acknowledge evolution**: Food traditions aren't museum pieces. They evolve, migrate, and fuse. Honor the roots AND the living tradition.
- **Name correctly**: Use the original language name alongside English. Shō, not "Japanese mouth organ." Bajan pepperpot, not "Caribbean stew."

### 4. IT MUST TEACH SOMETHING
Every recipe has three layers of education woven in:
- **Culinary education**: Why this technique works (the science of caramelization, the physics of emulsification, the chemistry of fermentation)
- **Cultural education**: Where this dish lives in its culture, what occasions it serves, who makes it, what it means
- **Synthesis education**: How the cooking technique maps to the engine's DSP (see the Translation Table below)

The education is woven into the recipe prose, not bolted on as sidebars. The reader learns by cooking.

### 5. IT MUST NOURISH
Real nutrition. Not empty calories dressed up with marketing. Every recipe should:
- Include a complete protein source (or complementary proteins for plant-based)
- Feature vegetables or fruits as co-stars, not garnish
- Use whole ingredients over processed
- Note the nutritional strengths of key ingredients without being preachy
- Encourage leftovers, batch cooking, and sharing — food as community

---

## The Translation Table: Cooking ↔ Synthesis

This is the core mapping. Every cooking technique has a DSP parallel. The recipe design process uses this table to find natural connections — never forced metaphors.

| Cooking Technique | Synthesis Parallel | Why It Maps | Example |
|-------------------|-------------------|------------|---------|
| **Caramelization** | Saturation / soft clipping | Heat transforms sugar's molecular structure; drive transforms a signal's harmonic structure. Both add warmth and complexity through controlled overload. | Caramelized onions → `buzz_intensity` |
| **Fermentation** | Slow modulation / LFO | Time transforms ingredients without heat; LFO transforms sound without direct intervention. Both require patience and produce results impossible to rush. | Kimchi → `pressure_instability` |
| **Emulsification** | Coupling / blending | Oil and water forced to coexist through an intermediary (lecithin, mustard); two engines forced to coexist through coupling types. Both create something neither component could alone. | Vinaigrette → adversarial coupling |
| **Maillard reaction** | Harmonic distortion | Amino acids + sugars at high heat = new flavor compounds; signal + nonlinearity = new harmonics. Both are about transformation through intensity. | Seared steak → `chiff_transient` |
| **Reduction** | Compression / filtering | Boiling removes water, concentrating flavor; compression removes dynamics, concentrating impact. Filter removes frequencies, concentrating timbre. | Balsamic reduction → filter cutoff |
| **Braising** | Low-pass filter + time | Low heat + liquid + time = tender transformation. LPF + slow envelope = gradual timbral opening. | Pot roast → filter envelope decay |
| **Smoking** | Reverb / space | Smoke infuses food with environment; reverb infuses sound with space. Both are about the container changing the content. | Smoked paprika → `crosstalk_leakage` |
| **Pickling / curing** | Quantization / bit reduction | Acid or salt preserves by removing water / reducing complexity; bit reduction preserves gesture by removing resolution. Both are deliberate simplification. | Quick pickles → `unison_detune` |
| **Tempering** | Envelope shaping | Controlled heating/cooling gives chocolate its snap; attack/decay/sustain/release gives notes their shape. Both are about the journey, not just the destination. | Tempered chocolate → ADSR |
| **Mise en place** | Parameter snapshot | Everything in its place before you cook; all params cached before the audio block. Both are about preparation enabling flow. | Prep bowls → ParamSnapshot |
| **Deglazing** | Sidechain / ducking | Liquid releases fond from pan; sidechain releases space from a mix. Both recover what was stuck. | Wine deglaze → spectral ducking |
| **Folding** | Additive synthesis | Gently incorporating air into batter; adding harmonic partials to build a complex tone. Both are about layering without destroying. | Soufflé → `cluster_density` |
| **Blanching** | Highpass filter | Brief boiling removes rawness/bitterness; HPF removes mud/rumble. Both clean without destroying character. | Blanched greens → HPF at 80Hz |
| **Stir-frying (wok hei)** | Fast transient + saturation | Maximum heat, minimum time = breath of the wok; fast attack + high drive = explosive transient. Both require controlled violence. | Wok hei → chiff + buzz |
| **Resting** | Release / tail | Meat rests to redistribute juices; notes release to let reverb bloom. Rushing either ruins the result. | Rested steak → release time |
| **Seasoning to taste** | Macro control | The cook's final adjustment; the producer's real-time parameter tweaking. Both are about trusting your senses over the recipe. | "Salt to taste" → M1-M4 |

---

## The Recipe Design Pipeline

### Step 1: Engine Identity → Regional Cuisine

Start with the engine's region and character. Map to a specific culinary tradition:

| Engine Region | Primary Cuisine | Secondary Influences |
|---------------|----------------|---------------------|
| East Asia (Oto) | Japanese (primary), Chinese, Lao/Thai, Southeast Asian | Korean, Taiwanese |
| Western Europe (Octave) | French (primary), Italian, Spanish | German, Dutch |
| Baltic/Eastern Europe (Oleg) | Latvian/Lithuanian (primary), Russian, Georgian | Polish, Ukrainian, Estonian |
| Americas (Otis) | Southern US (primary), Cajun/Creole, Caribbean | Mexican, Brazilian, African diaspora |

**Critical**: Don't collapse a region into one cuisine. Each chef's 4 organs span different sub-traditions. XOto's Shō (Japanese court music) demands different food than XOto's Khene (Lao/Thai folk). Let the instrument guide the dish.

### Step 2: Instrument Character → Dish Character

Map the instrument's sonic character to a dish's character:

| Sonic Character | Dish Character | Example |
|----------------|---------------|---------|
| Sustained, ethereal | Broth, consommé, infusion | Shō → dashi (clear, sustained, foundational) |
| Bright, articulate | Stir-fry, ceviche, fresh | Sheng → Cantonese stir-fry (bright, precise, dynamic) |
| Raw, buzzy, rhythmic | Fermented, pickled, funky | Khene → som tam (raw papaya salad — sharp, rhythmic pounding) |
| Warm, breath-driven | Baked, roasted, comfort | Melodica → Japanese milk bread (warm, soft, breath-risen) |
| Dark, symphonic | Braised, slow-cooked, deep | Cavaillé-Coll → cassoulet (dark, layered, hours of depth) |
| Precise, transparent | Composed salad, tartare | Baroque positiv → niçoise salad (every element distinct) |
| Buzzy, beating, triple-reed | Tangy, layered, vibrant | Musette → tarte tatin (caramelized layers, French countryside) |
| Transistor buzz, filtered | Street food, quick, punchy | Farfisa → croque monsieur (quick, satisfying, slightly trashy-good) |

### Step 3: Find the Science Bridge

Identify the cooking science that maps to the engine's DSP:

**Process**: For each recipe, find ONE primary technique that maps naturally to the engine's core synthesis method. Don't force multiple mappings — one clear bridge is better than three strained ones.

Write it as a single sentence: *"The way [cooking technique] transforms [ingredient] is the same way [DSP process] transforms [signal component]."*

Example: *"The way slow caramelization transforms raw onion sweetness into deep, complex umami is the same way the Maillard reaction in OceanDeep's exciter transforms a pure sine into a bioluminescent harmonic cloud."*

If you can't write this sentence naturally, the mapping is forced. Find a different dish.

### Step 4: Draft the Recipe

Use this template:

```markdown
# [Dish Name in Original Language] — [English Name]
*[Engine Name] | [Organ Selection] | [One-line soul of the dish]*

## The Connection
[2-3 sentences connecting this dish to this engine's sound. What does
cooking this teach you about synthesis? Natural, not forced.]

## Ingredients (serves 4)
[Grouped by component, not alphabetical. Listed in order of use.
Include weights AND volume measures. Note substitutions inline.]

## Method
[Numbered steps. Observation-based cues ("until fragrant and
translucent, about 5-7 minutes"). Note the synthesis parallel
at the most relevant step — ONE per recipe, woven into the prose,
not as a sidebar.]

## The Science
[1 paragraph: the food science of the key technique. Written for
curious cooks, not chemistry students. Connect to the synthesis
concept naturally.]

## Cultural Context
[1-2 paragraphs: where this dish lives in its culture. Who makes it,
when, for whom. What it means beyond nutrition. Source the tradition.]

## Variations
- Vegetarian/Vegan: [specific substitution]
- Gluten-free: [specific substitution]
- Simplify: [what to skip if short on time]
- Level up: [what to add if you want to go deeper]

## Pairs With
- **Engine preset**: [specific preset name that embodies this dish]
- **Coupling recipe**: [specific engine pairing that matches the dish's character]
- **Listen while cooking**: [a non-XO_OX music recommendation from the dish's culture]
```

### Step 5: The Accessibility Audit

Before finalizing, check every recipe against:

| Check | Pass? |
|-------|-------|
| All ingredients available at a standard grocery store (or substitution provided) | |
| Total ingredient cost < $30 for 4 servings | |
| No equipment beyond standard home kitchen | |
| Achievable in < 90 minutes active time (braising/fermenting time excluded) | |
| Dietary modification notes for at least 2 restrictions | |
| Cultural source named and credited | |
| Synthesis connection feels natural, not forced | |
| Would you actually cook this on a Tuesday night? | |

### Step 6: The Taste Test (Conceptual)

Read the recipe aloud. Ask:
1. Does the dish sound delicious independent of any synth connection?
2. Would someone from the source culture recognize this as respectful?
3. Does the synthesis parallel enhance understanding or feel gimmicky?
4. Could a beginning cook follow this successfully?

If any answer is no, revise.

---

## Recipe Density Per Quad

| Quad | Engines | Recipes Per Engine | Total | Notes |
|------|---------|-------------------|-------|-------|
| Chef (Organs) | 4 | 4 (one per organ selection) | 16 | Each organ = different sub-cuisine |
| Kitchen (Pianos) | 4 | 3 | 12 | Material-focused (cast iron, copper, stone, glass) |
| Choir (Voices) | 4 | 3 | 12 | Communal dishes (shared meals, feast food) |
| Garden (Strings) | 4 | 3 | 12 | Seasonal (spring/summer/fall/winter) |
| Cellar (Bass) | 4 | 3 | 12 | Preserved/aged/fermented |
| Broth (Pads) | 4 | 3 | 12 | Liquid-based (soups, stews, sauces, drinks) |
| **Total** | 24 | — | **76 recipes** | Enough for 2 substantial cookbooks |

---

## The Two Cookbooks Per Quad

### Synth Cookbook (Technical)
- How the engine works (parameter guide)
- Compiled from Guru Bin retreat notes
- Coupling recipes with other engines
- Preset design philosophy
- 8-12 pages per engine, 40-50 pages per quad

### Regional Cookbook (Cultural + Culinary)
- Real food recipes mapped to the engine's sound
- Cultural context for each dish
- Science bridges between cooking and synthesis
- Music recommendations from the tradition
- Photography direction notes (warm, lived-in, not sterile food photography)
- 4-5 pages per recipe, 50-70 pages per quad

### Combined E-Book Per Quad Release
- Interleaved: engine chapter → its recipes → next engine
- URL scheme deep links: tap a preset name → XOmnibus opens it
- Total per quad: ~100-120 pages
- Format: PDF (free), EPUB (Patreon), print-on-demand (premium)

---

## Cultural Advisory Framework

For each regional cuisine, the recipe design process must:

1. **Name at least 3 source cookbooks or authors** from the tradition being referenced
   - Oto (Japanese): Sonoko Sakai, Nancy Singleton Hachisu, Tadashi Ono
   - Octave (French): Jacques Pépin, Madeleine Kamman, Julia Child (as bridge)
   - Oleg (Baltic/EE): Olia Hercules, Zuza Zak, Alissa Timoshkina
   - Otis (Americas): Toni Tipton-Martin, Leah Chase, Jessica B. Harris

2. **Avoid the Big Five appropriation traps**:
   - Don't "elevate" — the original is already elevated
   - Don't "simplify" by removing what makes it culturally specific
   - Don't present a fusion as if it's the original
   - Don't use colonial-era names for ingredients (use current names from the source culture)
   - Don't claim discovery of techniques that have existed for centuries

3. **Include a "Go Deeper" section** per quad:
   - Recommended restaurants (from the source culture, not fusion interpretations)
   - Recommended cookbooks (by authors from the tradition)
   - Recommended music (traditional and contemporary artists from the region)
   - Markets/shops for authentic ingredients (online options for accessibility)

---

## Process Summary

```
Engine Identity
    ↓
Regional Cuisine Mapping (Step 1)
    ↓
Instrument → Dish Character Match (Step 2)
    ↓
Science Bridge (Step 3)
    ↓
Draft Recipe (Step 4)
    ↓
Accessibility Audit (Step 5)
    ↓
Taste Test (Step 6)
    ↓
SHIP (in regional cookbook, paired with engine chapter in synth cookbook)
```

**Repeat for each organ/instrument configuration within each engine.**

---

*This process is the kitchen's CLAUDE.md — the rules that ensure every recipe serves the people it's for, honors the people it comes from, and teaches something true about both food and sound.*
