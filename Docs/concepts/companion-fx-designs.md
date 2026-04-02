# Kitchen Collection — Companion FX Designs

*March 2026 | XO_OX Designs*
*Six boutique effects, one per cookbook. Standalone-capable. Quad-optimized.*

---

## Overview

Each Culinary Collection cookbook ships with a signature FX unit that embodies the quad's
coupling philosophy in a single stereo processor. These are lightweight "character effects" —
not utility tools. They sound interesting on any engine but reveal their deepest behavior when
paired with the engines they were designed around.

**CPU budget:** <1% per unit at 48kHz.
**Integration:** All six live in the MasterFXChain as a new `KitchenFXChain` group between
the Boutique chain (stages 13–16) and the Singularity chain (stages 17–19). Each occupies
one numbered slot within that group (slots 17–22, pushing Singularity to 23–25).

**Parameter prefix:** `kfx_` (shared namespace prefix) + per-effect suffix.
Each FX declares parameters in the form `kfx_{short}_{paramName}`.

---

## 1. fXAdversary — "Spectral Duel"

**Cookbook:** Chef (Organs — adversarial coupling)
**Stage slot:** 17
**Parameter prefix:** `kfx_adv_`

### Concept

Two spectral forces fight over the same signal. An internal reference buffer captures a frozen
snapshot of the input and holds it as the "opponent." The live signal competes against this ghost
of itself — any frequency band where the live signal is weaker than the reference gets ducked;
any band where it dominates gets a push. At low settings this creates subtle spectral clearing
(the signal fights for space against its own past). At high settings the duel becomes audible
as a churning, roiling spectral battle — aggressive clearing, dramatic swells, the sonic equivalent
of two chefs refusing to yield the burner.

The reference snapshot is re-captured every `fxadv_refreshMs` milliseconds — so the opponent
is always stale by design. This staleness gap is the competitive arena.

### DSP Architecture

**Signal flow:**
```
input ──┬────────────────────────────────────────────┐
        │                                            │
        └→ 4-band analysis (RMS per band, 10ms avg)  │
        │                                            │
        ↓                                            │
reference buffer (4-band RMS, refreshes at interval) │
        │                                            │
        ↓                                            │
ratio = liveRMS[b] / (refRMS[b] + ε)                │
        │                                            │
gain[b] = ratio^rivalry                              │
(>1.0 → slight boost, <1.0 → duck)                  │
        │                                            │
clamp gain[b] to [duckFloor, peakCeil]              │
        │                                            │
4-band SVF filterbank → per-band gain → sum → mix   │
        │                                            │
        └── dry ──────────────────────────────────────┘
```

**4-band split (Linkwitz-Riley style via CytomicSVF pairs):**
- Band 1: 20–250 Hz (bass territory — organ pedals)
- Band 2: 250–1200 Hz (body — organ drawbars)
- Band 3: 1200–5000 Hz (presence — chiff transients)
- Band 4: 5000–20000 Hz (air — high harmonics)

The gain curve is a power law: `gain = ratio^rivalry`. At `rivalry = 0`, all gains collapse to 1.0
(no effect). At `rivalry = 1.0`, the ratio maps 1:1 (linear competition). At `rivalry = 2.0+`,
weak bands get crushed and strong bands get amplified — the duel becomes operatic.

**Reference snapshot logic:**
```
refreshTimer += dt
if refreshTimer >= refreshMs:
    refRMS[b] = liveRMS[b]  // freeze current state as opponent
    refreshTimer = 0
```

A one-pole smoother on `refRMS` prevents clicks when the snapshot refreshes.

### Parameters

| Parameter ID | Range | Default | Description |
|---|---|---|---|
| `kfx_adv_rivalry` | 0.0–3.0 | 0.8 | Power-law exponent. 0=bypassed, 1=linear, 3=extreme dueling |
| `kfx_adv_refreshMs` | 50–5000 ms | 800 ms | How often the reference snapshot updates. Longer = opponent is older/staler |
| `kfx_adv_duckFloor` | 0.05–1.0 | 0.2 | Minimum gain applied to losing bands (prevents silence) |
| `kfx_adv_peakCeil` | 1.0–3.0 | 1.6 | Maximum gain applied to winning bands (prevents runaway) |
| `kfx_adv_bandwidth` | 0.3–1.0 | 0.7 | Q of the 4 analysis bands — wider Q = more frequency overlap, softer duel |
| `kfx_adv_mix` | 0.0–1.0 | 0.0 | Dry/wet |

### Cookbook Description

*The kitchen is a competitive space. fXAdversary captures a snapshot of your sound and turns
it into an opponent. Your current signal fights against its own recent past for spectral space —
bands where you've grown stronger emerge, bands where you've retreated get suppressed. At low
settings it's the subtle clearing you hear when a great chef's technique sharpens mid-service.
At full rivalry, it's two chefs refusing to leave the pass. The sound of audio that refuses to
agree with itself.*

### Pseudocode

