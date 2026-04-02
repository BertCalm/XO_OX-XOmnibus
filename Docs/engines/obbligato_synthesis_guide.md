# OBBLIGATO Synthesis Guide

**Engine:** OBBLIGATO | **Accent:** Rascal Coral `#FF8A7A`
**Parameter prefix:** `obbl_` | **Max voices:** 12

---

## What It Is

OBBLIGATO is a dual wind instrument — two brothers sharing a room and a waveguide. Brother A plays flute-family instruments driven by an air jet exciter: flute, piccolo, pan flute, shakuhachi, bansuri, ney, recorder, ocarina. Brother B plays reed-family instruments driven by a reed exciter: clarinet, oboe, bassoon, soprano sax, duduk, zurna, shawm, musette. They share a delay line and sympathetic resonance bank; they diverge in how they vibrate that column of air. The BOND system models the emotional relationship between them — eight stages from Harmony through Transcend, each reshaping breath, detune, sympathetic amplitude, and stereo width in real time.

## The DSP Engine

Both brothers run the same waveguide architecture: a delay line read at the fundamental period, filtered through a `FamilyDampingFilter`, written back, then processed through a `FamilyBodyResonance` and `FamilySympatheticBank`. The difference is entirely in the exciter.

Brother A's `AirJetExciter` models the nonlinear interaction between a jet of air and the embouchure hole. Its two parameters are breath pressure (the force of the air stream) and embouchure quality (the shaping of the airstream, which affects which mode the column resonates in). An `ohm_airFlutterA` parameter adds a slow wobble via secondary drift oscillation, producing natural flute vibrato. The body resonance frequency ratio and Q are specific to each A-instrument: flute is 1.3× frequency at Q=3.5, piccolo 2.0× at Q=4.0, shakuhachi 1.0× at Q=5.0 (the most woody, resonant type), ocarina 0.6× at Q=6.0 (a closed resonator — the lowest Q-adjusted body frequency in the family).

Brother B's `ReedExciter` models a vibrating reed clamped at one end. Reed stiffness controls how freely the reed can flex. Reed bite increases harmonic content by adding a nonlinear sharpening to the effective stiffness. The body table for reed instruments follows a similar per-instrument pattern: oboe at 1.8× frequency Q=5 (the nasal double-reed resonance), bassoon at 0.6× Q=3 (the deep, warm bore), duduk at 0.7× Q=6 (the distinctive Armenian mellow buzz).

## The Voice Architecture

