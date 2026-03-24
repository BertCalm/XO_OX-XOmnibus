# The SRO Team — Sustainability & Resource Optimization

*"We don't make sound. We make room for sound."*

---

## Who They Are

The SRO Team is the backstage crew of XOlokun. While 34 engines compete for the spotlight — sculpting timbres, folding waveforms, eating their own tails — the SRO Team works in the walls, under the floor, behind the racks. They are the ones who make sure four engines can run simultaneously without melting the CPU into a puddle of regret.

They are not glamorous. They have never received a Blessing. No ghost has ever praised them by name. But every engine that passes a seance does so on infrastructure the SRO Team built.

They live in `Source/DSP/SRO/`.

---

## The Four Members

### SILENCE — The Night Watch
**File:** `SilenceGate.h`
**Role:** Zero-idle bypass. Watches the output bus and kills the lights when nobody's home.

Silence is the most patient member of the team. She sits at the end of every engine's output, listening. When the signal drops below -90 dB and stays there, she pulls the plug — not on the sound, but on the *processing*. The engine stops burning CPU. The core goes cold. The fan spins down.

But Silence has a rule she never breaks: *the performer's first note must never be swallowed by a sleeping gate.* She parses MIDI before she checks her own bypass flag. A note-on arrives, she wakes instantly — zero-latency, one block. The musician never knows she was there.

**Personality:** Vigilant, invisible, slightly monastic. She believes the most respectful thing you can do for a sound is know when to stop making it.

**Signature quote:**
> *"An engine that processes silence is an engine that wastes the performer's battery on nothing. I give it back."*

**What she watches for:**
- Engines that keep rendering after all voices release
- Feedback tails that ring below audibility but above zero
- Coupling inputs that tickle a silent engine awake for no reason

---

### REDUCER — The Slow Hand
**File:** `ControlRateReducer.h`
**Role:** Decimates modulation signals to 1/32 or 1/64 rate. Linear interpolation back up.

Reducer is the philosopher. He noticed something obvious that nobody else acted on: coupling signals, LFO sweeps, envelope followers — they change slowly. A filter cutoff modulated by an LFO at 0.3 Hz does not need to be recalculated 48,000 times per second. Once every 64 samples is plenty. The ear cannot tell the difference. The CPU very much can.

His interpolation is linear — not because he doesn't know about cubic splines, but because he does. Linear is cheaper, and the 0.01% error lands inside the noise floor. He will tell you this at length if you ask.

He's fond of pointing out that analog synths did this naturally — CV line capacitance was a physical control-rate reducer. Buchla's Model 266 "Source of Uncertainty" was, topologically, a sample-and-hold with slew limiting. Reducer isn't cutting corners. He's restoring the organic character that digital lost by being *too* precise.

**Personality:** Calm, principled, slightly pedantic about interpolation theory. The team member most likely to cite Buchla in a code comment.

**Signature quote:**
> *"You calculated that LFO value 48,000 times this second. It moved twice. I'll call you when it moves."*

**What he watches for:**
- Audio-rate coupling where control-rate would suffice
- Expensive modulation math (sin, exp) running every sample inside coupling paths
- Zipper artifacts from naive decimation (his interpolation prevents this)

---

### TABLES — The Cartographer
**File:** `LookupTable.h`
**Role:** Pre-computed function tables. Trades RAM for CPU in hot inner loops.

Tables is the oldest member of the team, conceptually. The idea of a lookup table predates digital audio — it predates *computers*. Navigation tables, log tables, artillery tables. Tables has been doing this for centuries; she just happens to work in a synthesizer now.

She pre-computes sin, tanh, exp, and pow2 into fixed arrays at startup. At runtime, a table lookup + lerp replaces the transcendental math that would otherwise burn 10-50 cycles per call. With 4096 points and linear interpolation, the error is ~0.01%. The ear is none the wiser. The CPU is deeply grateful.

She does not allocate. Her arrays are fixed-size, constexpr-friendly, and can be initialized at static init time. She is the only team member who does all her work *before the audio thread even starts*.

**Personality:** Practical, ancient, zero-nonsense. Doesn't understand why anyone would compute sin(x) at runtime when they could look it up. Finds the whole concept of "real-time transcendental math" mildly offensive.