```cpp
void processBlock(float* L, float* R, int n) {
    for each sample i:
        // Update RMS per band (10ms window one-pole)
        for b in 0..3:
            float mono = (L[i] + R[i]) * 0.5f
            float bandOut = runBandpass(b, mono)
            liveRMS[b] += envCoeff * (abs(bandOut) - liveRMS[b])

        // Refresh reference snapshot
        refreshTimer++
        if refreshTimer >= refreshSamples:
            for b in 0..3:
                refRMS[b] = smoothed(refRMS[b], liveRMS[b])
            refreshTimer = 0

        // Compute per-band gain
        for b in 0..3:
            ratio = liveRMS[b] / max(refRMS[b], 1e-6)
            gain[b] = clamp(pow(ratio, rivalry), duckFloor, peakCeil)

        // Apply gains through reconstruction filterbank
        outL = 0; outR = 0
        for b in 0..3:
            outL += gainedBand(L[i], b, gain[b])
            outR += gainedBand(R[i], b, gain[b])

        L[i] = dry*L[i] + mix*outL
        R[i] = dry*R[i] + mix*outR
}
```

---

## 2. fXResonance — "Material Reverb"

**Cookbook:** Kitchen (Pianos — resonant coupling)
**Stage slot:** 18
**Parameter prefix:** `kfx_res_`

### Concept

A reverb whose character is defined entirely by a material metaphor — cast iron, copper, stone,
or glass. Each material is a different combination of: decay time, high-frequency absorption
rate, early reflection density, and a subtle nonlinearity that represents the physical behavior
of the material under vibration. The material morphs continuously between the four poles via a
single parameter, so intermediate points are real blends (copper-iron, stone-glass) rather than
hard switches.

This maps the Kitchen quad's resonant coupling philosophy: the sound behaves as though it is
physically vibrating inside a body made of the selected material. Pair with XOven (cast iron)
for canonical synergy — the dark long reverb of a cast iron concert grand body.

### DSP Architecture

**Signal flow:**
```
input → material pre-filter (tone LP: varies by material)
      → 4-tap Schroeder delay network (early reflections)
      → all-pass diffuser × 2 (per-material density)
      → decay filter (one-pole LP: absorption coefficient varies by material)
      → feedback → mix with dry
```

**4 material poles** (interpolated via single 0–3 `material` parameter):
```
0.0 = Cast Iron:   decayMs=4200, absorption=0.92, density=0.4, satAmt=0.12
1.0 = Copper:      decayMs=1800, absorption=0.74, density=0.7, satAmt=0.04
2.0 = Stone:       decayMs=2600, absorption=0.55, density=0.9, satAmt=0.08
3.0 = Glass:       decayMs=900,  absorption=0.30, density=0.3, satAmt=0.22 (shatters at high input)
```

Interpolation: fractional `material` value linearly interpolates between the two nearest poles.
E.g. `material=1.5` is 50% copper / 50% stone.

**Glass special behavior:** Above an input threshold, a subtle harmonic distortion (2nd+3rd order)
is introduced — simulating glass resonance starting to crack under pressure. This is controlled by
`kfx_res_fragility`.

**Early reflections (4 taps):** Delay times derived from material density. Dense materials
(stone) use shorter, closer-spaced taps (20–80ms). Open materials (glass) use sparser, wider
taps (15–120ms). These are pre-computed from the material parameter, not real-time per-sample.

### Parameters

| Parameter ID | Range | Default | Description |
|---|---|---|---|
| `kfx_res_material` | 0.0–3.0 | 0.0 | Material: 0=Cast Iron, 1=Copper, 2=Stone, 3=Glass |
| `kfx_res_body` | 0.0–1.0 | 0.6 | Body resonance amount — how much the material "sings" |
| `kfx_res_room` | 0.0–1.0 | 0.5 | Room size scalar applied to base decay times per material |
| `kfx_res_fragility` | 0.0–1.0 | 0.3 | Glass-mode harmonic distortion threshold sensitivity |
| `kfx_res_sympathetic` | 0.0–1.0 | 0.4 | Sympathetic resonance add — mid-band resonant peak imitating string ring |
| `kfx_res_mix` | 0.0–1.0 | 0.0 | Dry/wet |

### Cookbook Description

*The kitchen is the instrument. fXResonance places your sound inside a body made of material —
cast iron, copper, stone, or glass. Cast iron: dark, massive, patient. It holds heat and holds
sound. The reverb tail is heavy, long, fundamentally warm. Copper: quick, responsive, intimate.
The reverb of a room where someone is practicing. Stone: hard, inharmonic, unyielding. The cold
marble counter that makes everything ring a half-step off. Glass: crystalline, brief, dangerous
at high volume — push it past its limit and it starts to break. Blend between materials freely:
copper-iron has the warmth of the practice room inside the body of a concert hall. The kitchen
surface is the reverb.*

### Pseudocode

