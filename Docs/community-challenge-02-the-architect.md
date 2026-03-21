# Community Challenge #2 — The Architect

**Launch date:** May 25, 2026
**Results announced:** June 15-21, 2026 (Week 13)
**Prize:** Winning preset added to the factory library (next release)

---

## The Challenge

Build a 3-engine patch where each macro controls a different engine's character.

The 4 macros in XOmnibus — CHARACTER, MOVEMENT, COUPLING, SPACE — span all loaded engines simultaneously. That's the design: one gesture, the whole system responds. Most people use them as a global tone shaper. This challenge is about using them as an architectural tool instead.

The idea is specific. You're designing a patch where each of your 3 engines is *primarily shaped* by a different macro. Turning CHARACTER changes Engine 1's core tone. Turning MOVEMENT changes Engine 2's modulation state. Turning COUPLING modulates the routing intensity between Engine 2 and Engine 3. SPACE shapes the reverb and width of Engine 3, or the whole patch, but in a way that feels like it belongs to one engine's voice. The result is a patch that plays like a 3-dimensional instrument: one hand on the macros, three engines moving in distinct directions.

The challenge is named "The Architect" because an architect doesn't just design rooms — they design the relationships between rooms. The ceiling height in one room affects how you hear the next. The corridor between them is part of the plan. Here, your macro assignments are the architectural plan. We want to hear whether the plan is intentional, whether each room has its own voice, and whether the whole structure holds together as one building.

The coupling route is load-bearing. Without it, you have 3 independent sounds. With it, you have a system.

---

## Rules

**Engine count:** 3 engines loaded (Slots 1, 2, and 3). Slot 4 must be empty.

**Coupling requirement:** At least 1 active coupling route between any two of the three engines. Type is your choice — all 14 are legal. The coupling route must be audible, not decorative.

**Macro coverage:** Each of the 4 macros must produce a perceptibly different sonic result across its full range. At least 3 of the 4 macros should feel like they "own" a specific engine's character — turning that macro should change one engine more than the others.

**Submission format:** `.xometa` file + a paragraph (2-5 sentences) describing which macro controls which engine, why you assigned it that way, and what the coupling route is doing.

**One submission per person.** You can update your submission until the deadline.

---

## What We're Judging

**Architectural clarity.** Can you hear which macro controls which engine? The assignment should be intentional and perceptible — not just a parameter assignment, but a design decision. If someone picks up the patch and sweeps CHARACTER, they should hear Engine 1 change character. Not the whole patch shifting in the same direction.

**Cohesion.** Do the 3 engines feel like one instrument, or 3 separate sounds playing at the same time? The coupling route is the connective tissue. Without it working, you have a layer stack. With it working, you have an instrument.

**Macro design.** Is the full range of each macro musical? A macro that only sounds interesting at one extreme is lazy design. A macro that sweeps from one musical state to another, with useful territory throughout the range, is good design. We're judging the whole arc, not just the endpoints.

**Surprise.** Did you find a 3-engine combination that sounds like something new? The fleet has 46 engines. Most people will reach for familiar pairs. The patches that win challenges are the ones that found a combination nobody saw coming.

---

## Discord Post Copy (paste this into #challenge-the-architect)

---

🏗️ **Community Challenge #2 — The Architect**

Three engines. One coupling route. Each macro controls a different engine's character.

The macro system in XOmnibus is usually treated as a global tone control. CHARACTER makes everything brighter. SPACE adds reverb to the whole patch. That's valid — but it's not what the macros are for. The macros are a composition tool. This challenge is about using them as one.

**Your challenge:**

Load exactly 3 engines. Wire at least 2 of them together with a coupling route. Assign your macro mappings so that each macro primarily shapes one engine's character — not everything at once. The listener should be able to hear which macro controls which engine.

**Rules:**
- 3 engines only (Slot 4 empty)
- 1+ active coupling route (must be audible)
- At least 3 of the 4 macros should feel like they "own" a specific engine's character
- Share your `.xometa` file + 2-5 sentences on your routing and assignment choices

**Prize:** The winning patch gets added to the factory library in the next release, credited to you in the preset description.

This challenge runs for three weeks. Drop your entries in #challenge-the-architect.

Some starting points if you're stuck: the Coupling Cookbook post covers strong OBRIX pairs if you need a coupling anchor. The CHARACTER macro usually maps naturally to filter cutoff + harmonic content. MOVEMENT maps well to LFO rates, envelope times, and modulation depth. COUPLING is most interesting when it maps to coupling amount directly — not a parameter inside one engine, but the route strength itself. SPACE for spatial width and reverb depth is the natural fourth assignment.

The coupling amount sweet spot is almost always 0.08–0.20. Three-engine patches tend to benefit from restraint — one strong coupling route beats three weak ones.

xo-ox.org — free download, MIT license.

---

## Judging Rubric (internal)

| Criterion | Weight | Notes |
|-----------|--------|-------|
| Architectural clarity | 35% | Can you clearly hear which macro controls which engine? Is the assignment perceptible as a design choice? |
| Cohesion | 30% | Do the 3 engines function as one instrument? Does the coupling route provide connective tissue, not just spectral layering? |
| Macro design | 20% | Is the full range of each macro musical? Are there useful sounds throughout the sweep, not just at the endpoints? |
| Surprise | 15% | Did the designer find a 3-engine combination that produces a sound neither engine nor any obvious pair could produce alone? |

Minimum score to win: 7/10 overall. If no submission reaches 7/10, announce honorable mentions and carry the prize to Challenge #3.

---

## Results Announcement Template (Week 13)

---

🏆 **Challenge #2 Results — The Architect**

[X] submissions came in. Here's what we heard.

**Winner: [PRESET NAME] by [USERNAME]**

[2-3 sentences describing which engines they chose, how they assigned the macros, and what made the architectural clarity land. Name the coupling type if it's notable.]

Their patch is in the factory library as of today's update. Download at xo-ox.org.

**Honorable mentions:**

[2-3 honorable mentions with brief descriptions — focus on what each designer got right: a surprising engine combination, a macro assignment nobody expected, a coupling route that genuinely unified the patch]

Thank you to everyone who built something. The macro assignment approaches we saw ranged from [observation about common approaches] to [observation about surprising outlier]. [One insight about what the challenge revealed — what the macro system does when used as a compositional tool rather than a tone control.]

Challenge #3 — "The Reef" — starts [WEEK 13 END DATE]. This one is about the Aquatic FX Suite as a load-bearing sonic element. More soon.

---

## Related

- Field Guide Post: "The Coupling System Explained" — `site/guide-coupling.html`
- Field Guide Post: "Coupling Cookbook: OBRIX Pairs" — `site/guide-coupling-obrix-pairs.html`
- Challenge #1: "The Corridor" — `Docs/community-challenge-01-the-corridor.md`
- 6-Month Release Calendar: `Docs/6-month-release-calendar-2026.md`
- Community Activation Plan: `Docs/community-activation-2026-03-20.md`
- Tutorial: "Playing the Space Between" — `Docs/tutorials/playing-the-space-between.md`