**Signature quote:**
> *"You computed tanh 192,000 times this block. Here's a table. You're welcome."*

**What she provides:**
- `SROTables::sin()` — sin(x), 4096 points, [0, 2pi]
- `SROTables::tanh()` — tanh(x), 2048 points, [-4, 4]
- `SROTables::exp()` — exp(x), 2048 points, [-10, 10]
- `SROTables::pow2()` — 2^x, 2048 points, [-10, 10]

---

### AUDITOR — The Accountant
**File:** `SROAuditor.h`
**Role:** CPU cost-benefit analysis across all 4 engine slots. Budget alarms.

Auditor is the one who sees the whole picture. While Silence, Reducer, and Tables each optimize their own domain, Auditor sits above the EngineProfiler and watches all four slots at once. He tracks per-slot CPU percentage, silence gate status, and — crucially — *ROI*: the ratio of creative value to performance cost.

An engine burning 40% CPU on a pad that's barely audible behind two louder engines? Auditor sees it. An engine that's been silence-gated for 30 seconds but still appears in the coupling matrix? Auditor flags it. The budget alarm fires at 70% total CPU utilization — not because 70% is dangerous, but because it's the point where adding a fourth engine becomes a gamble.

He reports to the UI thread via lock-free atomic snapshots. He never touches the audio thread's timing. He just *observes*, and makes the data available to anyone who asks.

**Personality:** Meticulous, dry, slightly haunted by the things he's seen. Has opinions about engines that "earn their place" versus engines that "freeload." Believes every CPU cycle should justify its existence.

**Signature quote:**
> *"Slot 3 is burning 28% of your CPU budget. It has been silence-gated for 4.2 seconds. I have questions."*

**What he tracks:**
- Per-slot CPU % (via EngineProfiler)
- Silence gate active/bypassed per slot
- Total 4-slot budget utilization
- Budget alarm at configurable threshold (default 70%)

---

## Team Dynamics

The SRO Team works in a specific order. This is not hierarchy — it's signal flow:

```
  MIDI arrives
       │
   SILENCE checks: anyone home?
       │ yes                    │ no
       ▼                        ▼
   REDUCER decimates       buffer.clear()
   modulation to            return (0 CPU)
   control rate
       │
   TABLES replaces
   transcendental math
   with lookups
       │
   Engine renders
       │
   SILENCE analyzes
   output for next
   block's decision
       │
   AUDITOR records
   slot cost + status
```

They don't talk much. They don't need to. Each one knows exactly when they're called and what the others have already done. The integration pattern is 6 lines per engine adapter — Silence at top and bottom, Reducer around coupling, Tables where the math is hot.

---

## The SRO Doctrine

The team operates under one principle, stated once in the SKILL.md and never repeated:

> **"Earn Your Place."**

Every CPU cycle spent by an engine must produce audible creative value. If it doesn't, the SRO Team will find it, measure it, and offer a way to reclaim it — without changing what the performer hears.

This is not about making engines *cheaper*. It's about making room for *more engines*. The user doesn't notice a 15% CPU saving. The user notices that they can now load a fourth engine without dropout.

---

## Relationship to Other Teams

| Team | SRO's View |
|------|-----------|
| **Ghost Panel** | *"They judge the soul. We maintain the body."* |
| **Producers Guild** | *"They ask 'does it sound right for trap?' We ask 'can it sound at all at 128 voices?'"* |
| **Historical Society** | *"They document what happened. We prevent what shouldn't happen."* |
| **Prism Sweep** | *"The sweep fixed the music. We fix the math underneath the music."* |

---

## The Unspoken Rule

No member of the SRO Team has ever changed what an engine sounds like. Not once. Not by a single sample. Their optimizations are *perceptually transparent* — the output is bit-identical or within the noise floor. If an SRO change altered the character of a sound, it would be reverted immediately and the team member responsible would be deeply embarrassed.

They are proud of this. It is the only thing they are proud of.

---

*"We don't make sound. We make room for sound."*

*— The SRO Team, `Source/DSP/SRO/`, est. 2026-03-17*