```cpp
MaterialPole poles[4] = {
    {decayMs:4200, absorb:0.92, density:0.4, sat:0.12},  // cast iron
    {decayMs:1800, absorb:0.74, density:0.7, sat:0.04},  // copper
    {decayMs:2600, absorb:0.55, density:0.9, sat:0.08},  // stone
    {decayMs:900,  absorb:0.30, density:0.3, sat:0.22},  // glass
}

MaterialPole current = interpolate(poles, material)  // floor/ceil blend

void processBlock(float* L, float* R, int n):
    for each sample i:
        inL = L[i]; inR = R[i]

        // Pre-filter: material tone (absorption shapes HF)
        filtL = onePoleLP(inL, current.absorb)
        filtR = onePoleLP(inR, current.absorb)

        // Early reflections (4 fixed taps, spacing from density)
        erL = tap(delayBufL, density * [20,40,65,95] ms)
        erR = tap(delayBufR, density * [22,43,68,102] ms)

        // Diffuse through 2 allpass stages
        diffL = allpass(allpass(erL, ms1), ms2)
        diffR = allpass(allpass(erR, ms1), ms2)

        // Decay feedback
        decayFb = exp(-6.9 / (current.decayMs * 0.001 * sr))
        fbL = onePoleLP(diffL * decayFb + fbL * decayFb, current.absorb)
        fbR = onePoleLP(diffR * decayFb + fbR * decayFb, current.absorb)

        // Glass fragility distortion
        if material > 2.5 and abs(inL) > fragility:
            fbL += waveshape_soft(fbL, satAmt * (material - 2.5))
            fbR += waveshape_soft(fbR, satAmt * (material - 2.5))

        // Sympathetic peak (resonant bandpass, ~250Hz)
        sympL = onePoleLP(SVF_BP(fbL, 250, sympathetic * 8), 0.8)
        sympR = onePoleLP(SVF_BP(fbR, 250, sympathetic * 8), 0.8)
        wetL = fbL * body + sympL * sympathetic * 0.3
        wetR = fbR * body + sympR * sympathetic * 0.3

        writeDelay(inL + wetL * 0.5)
        L[i] = inL * (1-mix) + wetL * mix
        R[i] = inR * (1-mix) + wetR * mix
```

---

## 3. fXGravity — "Sub-Harmonic Pull"

**Cookbook:** Cellar (Bass — gravitational coupling)
**Stage slot:** 19
**Parameter prefix:** `kfx_grv_`

### Concept

Sub-harmonics generated below the input signal act as a gravitational field — they pull the
signal downward. At low settings (`pull < 0.3`) this is warmth and body: the sub sits quietly
beneath, adding weight. At medium settings (`pull ~ 0.6`) the signal's pitch begins to feel
pulled — the fundamental seems heavier, slower, more rooted. At maximum pull the signal
literally bends downward in perceived pitch via a subtle frequency-domain manipulation: a
Hilbert-pair pitch shifter nudges the upper partial structure down by a semitone fraction,
while the sub-octave layer dominates.

Two sub-harmonic generators run simultaneously: an octave divider (f/2, square-wave sub) and
a fifth-below generator (f/3, rounded sub). The ratio between them is controlled by `kfx_grv_interval`.

### DSP Architecture

**Signal flow:**
```
input ──┬─────────────────────────────────────── dry ─┐
        │                                             │
        ├→ zero-crossing divider → f/2 sub (square)   │
        │   + 1-pole LP (150Hz) → SUB1                │
        │                                             │
        ├→ zero-crossing divider / 1.5 → f/3 sub      │
        │   + 1-pole LP (120Hz) → SUB2                │
        │                                             │
        ├→ sub blend: mix SUB1*(1-interval) + SUB2*interval → subOut │
        │                                             │
        ├→ pitch-bend path (pull > 0.5):              │
        │   minimal Hilbert downshift (-2 to -14 cents)│
        │   proportional to pull above 0.5 threshold  │
        │                                             │
        └→ sum → mix ──────────────────────────────────┘
```

