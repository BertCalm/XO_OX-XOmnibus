# Scripture — OUTLOOK Retreat Chapter
*Revealed 2026-03-24 during the Albatross Awakening*

## Engine Identity
- **Name:** OUTLOOK (XOutlook) — Panoramic Visionary Synth
- **Creature:** The Albatross — Surface Soarer
- **Accent:** Horizon Indigo `#4169E1`
- **Polarity:** 25% feliX / 75% Oscar
- **DSP Signature:** Dual wavetable counter-scanning, parallax stereo field, vista filter, aurora conjunction modulation

---

## Verses Revealed

### Book I — The Oscillator Verses

**Verse 12: The Counter-Scan**
> *Two wavetables scanning in opposite directions are not simply "detuned" or "layered." They are in timbral conversation. When one brightens, the other darkens. The interference between these opposing spectral trajectories is richer than any unison, deeper than any chorus, and costs zero additional CPU. The counter-scan is a gift from Parsimonia.*

**Application:** Any engine with dual oscillators can benefit from counter-scanning. Set osc1's morph/scan/position to increase with a parameter while osc2 decreases with the same parameter. The spectral interference creates evolution without additional processing.

---

### Book II — The Filter Psalms

**Psalm 15: The Vista Principle**
> *A filter that opens in one direction while the oscillator evolves in the opposite direction creates spectral parallax — the listener perceives depth because the brightness gradient disagrees with the timbral gradient. This is the acoustic equivalent of motion parallax in vision. Use it whenever dual oscillators are present: scan them in opposition, filter in a third direction.*

**Application:** When designing filter sweeps for counter-scanned oscillators, modulate the filter in a direction that creates spectral disagreement. If the oscillators are brightening/darkening via scan, move the filter cutoff orthogonally (e.g., from resonance rather than cutoff) to create a three-dimensional timbral space.

---

### Book III — The Modulation Sutras

**Sutra 15: The Coprime Conjunction**
> *When two LFOs modulate different targets — one the spectrum, one the amplitude — their conjunction is a third modulation that exists nowhere in the parameter list. Phase-lock destroys this ghost modulation by making it periodic. Coprime rates resurrect it by making it perpetually surprising. In OUTLOOK, conjunction is luminosity. In all engines, conjunction is the modulation you didn't design but should have.*

**Application:** Before shipping any engine with 2+ LFOs, verify the default rates are coprime (or at least high-ratio). Test: multiply both rates by 1000, round to integers, compute GCD. If GCD > 5, the rates will phase-lock perceptibly. OUTLOOK's 67:113 (GCD=1) is ideal. Avoid clean ratios like 2:3, 3:5, 4:7.

**Sutra 16: The Movement Floor**
> *An LFO that modulates a filter through a depth parameter controlled by a macro will die when the macro is at zero — unless a floor is built into the code. The floor must be chosen with care: too high (0.3) and the macro feels broken, too low (0.01) and the engine appears static. The blessed floor is 0.1: enough to prove the engine breathes, quiet enough that the macro's full range feels like a journey from whisper to wind.*

**Application:** When implementing macro→LFO depth routing, always include `std::max(macroValue, 0.1f)` or equivalent. This ensures D005 breathing even when the macro is at zero. The floor of 0.1 was calibrated during the OUTLOOK retreat: at 0.1 × typical LFO depth (0.3–0.4), the filter modulation is 3–4% — below conscious perception but above the "static" threshold.

---

### Book V — The Stewardship Canons

**Canon 12: The Nine Gifts**
> *Nine parameter changes. Zero CPU added. Parsimonia teaches: the first optimization is not algorithmic. It is sonic. Before you rewrite the DSP, ask whether the current DSP is being asked the right questions. A filter at the wrong cutoff is not a slow filter — it is a misunderstood filter. Change the question before changing the code.*

**Application:** Before any CPU optimization pass, first audit the parameter defaults. Most engines can be improved 20–30% in perceived quality through parameter refinement alone. The retreat's 9 refinements (LFO rates, depths, envelope times, reverb levels, macro defaults) transformed OUTLOOK from a generic pad into a panoramic instrument — at zero CPU cost.

---

## OUTLOOK-Specific Truths

### The Parallax Stereo Sweet Spot
Parallax amount between 0.55–0.9 produces the most musical stereo depth. Below 0.4, the effect is too subtle for most monitoring setups. Above 0.9, high notes can feel disconnected from the stereo center. The sweet spot for mixed-register playing (chords spanning 2+ octaves) is 0.65.

### The Aurora Conjunction Window
At default coprime rates (0.067 Hz / 0.113 Hz), a full conjunction (both LFOs simultaneously at peak) occurs approximately every 132 seconds. Partial conjunctions (both LFOs > 0.5) occur roughly every 30 seconds. These partial conjunctions produce luminosity surges of ~3-5% amplitude — perceived as the sound "brightening" without the listener identifying why.

### The Reverb-Parallax Trade-off
Reverb wash smears the parallax stereo field. Total reverb (reverbMix + macroSpace) above 0.5 begins to collapse the stereo depth that parallax creates. For presets that showcase parallax, keep total reverb at 0.45 or below. For ambient presets that prioritize space over depth, reverb can go to 0.7+ but parallaxAmount should be reduced proportionally.

### Best Coupling Partners (Verified)
1. **OPENSKY** (AmpToFilter): OPENSKY's shimmer tail feeds OUTLOOK's vista filter — spectral vista breathes with the shimmer rhythm
2. **OMBRE** (EnvToMorph): OMBRE's memory envelope modulates horizon scan — past notes change OUTLOOK's timbral character
3. **OPAL** (LFOToPitch): OPAL's granular LFO modulates parallax depth — grain density controls stereo width
4. **OXBOW** (AmpToFilter): OXBOW's entangled reverb tail feeds into vista filter — golden-ratio decay creates evolving filter modulation

---

*The Albatross soars on borrowed wind. OUTLOOK synthesizes on borrowed modulation. Both are richer for what they receive.*

*— Guru Bin, 2026-03-24*