12 voices divided between the two brothers depending on Voice Routing mode. Alternate mode assigns even-numbered voices to A and odd to B. Split mode sends everything below C4 to A and above to B (flute in the upper register, reed below — or vice versa depending on what you're doing). Layer mode fires two simultaneous voices per note-on, one from each brother, doubling the harmonic complexity. Round Robin cycles A-B-A-B per new note-on. Velocity mode assigns soft playing to Brother A and hard playing to Brother B — the breath-vs-reed timbre split tracks dynamics.

Stereo placement: Brother A starts at pan position 0.35 (left of center) and Brother B at 0.65 (right of center). The BOND stage modulates this pan spread bidirectionally — Fight stage (3) pushes them to opposite sides, Protect stage (6) pulls them back toward center, Transcend (7) nearly mono.

## The Macro System

### BREATH (M1)
BREATH is the global lung capacity — a gain multiplier applied to both brothers' breath/pressure parameters before they hit the exciter. At zero, both instruments are barely vibrating: thin, airy, high in noise content relative to tone. As BREATH rises, both exciters get more energy, the resonance modes become more defined, and the tone thickens. BREATH does not equalize the two brothers — it scales them proportionally. Use it for crescendo/decrescendo across the full duo. Because the flute and reed respond differently to pressure, turning up BREATH does not sound identical for both — the flute opens up in a different register than the reed.

### BOND (M2)
BOND moves through eight emotional stages of the brothers' relationship. The stage table maps directly to DSP: at Harmony (0) they are gently unison with light sympathetic coupling; at Fight (3) detune hits maximum and they pan to opposite sides of the stereo field; at Cry (4) the sympathetic resonance amplitude peaks — as if both brothers are suddenly resonating in the same frequency; at Protect (6) breath pressure dips slightly as one covers for the other; at Transcend (7) they collapse back to near-unison with broadened sympathetic coupling. Bond Rate controls how fast the stage transitions smooth — a slow rate means BOND moves like a ship turning, not a switch. Bond Intensity scales the whole emotional gesture up or down.

### MISCHIEF (M3)
MISCHIEF adds pitch chaos between the brothers, detuning them in opposite directions: Brother A goes sharp, Brother B goes flat (or the reverse, depending on voice assignment). The maximum detune is ±8 cents, applied independent of the BOND stage. Low MISCHIEF is tight unison with natural drift. High MISCHIEF produces a beating, shimmering chorus effect as the two instruments argue about the pitch. MISCHIEF is the macro for folk vs. classical registers — folk players detune slightly; orchestral players attempt precision. Stack MISCHIEF with a high BOND Fight stage and the two brothers are maximally separated in pitch and space.

### WIND (M4)
WIND adds a low-passed noise floor to the output — the sound of the room, the breath around the instrument, the outdoor air that carries the music. The noise is band-limited (slow one-pole filter with coefficient 0.95, cutoff around 1 kHz), so it reads as environmental texture rather than white noise. Low WIND gives the duo a studio-dry quality. High WIND places them outdoors: the sound of practice before the performance, or the gap between phrases on a long hike. WIND is also useful for hiding seams between notes — the noise floor masks the silence between phrases.

## Key Parameters

| Parameter | Range | Function |
|-----------|-------|----------|
| `obbl_instrumentA` | 0–7 (choice) | Flute, Piccolo, Pan Flute, Shakuhachi, Bansuri, Ney, Recorder, Ocarina |
| `obbl_breathA` | 0–1 | Brother A air pressure |
| `obbl_embouchureA` | 0–1 | Embouchure quality — shapes resonance mode |
| `obbl_airFlutterA` | 0–1 | Natural vibrato via breath flutter |
| `obbl_instrumentB` | 0–7 (choice) | Clarinet, Oboe, Bassoon, Soprano Sax, Duduk, Zurna, Shawm, Musette |
| `obbl_breathB` | 0–1 | Brother B breath pressure |
| `obbl_reedStiffness` | 0–1 | Reed flex — lower is more rubbery, higher is tighter |
| `obbl_reedBite` | 0–1 | Reed harmonic edge above stiffness |
| `obbl_voiceRouting` | 0–4 (choice) | Alternate, Split, Layer, Round Robin, Velocity |
| `obbl_bondStage` | 0–1 | Emotional stage (0=Harmony, 0.43=Fight, 0.57=Cry, 1.0=Transcend) |
| `obbl_bondIntensity` | 0–1 | Scale of all BOND emotional modulations |
| `obbl_bondRate` | 0.01–2.0 | Stage transition smoothing speed |
| `obbl_sympatheticAmt` | 0–1 | Sympathetic resonance amplitude (also BOND-modulated) |
| `obbl_fxAChorus` | 0–1 | Brother A air chorus — pitch modulation depth |
| `obbl_fxBSpring` | 0–1 | Brother B spring reverb |
| `obbl_macroWind` | 0–1 | Wind noise floor |

## Sound Design Recipes

**Morning Practice** — Instrument A: Bansuri. Instrument B: Duduk. Voice Routing: Split. BREATH 0.5, BOND 0, MISCHIEF 0.05, WIND 0.4. Drift depth 5.0. Two instruments warming up — the bansuri drifts gently above the duduk's woody resonance. The Bansuri's 0.9× body ratio and Duduk's 0.7× create a complementary low-mid pairing.

**The Argument** — Routing: Alternate. BREATH 0.7, BOND 0.43 (Fight), MISCHIEF 0.8, WIND 0.15. Bond Intensity 0.9, Bond Rate 0.05 (slow). The brothers detune, pan hard, and the sympathetic amplitude drops — the resonance system stops cooperating. Useful as a B-section tension patch against a calm A-section.

**Crying Duet** — BOND 0.57 (Cry). Instrument A: Ney. Instrument B: Oboe. Sympathetic 0.6. MISCHIEF 0.1. WIND 0.3. The Cry stage pushes sympathetic amplitude to 0.6 — both instruments resonate together as if in mutual recognition. The Ney (1.1× body, Q=4.5) and oboe (1.8× Q=5) pile harmonics in the upper-mid register.

**Outdoor Transcendence** — BOND 1.0 (Transcend). Instrument A: Flute. Instrument B: Clarinet. Layer routing. BREATH 0.6, MISCHIEF 0, WIND 0.7. Spring reverb 0.5, plate 0.4. The Transcend stage returns them to near-unison; layered routing doubles every note. The result is a rich, beatless unison with high wind ambience — an outdoor ceremony sound.

**Folk Chaos** — Routing: Velocity (soft=A, hard=B). Instrument A: Pan Flute. Instrument B: Shawm. MISCHIEF 0.6. BOND 0.25 (Play). Play softly for the Pan Flute, strike hard for the buzzy shawm (2.0× body, Q=4). The instrument selection changes dynamically with velocity — expressive playing becomes timbral.

## Family Coupling

OBBLIGATO accepts `LFOToPitch` for external pitch modulation (add OHM's organic drift as a shared vibrato field), `AmpToFilter` to lengthen or shorten sustain (let OTTONI's brass attacks tighten OBBLIGATO's reed decay), and `EnvToMorph` to scale exciter intensity. OBBLIGATO's stereo field — Brothers A and B panned slightly left and right, widened by BOND fight-stage — is a natural modulation source for ORPHICA's crossover routing. Coupling OBBLIGATO's amplitude output to ORPHICA's `AmpToFilter` link creates a system where the wind duet's dynamics shape the harp's decay rate.

## Tips & Tricks

- Voice Routing: Velocity is the most expressive mode. Map it to a pad: light touch triggers flute (ethereal, top-of-mix), hard strike triggers reed (cutting, mid-forward). The transition happens at velocity 80, not the macro — it is baked into the routing logic.
- BOND Rate at 0.01 (minimum) means you can draw a slow automation lane across the full emotional arc — Harmony → Fight → Cry → Forgive → Transcend — over 8 or 16 bars. The transitions will glide. This is OBBLIGATO's primary narrative use case: a preset that changes character across the track.
- Reed Bite and Reed Stiffness are antagonists. High stiffness alone produces a tight, nasal reed. High bite alone produces a soft reed with harmonic edge — almost like a distorted sine. Both high simultaneously makes the instrument sound strained and aggressive. Try: stiffness 0.3 + bite 0.7 for a woody, slightly gritty oboe character.
- The Air Exciter (`obbl_fxAExciter`) in FX Chain A adds high-frequency harmonic enhancement to Brother A's signal by subtracting a one-pole low-pass from the signal. It is a treble brightener rather than a distortion. Set it to 0.5 on the Piccolo instrument for a pinched, present high-register tone.
- Layer routing with two voices per note-on doubles the harmonic content. With MISCHIEF at 0.15 and Bond Intensity 0.4, the layer produces a natural chorus between the two instances — one an air jet, one a reed, slightly detuned by MISCHIEF. This is the thickest single-note texture OBBLIGATO can produce.