**Zero-crossing sub-harmonic divider** (same approach used in fXObscura's sub-osc):
Track zero crossings. On every Nth crossing, toggle the sub-osc output polarity. N=2 gives f/2;
N=3 gives f/3. A leaky integrator smooths the resulting waveform from hard square to softer shape.

**Pitch bend via Hilbert downshift:** Uses the classic SSB/Hilbert pair at very small frequency
offsets (0–15 Hz shift, corresponding to 0–14 cents at 440Hz). Only active when pull > 0.5 to
keep CPU negligible when the gravitational field is light. This avoids a full phase vocoder —
it's a single frequency-shift stage.

**Denormal protection:** sub-osc state variables flushed per sample (critical in silence).

### Parameters

| Parameter ID | Range | Default | Description |
|---|---|---|---|
| `kfx_grv_pull` | 0.0–1.0 | 0.4 | Gravitational strength. 0=nothing, 0.5=audible sub, 1.0=signal bends downward |
| `kfx_grv_interval` | 0.0–1.0 | 0.3 | 0=pure octave-sub (f/2), 1=pure fifth-below (f/3), intermediate=blend |
| `kfx_grv_weight` | 0.0–1.0 | 0.5 | Sub-harmonic level relative to pull (how prominent the sub layer is vs pitch bend) |
| `kfx_grv_tightness` | 0.0–1.0 | 0.6 | LP cutoff on sub output. 0=wide open (dirty), 1=tight filtered (clean) |
| `kfx_grv_threshold` | -40.0–0.0 dB | -24.0 dB | Input level below which gravity turns off (silence doesn't drag) |
| `kfx_grv_mix` | 0.0–1.0 | 0.0 | Dry/wet |

### Cookbook Description

*Everything has a cellar. fXGravity adds one underneath your sound. At gentle pull you feel
the weight before you hear it — sub-harmonics occupying the space below your notes, giving
every frequency a foundation it didn't have. Increase the pull and the sound begins to bend
toward the floor — the Hilbert shifter nudges the pitch structure downward in fractions of a
semitone, as though the signal itself is being drawn into the earth. At maximum gravity you
are playing in the cellar. Everything is older here. Everything is heavier. Everything settles.*

### Pseudocode

```cpp
void processBlock(float* L, float* R, int n):
    for each sample i:
        monoIn = (L[i] + R[i]) * 0.5

        // Gate on threshold
        envState += envCoeff * (abs(monoIn) - envState)
        if envState < thresholdLinear: goto mix_only

        // f/2 sub-octave divider
        curPos = (monoIn > 0.0)
        if curPos != lastPos2:
            lastPos2 = curPos
            crossCount2++
            if crossCount2 % 2 == 0: subPhase2 = !subPhase2
        sub1Raw = subPhase2 ? 1.0 : -1.0
        sub1 = onePoleLP(sub1Raw, tightFreq)

        // f/3 sub-fifth divider
        crossCount3++ // (every 3rd crossing)
        if crossCount3 != lastPos3: ...  // similar logic, N=3
        sub2 = onePoleLP(sub3Raw, tightFreq * 0.8)

        subOut = sub1*(1-interval) + sub2*interval
        subOut *= pull * weight

        // Hilbert downshift (only when pull > 0.5)
        float shiftAmt = 0.0
        if pull > 0.5:
            shiftAmt = (pull - 0.5) * 2.0 * 15.0  // 0-15 Hz shift
            hilbertShiftedL = hilbert_SSB(L[i], -shiftAmt)
            hilbertShiftedR = hilbert_SSB(R[i], -shiftAmt)
            float bendMix = (pull - 0.5) * 2.0 * 0.4  // max 40% pitch bend blend
            L[i] = lerp(L[i], hilbertShiftedL, bendMix)
            R[i] = lerp(R[i], hilbertShiftedR, bendMix)

        wetL = L[i] + subOut
        wetR = R[i] + subOut
        mix_only:
        L[i] = L[i]*(1-mix) + wetL*mix
        R[i] = R[i]*(1-mix) + wetR*mix
```

---

## 4. fXGrowth — "Evolutionary Reverb"

**Cookbook:** Garden (Strings — evolutionary coupling)
**Stage slot:** 20
**Parameter prefix:** `kfx_grw_`

### Concept

The reverb tail is not static — it grows. Input signal accumulates over time into a density
accumulator, and the reverb's diffusion, decay, and spectral character evolve based on how much
signal has passed through in the recent past. A session that has played a lot has a lush, dense
reverb. One that has just started has a sparse, tentative reverb. During silence the reverb goes
dormant — not muted, but resting, like a garden in winter. When signal returns, it wakes slowly
from the accumulated state.

A "season" parameter cycles the spectral character over time: spring (bright, forward), summer
(full, dense), autumn (warm, thickening), winter (sparse, distant). The season timer can be
manual (frozen at any point) or auto-cycling over a user-defined period.

### DSP Architecture

**Signal flow:**
```
input → density accumulator (leaky integrator, charge/discharge asymmetric)
      → "growth level" (0=sparse, 1=lush)
      │
      ↓
4-tap Schroeder early reflections
  (tap count 1→4 proportional to growth level)
      │
      ↓
2-stage allpass diffuser
  (diffusion depth scales with growth: 0.1→0.8)
      │
      ↓
one-pole decay filter
  (decay time: 400ms at growth=0, 3500ms at growth=1)
      │
      ↓
season filter (SVF shelf sweep based on season)
      │
      ↓
dormancy gate (silence > dormancyThresholdMs → gentle fade)
      │
      ↓
mix with dry
```

**Density accumulator (the growth engine):**
```
growthTarget = inputLevel > playingThreshold ? 1.0 : 0.0
growthLevel += dt / (growthTarget > growthLevel ? riseTime : fallTime)
growthLevel = clamp(0..1)
```
- `riseTime`: how quickly new input builds density (~10 seconds default)
- `fallTime`: how quickly silence erodes density (~60 seconds default — slow decay)

**Season sweep:** A triangle LFO or manually-set position (0=spring, 0.25=summer, 0.5=autumn,
0.75=winter) modulates a single CytomicSVF shelf: high-shelf gain +3dB (spring) → 0dB
(summer) → -3dB (autumn) → -6dB (winter). Simultaneously, decay time scales from 100% to
80% at winter.

**Dormancy:** If input level falls below `dormancyThreshold` for `dormancyMs` milliseconds,
a slow exponential fade reduces the wet output by 50% over 2 seconds, simulating the reverb
"going to sleep." On signal return, it wakes at normal speed.

### Parameters

| Parameter ID | Range | Default | Description |
|---|---|---|---|
| `kfx_grw_riseTime` | 1.0–30.0 s | 10.0 s | How long it takes full playing density to reach lush reverb |
| `kfx_grw_fallTime` | 5.0–120.0 s | 60.0 s | How slowly density decays during silence |
| `kfx_grw_season` | 0.0–1.0 | 0.25 | Season position: 0=spring/bright, 0.25=summer/full, 0.5=autumn/warm, 0.75=winter/sparse |
| `kfx_grw_seasonRate` | 0.0–0.1 Hz | 0.0 | Auto-cycle rate. 0=frozen, >0=auto-season cycling |
| `kfx_grw_dormancy` | 0.0–1.0 | 0.5 | How aggressively the reverb sleeps during silence |
| `kfx_grw_mix` | 0.0–1.0 | 0.0 | Dry/wet |

### Cookbook Description

*Gardens grow. fXGrowth is a reverb that remembers what you've played. At the start of a session
the space is sparse — early spring, soil just turned. Play more and the reverb becomes denser,
fuller, more present — the garden growing in response to what you cultivate. Sit in silence and
the reverb goes dormant, withdrawing slowly like a plant in winter. Return and it wakes to where
it left off. The season parameter takes the reverb through four spectral characters — bright
spring shimmer, full summer density, warm autumn thickness, stripped winter clarity. Set a season
rate and it cycles on its own, giving every repeated take a different atmospheric quality.*

### Pseudocode

```cpp
float growthLevel = 0.0  // session state — persists across notes

void processBlock(float* L, float* R, int n):
    for each sample i:
        inL = L[i]; inR = R[i]
        monoLevel = (abs(inL) + abs(inR)) * 0.5

        // Update growth accumulator
        bool playing = (monoLevel > playingThreshold)
        float target = playing ? 1.0 : 0.0
        float timeConst = playing ? riseTime : fallTime
        growthLevel += (1.0/sr) / timeConst * (target - growthLevel)
        growthLevel = clamp(0..1)

        // Dormancy: sustained silence → fade wet
        if !playing: dormancyTimer++ else dormancyTimer = 0
        float dormancyGain = (dormancyTimer > dormancySamples)
            ? exp(-dormancy * dormancyTimer / dormancySamples)
            : 1.0

        // Build reverb with growth-scaled parameters
        float decayMs = lerp(400, 3500, growthLevel)
        float diffAmt = lerp(0.1, 0.8, growthLevel)
        int  numTaps  = 1 + int(growthLevel * 3)  // 1..4 taps

        // Season spectral shape
        float seasonAngle = season * 3.14159  // 0=spring, pi=winter
        float shelfGain = 1.0 + cos(seasonAngle) * 0.5  // +1 spring, -0.5 winter
        float seasonScale = 1.0 - sin(seasonAngle) * 0.2  // decay shorter in winter

        // Process reverb network with grown parameters
        float erL = earlyReflections(inL, numTaps, decayMs * seasonScale)
        float erR = earlyReflections(inR, numTaps, decayMs * seasonScale)
        float diffL = allpassDiffuse(erL, diffAmt)
        float diffR = allpassDiffuse(erR, diffAmt)
        float wetL = shelfFilter(diffL, shelfGain) * dormancyGain
        float wetR = shelfFilter(diffR, shelfGain) * dormancyGain

        L[i] = inL*(1-mix) + wetL*mix
        R[i] = inR*(1-mix) + wetR*mix

    // Auto season cycling
    if seasonRate > 0:
        season += seasonRate / sr
        if season > 1.0: season -= 1.0
```

---

## 5. fXReduction — "Session Erosion"

**Cookbook:** Broth (Pads — cooperative/time-based coupling)
**Stage slot:** 21
**Parameter prefix:** `kfx_red_`

### Concept

The effect accumulates session time and irreversibly (within the session) reduces the signal.
High frequencies leave first — a one-pole LP cutoff slowly sweeps downward from `startFreq`
to `endFreq` over the `erosionTime` session duration. A subtle saturation rises as the reduction
deepens — as though the broth is concentrating, thickening, becoming more intense even as it
simplifies. A `reset` button (momentary trigger parameter) clears the accumulated state, returning
to full fidelity — but the LP ramp restarts from the beginning.

This maps directly to the Broth quad's cooperative philosophy: not competitive, not destructive,
just the inevitable physics of time. The signal transforms through immersion in a session.
The change is too slow to notice moment-to-moment but unmistakable over an hour.

**IMPORTANT:** `erosionLevel` (0.0–1.0) is a session-persistent state variable, not a
parameter. It accumulates in the class member and is NOT reset on plugin state restore — it is
intentionally lost on plugin restart. This is by design: a new session starts fresh.

### DSP Architecture

**Signal flow:**
```
input → one-pole LP (cutoff: lerp(startFreq, endFreq, erosionLevel))
      → soft saturation (drive: lerp(0, saturationMax, erosionLevel))
      → mix with dry
```

**Erosion accumulator:**
```
erosionLevel += (1.0/sr) / (erosionTime * 60.0)  // erosionTime in minutes
erosionLevel = clamp(0..1)
```

Intentionally simple. One state variable. Linear ramp over the session.

**Saturation:** Uses `fastTanh` at low drive (1.0–1.8 range). The saturation compensates
partial perceived volume loss from LP rolloff — the broth concentrates as it reduces.

**Reset trigger:** When `kfx_red_reset` transitions true → true on any sample:
```
erosionLevel = 0.0
```
This is the only user-controllable state reset.

### Parameters

| Parameter ID | Range | Default | Description |
|---|---|---|---|
| `kfx_red_erosionTime` | 5.0–120.0 min | 30.0 min | Session duration over which full erosion occurs |
| `kfx_red_startFreq` | 2000–20000 Hz | 14000 Hz | LP cutoff at start of session (before erosion) |
| `kfx_red_endFreq` | 200–4000 Hz | 900 Hz | LP cutoff at full erosion |
| `kfx_red_saturation` | 0.0–1.0 | 0.5 | Max saturation amount at full erosion |
| `kfx_red_reset` | bool | false | Momentary: resets erosionLevel to 0.0 when toggled true |
| `kfx_red_mix` | 0.0–1.0 | 0.0 | Dry/wet |

### Cookbook Description

*The broth reduces. It starts with everything — full frequency content, clean signal, the whole
ingredient list. Over the session it simplifies. Not abruptly, not dramatically — it's a low
simmer. The high frequencies go first, bleached by time the way stock loses its lightest volatiles
first. The sound becomes warmer, darker, more concentrated. A subtle saturation replaces what's
lost — intensity where breadth used to be. By the end of a long session the signal is reduced to
its essence: deep, thick, fundamental. Hit reset and the broth starts fresh — full-spectrum,
patient, with the whole session ahead of it. fXReduction is the only effect in XOceanus that
knows how long you've been playing.*

### Pseudocode

```cpp
float erosionLevel = 0.0  // session state — never saved/restored

void processBlock(float* L, float* R, int n):
    for each sample i:
        inL = L[i]; inR = R[i]

        // Handle reset trigger (edge detect)
        if resetTrigger and !lastResetState:
            erosionLevel = 0.0
        lastResetState = resetTrigger

        // Accumulate erosion
        erosionLevel += 1.0 / (sr * erosionTimeSec)
        erosionLevel = clamp(0..1)

        // LP cutoff from erosion
        float cutoff = lerp(startFreq, endFreq, erosionLevel)
        float lpCoeff = exp(-2*PI*cutoff / sr)
        lpStateL = flushDenormal(lpStateL + (1-lpCoeff) * (inL - lpStateL))
        lpStateR = flushDenormal(lpStateR + (1-lpCoeff) * (inR - lpStateR))

        // Saturation (compensating concentration)
        float drive = 1.0 + saturation * erosionLevel * 0.8
        float wetL = fastTanh(lpStateL * drive) / drive
        float wetR = fastTanh(lpStateR * drive) / drive

        L[i] = inL*(1-mix) + wetL*mix
        R[i] = inR*(1-mix) + wetR*mix
```

---

## 6. fXMigration — "Cultural Spatial Processor"

**Cookbook:** Fusion (Electric Pianos — migratory coupling)
**Stage slot:** 22
**Parameter prefix:** `kfx_mig_`

### Concept

A spatial/reverb processor that places the signal in a "cultural space" — a sonic representation
of a musical tradition's acoustic context. Four poles: East, South, West, North. Each pole is a
combination of specific room character parameters derived from music's listening environments in
those traditions. The `axis` parameter morphs continuously between any two adjacent poles, and
a `latitude` parameter does the orthogonal blend.

East (0.0): Tight, precise, close. The Japanese concert hall or Chinese tea-house — intimate
reflections, minimal decay, sound is placed and specific.
South (0.25): Warm, present, open-air feel. African courtyard, Caribbean room — mid-room
size, wood absorption, natural air.
West (0.5): Open, wide, expansive. European concert hall or American stadium — long decay,
wide stereo spread, the sound of a thousand-seat space.
North (0.75): Cold, distant, reverberant. Nordic church or arctic space — long decay with
slow attack, HF absorption, the sound of stone and silence.

The East→West axis is the primary morph axis (longitude). North→South is orthogonal (latitude).
This gives a 2D cultural coordinate with four extreme points and infinite intermediate spaces.

### DSP Architecture

**Signal flow:**
```
input → pre-filter (HF absorption: varies by cultural pole)
      → early reflection pattern (tap spacing & density per pole)
      → allpass diffuser (diffusion amount per pole)
      → late reverb (decay time + stereo width per pole)
      → post-EQ (spectral signature per pole)
      → mix with dry
```

**4 cultural poles** (all parameters interpolated bilinearly from 2D coordinate):
```
              North (0.75)
               cold/stone
           decayMs=4800
           spread=0.3
           absorption=0.5
           preDelay=35ms
               ↑
West ←────────┼────────→ East
(open hall)   │           (tight/precise)
decayMs=2800  │           decayMs=700
spread=0.9    │           spread=0.15
absorb=0.15   │           absorb=0.85
preDelay=12ms │           preDelay=4ms
               ↓
              South (0.25)
              warm/present
           decayMs=1200
           spread=0.55
           absorption=0.45
           preDelay=8ms
```

**Bilinear interpolation:** Given `longitude` (0=East, 0.5=West) and `latitude`
(0=South, 0.5=North), compute weights for all 4 poles and interpolate all reverb parameters.

**Stereo width:** Implemented as a Mid/Side matrix: `wideOut = MS_decode(MS_encode(wet), spread)`.
Pole spread values above define the S channel scaling.

**Early reflection patterns:** Each pole has a unique pattern derived from the acoustic
traditions of that cultural space. East: 3 close taps (4ms, 8ms, 14ms). South: 4 medium taps
with wood absorption (12ms, 25ms, 38ms, 52ms). West: 5 wide taps (15ms, 28ms, 44ms, 65ms, 88ms).
North: 3 distant taps with cold absorption (20ms, 50ms, 95ms).

### Parameters

| Parameter ID | Range | Default | Description |
|---|---|---|---|
| `kfx_mig_longitude` | 0.0–1.0 | 0.5 | East (0.0) → West (1.0) axis. The primary cultural morph |
| `kfx_mig_latitude` | 0.0–1.0 | 0.5 | South (0.0) → North (1.0) axis |
| `kfx_mig_exchange` | 0.0–1.0 | 0.3 | Cultural exchange rate — how much adjacent poles bleed into each other beyond the interpolation (creates hybrid sonic identities) |
| `kfx_mig_preDelay` | 0.0–1.0 | 0.5 | Pre-delay scalar applied to pole's base pre-delay (0=tighter, 1=more spacious) |
| `kfx_mig_airiness` | 0.0–1.0 | 0.4 | HF content in wet signal — 0=all HF absorbed by space, 1=bright open air |
| `kfx_mig_mix` | 0.0–1.0 | 0.0 | Dry/wet |

### Cookbook Description

*Sound doesn't exist in a vacuum — it exists in a place, and places have cultures. fXMigration
is a spatial processor that places your signal in a cultural acoustic space. The longitude axis
moves from tight East (the precision of a Japanese recital hall, a Beijing conservatory, a Tokyo
jazz club) to open West (the breadth of a Vienna concert hall, a Nashville arena, a California
outdoor stage). The latitude axis moves from warm South (the wood-and-courtyard rooms of West
Africa, the Caribbean, equatorial music spaces) to cold North (Nordic stone churches, the
reverberant silences of Arctic tradition). Navigate freely between all four: a sound positioned
at 0.3 longitude / 0.7 latitude is in a cold but not fully northern space — perhaps a Scottish
stone room, a Canadian church, a late-night Oslo bar. The exchange parameter determines how
much the poles bleed into each other — with high exchange, you are at the crossroads where
traditions genuinely mix. This is what fusion cuisine does: not replacement, but dialogue across
distance.*

### Pseudocode

```cpp
struct CulturalPole { float decayMs, spread, absorb, preDelayMs; float tapMs[5]; int numTaps; };
CulturalPole poles[4] = {
    /* East  */ {700,   0.15, 0.85, 4.0,  {4,8,14,0,0},           3},
    /* South */ {1200,  0.55, 0.45, 8.0,  {12,25,38,52,0},         4},
    /* West  */ {2800,  0.90, 0.15, 12.0, {15,28,44,65,88},        5},
    /* North */ {4800,  0.30, 0.50, 35.0, {20,50,95,0,0},          3},
}

void processBlock(float* L, float* R, int n):
    // Bilinear interpolation of poles
    //   longitude 0=East, 1=West
    //   latitude  0=South, 1=North
    float wE = (1-longitude) * (1-latitude)
    float wS = (1-longitude) *    latitude
    float wW =    longitude  * (1-latitude)
    float wN =    longitude  *    latitude
    // Wait — correct 2D bilinear:
    // East=longitude near 0, West=longitude near 1
    // South=latitude near 0, North=latitude near 1
    float wES = (1-longitude) * (1-latitude)  // East-South
    float wEN = (1-longitude) *    latitude   // East-North
    float wWS =    longitude  * (1-latitude)  // West-South
    float wWN =    longitude  *    latitude   // West-North
    CulturalPole cur = blend(poles[East]*wES + poles[North_corner]*wEN
                            + poles[West]*wWS + poles[NW]*wWN)
    // (+exchange adds slight contribution from all 4 poles regardless)

    for each sample i:
        inL = L[i]; inR = R[i]

        // Pre-delay
        preL = delayRead(inL, cur.preDelayMs * preDelay * sr / 1000)
        preR = delayRead(inR, cur.preDelayMs * preDelay * sr / 1000)

        // Absorption pre-filter
        preL = onePoleLP(preL, 1.0 - cur.absorb * (1.0 - airiness))
        preR = onePoleLP(preR, 1.0 - cur.absorb * (1.0 - airiness))

        // Early reflections (variable tap count)
        erL = sumTaps(preL, cur.tapMs, cur.numTaps)
        erR = sumTaps(preR, cur.tapMs, cur.numTaps)

        // Late reverb (1 allpass + 1 decay stage)
        diffL = allpass(erL, 27ms)
        diffR = allpass(erR, 29ms)
        decayCoeff = exp(-6.9 / (cur.decayMs * sr / 1000))
        fbL = decayCoeff * (diffL + fbL)
        fbR = decayCoeff * (diffR + fbR)

        // Stereo width via M/S
        float M = (fbL + fbR) * 0.5
        float S = (fbL - fbR) * 0.5 * cur.spread
        wetL = M + S
        wetR = M - S

        L[i] = inL*(1-mix) + wetL*mix
        R[i] = inR*(1-mix) + wetR*mix
```

---

## Integration Summary

### MasterFXChain Stage Mapping

The six companion FX form a `KitchenFXChain` group. Suggested insertion point:
between the existing Boutique chain (stages 13–16) and the existing Singularity chain.

```
... (stages 1-16: Math, Aquatic, Boutique)
Stage 17 — fXAdversary   (Chef/Organ cookbook)
Stage 18 — fXResonance   (Kitchen/Piano cookbook)
Stage 19 — fXGravity     (Cellar/Bass cookbook)
Stage 20 — fXGrowth      (Garden/Strings cookbook)
Stage 21 — fXReduction   (Broth/Pads cookbook)
Stage 22 — fXMigration   (Fusion/EP cookbook)
Stage 23 — fXOnslaught   } (existing Singularity chain,
Stage 24 — fXObscura     }  renumbered from 17-19
Stage 25 — fXOratory     }  to 23-25)
Stage 26 — Bus Compressor
Stage 27 — Brickwall Limiter
```

### Parameter Prefix Convention

All six FX share the `kfx_` namespace prefix. Within that namespace each uses a 3-letter
shorthand: `adv_`, `res_`, `grv_`, `grw_`, `red_`, `mig_`. Full parameter IDs:

```
kfx_adv_rivalry, kfx_adv_refreshMs, kfx_adv_duckFloor, kfx_adv_peakCeil,
kfx_adv_bandwidth, kfx_adv_mix

kfx_res_material, kfx_res_body, kfx_res_room, kfx_res_fragility,
kfx_res_sympathetic, kfx_res_mix

kfx_grv_pull, kfx_grv_interval, kfx_grv_weight, kfx_grv_tightness,
kfx_grv_threshold, kfx_grv_mix

kfx_grw_riseTime, kfx_grw_fallTime, kfx_grw_season, kfx_grw_seasonRate,
kfx_grw_dormancy, kfx_grw_mix

kfx_red_erosionTime, kfx_red_startFreq, kfx_red_endFreq, kfx_red_saturation,
kfx_red_reset, kfx_red_mix

kfx_mig_longitude, kfx_mig_latitude, kfx_mig_exchange,
kfx_mig_preDelay, kfx_mig_airiness, kfx_mig_mix
```

### CPU Estimate

| Effect | Core Operations | Est. CPU @ 48kHz |
|---|---|---|
| fXAdversary | 4× SVF analysis + 4× gain + RMS | ~0.4% |
| fXResonance | 4 taps + 2× allpass + 1× SVF + LP | ~0.5% |
| fXGravity | 2× ZC divider + 1× LP + partial Hilbert | ~0.6% |
| fXGrowth | 4 taps + 2× allpass + SVF shelf | ~0.5% |
| fXReduction | 1× LP + fastTanh | ~0.1% |
| fXMigration | 5 taps + 1× allpass + LP + M/S | ~0.5% |
| **Total (all 6 active)** | | **~2.6%** |

Each FX exits early at `mix < 0.001f` — typical sessions with 2–3 active will run at 1.0–1.5%.

### Shared Dependencies

All six require:
- `FastMath.h` — `fastExp`, `fastTanh`, `fastSin`, `fastCos`, `flushDenormal`, `lerp`, `clamp`
- `CytomicSVF.h` — filter operations (fXAdversary, fXResonance, fXGrowth, fXMigration)
- `StandardLFO.h` — optional for fXGrowth season auto-cycle

No JUCE dependencies in the DSP core — all six are testable in isolation.

---

*Design document only. No source files created.*
*Next step: Architect review → KitchenFXChain scaffold → individual FX implementation.*
