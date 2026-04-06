// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_graphics/juce_graphics.h>
#include <cmath>
#include "../GalleryColors.h"

namespace xoceanus
{

//==============================================================================
// CreatureRenderer — procedural per-engine creature sprites for CompactEngineTile
//
// Draws a distinctive creature shape based on engine archetype.
// All creatures:
//   • Fit within the supplied bounds rectangle
//   • Use the accent colour
//   • React to breathScale (1.0 ± 0.1 typical range)
//   • React to couplingLean (-1 to +1) for directional lean
//   • Drawn with ~5-15 Graphics calls (simple, fast, no allocation)
//
// Engine archetypes:
//   OBRIX       — coral branch (3-4 vertical lines + tip circles, breathe = sway)
//   OPERA       — singing fish (oval body + open-mouth arc, breathe = mouth opens)
//   OXYTOCIN    — jellyfish (dome arc + 4 tentacles, breathe = tentacle wave)
//   ONSET       — pistol shrimp (rect body + oversized claw circle, breathe = claw)
//   OUROBOROS   — serpent eating its own tail (thick arc + triangle head)
//   ORGANISM    — cellular automata 4x4 grid (cells pulse in/out)
//   OWARE       — mancala seed pod (oval + 6 seed circles + tendrils)
//   ORBITAL     — rotating ring system (central dot + 2 elliptical rings + moons)
//   OBSIDIAN    — dark crystal formation (3 overlapping hexagons, faceted)
//   ODYSSEY     — explorer fish with compass-arrow dorsal fin
//   OBLONG      — elongated blob (stretched ellipse + 3 bottom tentacles + two eyes)
//   OPAL        — iridescent shell creature (half-dome + concentric ridges + body)
//   OVERWORLD   — pixelated retro fish (8x8 pixel grid approach)
//   OXBOW       — river loop serpent (S-curve body + head circle + eye)
//   OCELOT      — spotted predator fish (tapered body + spots + forked tail)
//   OWLFISH     — deep-sea owl-faced fish (large round eyes + beak + tiny fins)
//   ORCA        — stylized killer whale (body + eye patch + dorsal fin + fluked tail)
//   OSTINATO    — rhythmic pulsing anemone (base + 10 sine tentacles)
//   OBESE       — round pufferfish (circle + stubby fins + spine dots)
//   ODDFELIX    — neon tetra with glowing stripe (fish body + horizontal stripe)
//   default     — blob + eye + 2-3 particle dots
//
class CreatureRenderer
{
public:
    static void drawCreature(juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& engineId,
                             const juce::Colour& accent, float breathScale, float couplingLean)
    {
        auto id = engineId.toUpperCase();

        if (id == "OBRIX")
            drawCoral(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OPERA")
            drawSingingFish(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OXYTOCIN")
            drawJellyfish(g, bounds, accent, breathScale, couplingLean);
        else if (id == "ONSET")
            drawPistolShrimp(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OUROBOROS")
            drawOuroboros(g, bounds, accent, breathScale, couplingLean);
        else if (id == "ORGANISM")
            drawOrganism(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OWARE")
            drawOware(g, bounds, accent, breathScale, couplingLean);
        else if (id == "ORBITAL")
            drawOrbital(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OBSIDIAN")
            drawObsidian(g, bounds, accent, breathScale, couplingLean);
        else if (id == "ODYSSEY")
            drawOdyssey(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OBLONG")
            drawOblong(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OPAL")
            drawOpal(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OVERWORLD")
            drawOverworld(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OXBOW")
            drawOxbow(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OCELOT")
            drawOcelot(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OWLFISH")
            drawOwlfish(g, bounds, accent, breathScale, couplingLean);
        else if (id == "ORCA")
            drawOrca(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OSTINATO")
            drawOstinato(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OBESE")
            drawObese(g, bounds, accent, breathScale, couplingLean);
        else if (id == "ODDFELIX")
            drawOddfelix(g, bounds, accent, breathScale, couplingLean);
        else
            drawDefault(g, bounds, accent, breathScale, couplingLean);
    }

private:
    //--------------------------------------------------------------------------
    // OBRIX — Coral branch
    // 3-4 vertical lines with small circles at the tips.
    // breathScale drives a left/right sway offset.
    // couplingLean biases the sway direction.
    static void drawCoral(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent, float breathScale,
                          float couplingLean)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        float h = b.getHeight();
        float w = b.getWidth();

        // Sway: breathScale oscillates around 1.0; map to ±3px lateral offset
        float sway = (breathScale - 1.0f) * 20.0f + couplingLean * 3.0f;

        // Branch positions (relative to centre)
        const float branchXOffsets[] = {-w * 0.28f, -w * 0.08f, w * 0.10f, w * 0.28f};
        const float branchHeights[] = {h * 0.55f, h * 0.70f, h * 0.60f, h * 0.50f};

        g.setColour(accent.withAlpha(0.80f));

        for (int i = 0; i < 4; ++i)
        {
            float bx = cx + branchXOffsets[i] + sway * (0.5f + (float)i * 0.15f);
            float top = cy - branchHeights[i] * 0.5f;
            float bot = cy + branchHeights[i] * 0.5f;

            // Stem — 1.5px wide line
            g.drawLine(bx, bot, bx, top, 1.5f);

            // Tip circle — radius 2.5px
            g.fillEllipse(bx - 2.5f, top - 2.5f, 5.0f, 5.0f);
        }

        // Base connector bar
        g.setColour(accent.withAlpha(0.55f));
        float baseY = cy + h * 0.35f;
        g.drawLine(cx + branchXOffsets[0] + sway * 0.5f, baseY, cx + branchXOffsets[3] + sway * 0.8f, baseY, 1.5f);
    }

    //--------------------------------------------------------------------------
    // OPERA — Singing fish
    // Oval body with an open mouth (arc). breathScale drives mouth aperture.
    // couplingLean tilts the fish horizontally.
    static void drawSingingFish(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                                float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.12f;
        float cy = b.getCentreY();
        float bW = b.getWidth() * 0.65f * breathScale;
        float bH = b.getHeight() * 0.45f;

        // Body — filled ellipse
        g.setColour(accent.withAlpha(0.70f));
        g.fillEllipse(cx - bW * 0.5f, cy - bH * 0.5f, bW, bH);

        // Eye — primary text colour dot, upper-right quadrant of body
        float eyeX = cx + bW * 0.20f;
        float eyeY = cy - bH * 0.18f;
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillEllipse(eyeX - 2.0f, eyeY - 2.0f, 4.0f, 4.0f);

        // Mouth — open arc on right side, aperture driven by breathScale
        // breathScale ~1.0 → small mouth; ~1.1 → wide open
        float mouthAperture = juce::MathConstants<float>::pi * 0.25f * (breathScale - 0.9f) * 8.0f;
        mouthAperture = juce::jlimit(0.05f, juce::MathConstants<float>::pi * 0.45f, mouthAperture);

        float mouthR = bH * 0.22f;
        float mouthCx = cx + bW * 0.38f;
        float mouthCy = cy + bH * 0.08f;

        juce::Path mouth;
        mouth.addCentredArc(mouthCx, mouthCy, mouthR, mouthR * 0.6f, 0.0f, -mouthAperture, mouthAperture, true);
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.70f));
        g.strokePath(mouth, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Tail — simple triangle chevron on left
        float tailTipX = cx - bW * 0.60f;
        float tailBaseX = cx - bW * 0.38f;
        g.setColour(accent.withAlpha(0.60f));
        juce::Path tail;
        tail.startNewSubPath(tailBaseX, cy - bH * 0.25f);
        tail.lineTo(tailTipX, cy);
        tail.lineTo(tailBaseX, cy + bH * 0.25f);
        g.strokePath(tail, juce::PathStrokeType(1.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
    }

    //--------------------------------------------------------------------------
    // OXYTOCIN — Jellyfish
    // Dome (arc) with 4 trailing tentacles (vertical sine curves).
    // breathScale drives tentacle wave amplitude.
    // couplingLean tilts the dome slightly.
    static void drawJellyfish(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                              float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.10f;
        float cy = b.getCentreY();
        float domeR = b.getWidth() * 0.34f * breathScale;
        float domeY = cy - b.getHeight() * 0.05f;

        // Bell / dome — filled half-ellipse
        juce::Path dome;
        dome.addCentredArc(cx, domeY, domeR, domeR * 0.65f, 0.0f, juce::MathConstants<float>::pi,
                           juce::MathConstants<float>::twoPi, true);
        dome.closeSubPath();
        g.setColour(accent.withAlpha(0.55f));
        g.fillPath(dome);

        // Dome outline
        g.setColour(accent.withAlpha(0.85f));
        g.strokePath(dome, juce::PathStrokeType(1.2f));

        // 4 tentacles — vertical sine curves hanging from dome base
        const float tentacleXOffsets[] = {-domeR * 0.6f, -domeR * 0.2f, domeR * 0.2f, domeR * 0.6f};
        float tentacleLen = b.getHeight() * 0.38f;
        float waveAmp = 3.0f * breathScale; // breathing drives wave width

        g.setColour(accent.withAlpha(0.65f));
        for (int t = 0; t < 4; ++t)
        {
            float tx = cx + tentacleXOffsets[t];
            float startY = domeY + domeR * 0.55f; // base of dome

            // Phase offset per tentacle so they wave out of phase
            float phaseOffset = (float)t * juce::MathConstants<float>::halfPi;

            juce::Path tentacle;
            const int kSegments = 6;
            tentacle.startNewSubPath(tx, startY);
            for (int s = 1; s <= kSegments; ++s)
            {
                float frac = (float)s / (float)kSegments;
                float segY = startY + frac * tentacleLen;
                float segX = tx + waveAmp * std::sin(frac * juce::MathConstants<float>::twoPi + phaseOffset);
                tentacle.lineTo(segX, segY);
            }
            g.strokePath(tentacle,
                         juce::PathStrokeType(1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    //--------------------------------------------------------------------------
    // ONSET — Pistol shrimp
    // Rectangular body with one oversized claw (large circle).
    // breathScale drives claw open/close (radius pulses).
    // couplingLean determines which side the large claw is on.
    static void drawPistolShrimp(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                                 float breathScale, float couplingLean)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        float bodyW = b.getWidth() * 0.42f;
        float bodyH = b.getHeight() * 0.32f;

        // Body — rounded rectangle
        g.setColour(accent.withAlpha(0.70f));
        g.fillRoundedRectangle(cx - bodyW * 0.5f, cy - bodyH * 0.5f, bodyW, bodyH, bodyH * 0.3f);

        // Small eye on body
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillEllipse(cx + bodyW * 0.28f, cy - bodyH * 0.15f, 3.0f, 3.0f);

        // Claw side depends on couplingLean
        float clawSide = (couplingLean >= 0.0f) ? 1.0f : -1.0f;
        float clawCx = cx + clawSide * (bodyW * 0.5f + b.getWidth() * 0.18f);
        float clawCy = cy - b.getHeight() * 0.08f;
        float clawR = b.getWidth() * 0.18f * breathScale; // pulse on breathScale

        // Oversized claw — hollow circle (the "snap" chamber)
        g.setColour(accent.withAlpha(0.75f));
        g.drawEllipse(clawCx - clawR, clawCy - clawR, clawR * 2.0f, clawR * 2.0f, 2.0f);

        // Inner accent dot — smaller filled circle (pressure bubble)
        float innerR = clawR * 0.35f;
        g.setColour(accent.withAlpha(0.90f));
        g.fillEllipse(clawCx - innerR, clawCy - innerR, innerR * 2.0f, innerR * 2.0f);

        // Connector arm from body to claw
        g.setColour(accent.withAlpha(0.60f));
        float armStartX = cx + clawSide * bodyW * 0.5f;
        float armEndX = clawCx - clawSide * clawR;
        g.drawLine(armStartX, cy, armEndX, clawCy, 1.5f);

        // Small walking legs — 3 tiny lines below body
        g.setColour(accent.withAlpha(0.45f));
        for (int l = 0; l < 3; ++l)
        {
            float legX = cx - bodyW * 0.25f + (float)l * bodyW * 0.25f;
            float legTopY = cy + bodyH * 0.5f;
            float legBotY = legTopY + b.getHeight() * 0.22f;
            g.drawLine(legX, legTopY, legX + (l - 1) * 2.0f, legBotY, 1.0f);
        }
    }

    //--------------------------------------------------------------------------
    // OUROBOROS — Serpent eating its own tail
    // 270-degree thick arc with a triangular head at one end biting the tail.
    // breathScale rotates the arc slightly.
    // couplingLean offsets the whole body horizontally.
    static void drawOuroboros(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                              float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float r = b.getWidth() * 0.30f;

        // Arc rotation driven by breathScale — subtle rotation effect
        float rotOffset = (breathScale - 1.0f) * 0.8f;

        // 270-degree arc (leaving a gap at the bottom-right where head meets tail)
        float arcStartAngle = juce::MathConstants<float>::halfPi * 0.5f + rotOffset; // ~45 deg
        float arcEndAngle   = arcStartAngle + juce::MathConstants<float>::twoPi * 0.75f; // 270 deg sweep

        juce::Path body;
        body.addCentredArc(cx, cy, r, r, 0.0f, arcStartAngle, arcEndAngle, true);
        g.setColour(accent.withAlpha(0.80f));
        g.strokePath(body, juce::PathStrokeType(4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Scale pattern — dashed overlay on same arc at lower alpha
        juce::Path scaleArc;
        scaleArc.addCentredArc(cx, cy, r, r, 0.0f, arcStartAngle, arcEndAngle, true);
        juce::PathStrokeType scaleStroke(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt);
        float dashPattern[] = {3.0f, 4.0f};
        scaleStroke.createDashedStroke(scaleArc, scaleArc, dashPattern, 2);
        g.setColour(accent.withAlpha(0.35f));
        g.fillPath(scaleArc);

        // Head — triangle at the end of the arc (biting position)
        float headAngle = arcEndAngle;
        float hx = cx + r * std::cos(headAngle);
        float hy = cy + r * std::sin(headAngle);

        // Triangle pointing toward tail (toward arcStartAngle)
        float tailAngle = arcStartAngle;
        float tx = cx + r * std::cos(tailAngle);
        float ty = cy + r * std::sin(tailAngle);
        float headSize = b.getWidth() * 0.09f;

        // Direction vector from head toward centre of arc gap
        float dirX = tx - hx;
        float dirY = ty - hy;
        float dirLen = std::sqrt(dirX * dirX + dirY * dirY);
        if (dirLen > 0.001f) { dirX /= dirLen; dirY /= dirLen; }

        juce::Path head;
        head.startNewSubPath(hx + dirX * headSize, hy + dirY * headSize); // tip (biting point)
        head.lineTo(hx - dirY * headSize * 0.5f, hy + dirX * headSize * 0.5f);
        head.lineTo(hx + dirY * headSize * 0.5f, hy - dirX * headSize * 0.5f);
        head.closeSubPath();
        g.setColour(accent.withAlpha(0.95f));
        g.fillPath(head);

        // Tail tip — small dot at arcStartAngle
        g.setColour(accent.withAlpha(0.65f));
        g.fillEllipse(tx - 2.5f, ty - 2.5f, 5.0f, 5.0f);
    }

    //--------------------------------------------------------------------------
    // ORGANISM — Cellular automata grid creature
    // 4x4 grid of small circles, some filled (alive) based on accent hue.
    // breathScale pulses the alive cells in/out.
    // couplingLean shifts which cells are "alive" (offset the pattern).
    static void drawOrganism(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        float gridW = b.getWidth() * 0.72f;
        float cellSize = gridW / 4.0f;
        float cellR = cellSize * 0.32f * breathScale;

        float startX = cx - gridW * 0.5f + cellSize * 0.5f;
        float startY = cy - gridW * 0.5f + cellSize * 0.5f;

        // Alive pattern: fixed checkerboard-ish pattern + couplingLean offset
        // Use a bitmask so the pattern is visually interesting
        const int aliveMask = 0b0110100110010110; // 16 bits for 4x4 grid
        int leanShift = (int)(couplingLean * 2.0f + 2.0f); // 0-4

        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                int idx = (row * 4 + col + leanShift) & 15;
                bool alive = (aliveMask >> idx) & 1;

                float px = startX + col * cellSize;
                float py = startY + row * cellSize;

                if (alive)
                {
                    g.setColour(accent.withAlpha(0.85f));
                    g.fillEllipse(px - cellR, py - cellR, cellR * 2.0f, cellR * 2.0f);
                }
                else
                {
                    // Dead cell — faint outline only
                    float deadR = cellSize * 0.22f;
                    g.setColour(accent.withAlpha(0.22f));
                    g.drawEllipse(px - deadR, py - deadR, deadR * 2.0f, deadR * 2.0f, 0.8f);
                }
            }
        }
    }

    //--------------------------------------------------------------------------
    // OWARE — Mancala seed pod
    // Oval body with 6 small seed circles in two rows of 3.
    // Two curving tendrils from each end.
    // breathScale shimmers the seeds (pulse their size).
    // couplingLean tilts the pod.
    static void drawOware(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                          float breathScale, float couplingLean)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        float podW = b.getWidth() * 0.72f;
        float podH = b.getHeight() * 0.38f;

        // Lean the pod by offsetting cy slightly
        float leanY = couplingLean * b.getHeight() * 0.08f;

        // Pod body — filled ellipse
        g.setColour(accent.withAlpha(0.65f));
        g.fillEllipse(cx - podW * 0.5f, cy - podH * 0.5f + leanY, podW, podH);

        // Pod outline
        g.setColour(accent.withAlpha(0.90f));
        g.drawEllipse(cx - podW * 0.5f, cy - podH * 0.5f + leanY, podW, podH, 1.2f);

        // 6 seeds in 2 rows of 3
        float seedR = podH * 0.18f * breathScale;
        float seedSpacingX = podW * 0.22f;
        float rowOffsetY = podH * 0.18f;

        for (int row = 0; row < 2; ++row)
        {
            float seedY = cy + leanY + (row == 0 ? -rowOffsetY : rowOffsetY);
            for (int col = 0; col < 3; ++col)
            {
                float seedX = cx + (col - 1) * seedSpacingX;
                g.setColour(accent.brighter(0.4f).withAlpha(0.85f));
                g.fillEllipse(seedX - seedR, seedY - seedR, seedR * 2.0f, seedR * 2.0f);
            }
        }

        // Left tendril (curving arc from left end of pod)
        float tendrilEndX = cx - podW * 0.5f;
        juce::Path leftTendril;
        leftTendril.startNewSubPath(tendrilEndX, cy + leanY);
        leftTendril.cubicTo(tendrilEndX - podW * 0.18f, cy + leanY - podH * 0.5f,
                            tendrilEndX - podW * 0.28f, cy + leanY + podH * 0.4f,
                            tendrilEndX - podW * 0.22f, cy + leanY + podH * 0.65f);
        g.setColour(accent.withAlpha(0.55f));
        g.strokePath(leftTendril, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Right tendril
        float rightEndX = cx + podW * 0.5f;
        juce::Path rightTendril;
        rightTendril.startNewSubPath(rightEndX, cy + leanY);
        rightTendril.cubicTo(rightEndX + podW * 0.18f, cy + leanY - podH * 0.5f,
                             rightEndX + podW * 0.28f, cy + leanY + podH * 0.4f,
                             rightEndX + podW * 0.22f, cy + leanY + podH * 0.65f);
        g.strokePath(rightTendril, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    //--------------------------------------------------------------------------
    // ORBITAL — Rotating ring system
    // Central dot with 2 concentric elliptical rings at different tilts.
    // Small moon dots on each ring.
    // breathScale rotates the rings (phase advance).
    // couplingLean tilts the ring plane.
    static void drawOrbital(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        float baseR = b.getWidth() * 0.28f;

        // Central dot
        float coreDot = b.getWidth() * 0.07f;
        g.setColour(accent.withAlpha(0.95f));
        g.fillEllipse(cx - coreDot, cy - coreDot, coreDot * 2.0f, coreDot * 2.0f);

        // Ring 1 — slightly tilted ellipse (major axis horizontal-ish)
        float r1X = baseR * 0.95f;
        float r1Y = baseR * 0.35f + couplingLean * baseR * 0.12f;
        g.setColour(accent.withAlpha(0.60f));
        g.drawEllipse(cx - r1X, cy - r1Y, r1X * 2.0f, r1Y * 2.0f, 1.2f);

        // Moon on ring 1 — position driven by breathScale as rotation angle
        float moonAngle1 = breathScale * juce::MathConstants<float>::twoPi * 2.0f;
        float moon1X = cx + r1X * std::cos(moonAngle1);
        float moon1Y = cy + r1Y * std::sin(moonAngle1);
        g.setColour(accent.withAlpha(0.90f));
        g.fillEllipse(moon1X - 2.5f, moon1Y - 2.5f, 5.0f, 5.0f);

        // Ring 2 — tighter, more vertical tilt
        float r2X = baseR * 0.58f;
        float r2Y = baseR * 0.88f - couplingLean * baseR * 0.10f;
        g.setColour(accent.withAlpha(0.45f));
        g.drawEllipse(cx - r2X, cy - r2Y, r2X * 2.0f, r2Y * 2.0f, 1.0f);

        // Moon on ring 2 — counter-rotation
        float moonAngle2 = -breathScale * juce::MathConstants<float>::twoPi * 3.0f;
        float moon2X = cx + r2X * std::cos(moonAngle2);
        float moon2Y = cy + r2Y * std::sin(moonAngle2);
        g.setColour(accent.withAlpha(0.80f));
        g.fillEllipse(moon2X - 2.0f, moon2Y - 2.0f, 4.0f, 4.0f);
    }

    //--------------------------------------------------------------------------
    // OBSIDIAN — Dark crystal formation
    // 3 overlapping irregular hexagons at different angles, sharp faceted look.
    // breathScale shimmers the alpha of each facet.
    // couplingLean rotates the cluster.
    static void drawObsidian(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        float crystalR = b.getWidth() * 0.26f;

        float baseRotation = couplingLean * juce::MathConstants<float>::pi * 0.25f;
        float shimmer = breathScale; // used to modulate alpha

        const float crystalAngles[] = {0.0f, juce::MathConstants<float>::pi * 0.38f, -juce::MathConstants<float>::pi * 0.55f};
        const float crystalScales[] = {1.0f, 0.80f, 0.68f};
        const float alphas[] = {0.55f, 0.45f, 0.35f};

        for (int c = 0; c < 3; ++c)
        {
            float angle = baseRotation + crystalAngles[c];
            float cr = crystalR * crystalScales[c];
            // Irregular hexagon — 6 vertices with slight irregularity
            juce::Path hex;
            const int sides = 6;
            const float irregularity[] = {1.0f, 0.85f, 1.10f, 0.92f, 1.05f, 0.88f};
            for (int v = 0; v < sides; ++v)
            {
                float vAngle = angle + (float)v * juce::MathConstants<float>::twoPi / (float)sides;
                float vR = cr * irregularity[v];
                float vx = cx + vR * std::cos(vAngle);
                float vy = cy + vR * std::sin(vAngle);
                if (v == 0)
                    hex.startNewSubPath(vx, vy);
                else
                    hex.lineTo(vx, vy);
            }
            hex.closeSubPath();

            // Fill with varying alpha — shimmer modulates
            float shimmerAlpha = alphas[c] * (0.7f + shimmer * 0.3f);
            g.setColour(accent.withAlpha(shimmerAlpha));
            g.fillPath(hex);

            // Sharp outline
            g.setColour(accent.withAlpha(shimmerAlpha + 0.20f));
            g.strokePath(hex, juce::PathStrokeType(1.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
        }
    }

    //--------------------------------------------------------------------------
    // ODYSSEY — Explorer fish with compass-arrow dorsal fin
    // Ellipse body, triangle dorsal fin pointing forward like a compass arrow,
    // tail fin, and eye. breathScale drives gentle swim sway.
    // couplingLean leans the whole fish.
    static void drawOdyssey(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.10f;
        float cy = b.getCentreY() + (breathScale - 1.0f) * b.getHeight() * 0.12f; // swim sway

        float bodyW = b.getWidth() * 0.60f;
        float bodyH = b.getHeight() * 0.35f;

        // Body — filled ellipse
        g.setColour(accent.withAlpha(0.72f));
        g.fillEllipse(cx - bodyW * 0.5f, cy - bodyH * 0.5f, bodyW, bodyH);

        // Eye — upper-right of body
        float eyeX = cx + bodyW * 0.22f;
        float eyeY = cy - bodyH * 0.15f;
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillEllipse(eyeX - 2.0f, eyeY - 2.0f, 4.0f, 4.0f);

        // Compass-arrow dorsal fin — triangle pointing right (forward)
        float finBaseX = cx;
        float finTopX  = cx + bodyW * 0.32f; // points forward
        float finTopY  = cy - bodyH * 0.80f; // rises above body
        juce::Path dorsalFin;
        dorsalFin.startNewSubPath(finBaseX - bodyW * 0.12f, cy - bodyH * 0.48f);
        dorsalFin.lineTo(finTopX, finTopY);
        dorsalFin.lineTo(finBaseX + bodyW * 0.18f, cy - bodyH * 0.48f);
        dorsalFin.closeSubPath();
        g.setColour(accent.withAlpha(0.88f));
        g.fillPath(dorsalFin);

        // Tail — forked on the left
        float tailTipX = cx - bodyW * 0.62f;
        float tailBaseX = cx - bodyW * 0.40f;
        g.setColour(accent.withAlpha(0.60f));
        juce::Path tail;
        tail.startNewSubPath(tailBaseX, cy);
        tail.lineTo(tailTipX, cy - bodyH * 0.32f);
        tail.startNewSubPath(tailBaseX, cy);
        tail.lineTo(tailTipX, cy + bodyH * 0.32f);
        g.strokePath(tail, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    //--------------------------------------------------------------------------
    // OBLONG — Elongated curious blob
    // Stretched ellipse body with 3 tentacles dangling from bottom.
    // Two eyes of different sizes (curious expression).
    // breathScale waves the tentacles. couplingLean leans the body.
    static void drawOblong(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY() - b.getHeight() * 0.08f;
        float bodyW = b.getWidth() * 0.72f;
        float bodyH = b.getHeight() * 0.32f;

        // Lean
        float leanX = couplingLean * b.getWidth() * 0.10f;

        // Body — wide stretched ellipse
        g.setColour(accent.withAlpha(0.68f));
        g.fillEllipse(cx - bodyW * 0.5f + leanX, cy - bodyH * 0.5f, bodyW, bodyH);

        // Two eyes (different sizes for curious expression)
        // Left eye — larger
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.92f));
        g.fillEllipse(cx - bodyW * 0.20f + leanX - 3.5f, cy - bodyH * 0.15f - 3.5f, 7.0f, 7.0f);
        // Right eye — smaller
        g.fillEllipse(cx + bodyW * 0.16f + leanX - 2.0f, cy - bodyH * 0.10f - 2.0f, 4.0f, 4.0f);

        // 3 tentacles dangling from the bottom
        float tentBaseY = cy + bodyH * 0.50f;
        float tentLen = b.getHeight() * 0.32f;
        float waveAmp = 3.5f * (breathScale - 0.95f) * 20.0f; // wave from breathScale
        waveAmp = juce::jlimit(-3.0f, 3.0f, waveAmp);

        g.setColour(accent.withAlpha(0.65f));
        for (int t = 0; t < 3; ++t)
        {
            float tx = cx + (t - 1) * bodyW * 0.28f + leanX;
            float phaseShift = (float)t * juce::MathConstants<float>::pi * 0.6f;

            juce::Path tentacle;
            tentacle.startNewSubPath(tx, tentBaseY);
            const int segs = 5;
            for (int s = 1; s <= segs; ++s)
            {
                float frac = (float)s / (float)segs;
                float segY = tentBaseY + frac * tentLen;
                float segX = tx + waveAmp * std::sin(frac * juce::MathConstants<float>::pi + phaseShift);
                tentacle.lineTo(segX, segY);
            }
            g.strokePath(tentacle, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    //--------------------------------------------------------------------------
    // OPAL — Iridescent shell creature
    // Half-dome shell with concentric ridges. Small body peeking from underneath.
    // Multiple thin arcs at offset hues for rainbow shimmer.
    // breathScale opens/closes the shell slightly. couplingLean tilts the shell.
    static void drawOpal(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                         float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.08f;
        float cy = b.getCentreY();
        float shellW = b.getWidth() * 0.62f;
        float shellH = b.getHeight() * 0.38f * breathScale; // opens/closes with breath

        float shellTopY = cy - shellH * 0.1f;

        // Shell body — filled half ellipse (upper arc)
        juce::Path shell;
        shell.addCentredArc(cx, shellTopY, shellW * 0.5f, shellH, 0.0f,
                            juce::MathConstants<float>::pi, juce::MathConstants<float>::twoPi, true);
        shell.closeSubPath();
        g.setColour(accent.withAlpha(0.65f));
        g.fillPath(shell);

        // Concentric ridge arcs — 4 ridges at decreasing radii
        const float ridgeFracs[] = {0.85f, 0.68f, 0.50f, 0.32f};
        const float ridgeHueShifts[] = {0.0f, 0.08f, 0.16f, 0.24f}; // rainbow shimmer

        for (int r = 0; r < 4; ++r)
        {
            float rw = shellW * 0.5f * ridgeFracs[r];
            float rh = shellH * ridgeFracs[r];
            // Slight hue rotation for iridescent look
            juce::Colour ridgeCol = accent.withRotatedHue(ridgeHueShifts[r]).withAlpha(0.50f);
            g.setColour(ridgeCol);

            juce::Path ridge;
            ridge.addCentredArc(cx, shellTopY, rw, rh, 0.0f,
                                juce::MathConstants<float>::pi, juce::MathConstants<float>::twoPi, true);
            g.strokePath(ridge, juce::PathStrokeType(0.9f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Shell outline
        g.setColour(accent.withAlpha(0.88f));
        g.strokePath(shell, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Small body peeking from underneath shell base
        float bodyW = shellW * 0.35f;
        float bodyH = b.getHeight() * 0.18f;
        float bodyY = shellTopY + shellH * 0.05f; // just under shell rim
        g.setColour(accent.brighter(0.3f).withAlpha(0.75f));
        g.fillEllipse(cx - bodyW * 0.5f, bodyY, bodyW, bodyH);

        // Body eye
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.85f));
        g.fillEllipse(cx + bodyW * 0.15f - 1.5f, bodyY + bodyH * 0.25f - 1.5f, 3.0f, 3.0f);
    }

    //--------------------------------------------------------------------------
    // OVERWORLD — Pixelated retro fish
    // Small rectangles composing a blocky pixel-art fish silhouette.
    // breathScale shimmers (pulses alpha). couplingLean mirrors the fish.
    static void drawOverworld(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                              float breathScale, float couplingLean)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        float pixelSize = b.getWidth() * 0.095f;

        // 8x8 pixel fish shape — 1 = body, 0 = empty
        // Row 0 = top
        const int pixels[8][8] = {
            {0, 0, 0, 1, 0, 0, 0, 0},
            {0, 0, 1, 1, 1, 0, 0, 0},
            {1, 0, 1, 1, 1, 1, 1, 0},
            {1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1},
            {1, 0, 1, 1, 1, 1, 1, 0},
            {0, 0, 1, 1, 1, 0, 0, 0},
            {0, 0, 0, 1, 0, 0, 0, 0}
        };

        float gridW = pixelSize * 8.0f;
        float startX = cx - gridW * 0.5f;
        float startY = cy - gridW * 0.5f;

        float shimmerAlpha = 0.60f + (breathScale - 1.0f) * 1.5f;
        shimmerAlpha = juce::jlimit(0.45f, 0.92f, shimmerAlpha);

        for (int row = 0; row < 8; ++row)
        {
            for (int col = 0; col < 8; ++col)
            {
                // Mirror with couplingLean
                int srcCol = (couplingLean < 0.0f) ? (7 - col) : col;
                if (pixels[row][srcCol] == 1)
                {
                    float px = startX + col * pixelSize;
                    float py = startY + row * pixelSize;
                    g.setColour(accent.withAlpha(shimmerAlpha));
                    g.fillRect(px, py, pixelSize - 0.5f, pixelSize - 0.5f);
                }
            }
        }

        // Eye pixel — slightly brighter
        int eyeCol = (couplingLean < 0.0f) ? 2 : 5;
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillRect(startX + eyeCol * pixelSize, startY + 3 * pixelSize, pixelSize - 0.5f, pixelSize - 0.5f);
    }

    //--------------------------------------------------------------------------
    // OXBOW — River loop serpent
    // S-curve thick path body with head circle + eye at one end.
    // breathScale undulates the S-curve amplitude.
    // couplingLean shifts the lean direction of the S.
    static void drawOxbow(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                          float breathScale, float couplingLean)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        float amplitude = b.getWidth() * 0.22f * breathScale + couplingLean * b.getWidth() * 0.08f;
        float segH = b.getHeight() * 0.22f;

        // S-curve body — 4 control points making an S shape
        juce::Path sCurve;
        float topY    = cy - segH * 1.5f;
        float midTopY = cy - segH * 0.5f;
        float midBotY = cy + segH * 0.5f;
        float botY    = cy + segH * 1.5f;

        sCurve.startNewSubPath(cx, topY);
        sCurve.cubicTo(cx + amplitude, topY + segH * 0.5f,
                       cx - amplitude, midTopY,
                       cx,             midTopY + segH * 0.5f);
        sCurve.cubicTo(cx + amplitude, midBotY,
                       cx - amplitude, botY - segH * 0.5f,
                       cx,             botY);

        g.setColour(accent.withAlpha(0.72f));
        g.strokePath(sCurve, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Head circle at top of S
        float headR = b.getWidth() * 0.10f;
        g.setColour(accent.withAlpha(0.90f));
        g.fillEllipse(cx - headR, topY - headR, headR * 2.0f, headR * 2.0f);

        // Eye on head
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillEllipse(cx + headR * 0.30f - 1.5f, topY - headR * 0.20f - 1.5f, 3.0f, 3.0f);

        // Tail tip — small circle at bottom
        float tailR = b.getWidth() * 0.05f;
        g.setColour(accent.withAlpha(0.60f));
        g.fillEllipse(cx - tailR, botY - tailR, tailR * 2.0f, tailR * 2.0f);
    }

    //--------------------------------------------------------------------------
    // OCELOT — Spotted predator fish
    // Sleek tapered ellipse body, circular spots, pointed snout, forked tail.
    // breathScale drives predatory sway. couplingLean tilts direction.
    static void drawOcelot(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.08f;
        float cy = b.getCentreY() + (breathScale - 1.0f) * b.getHeight() * 0.10f;

        float bodyW = b.getWidth() * 0.68f;
        float bodyH = b.getHeight() * 0.28f;

        // Body — tapered ellipse (slightly narrower at right/snout)
        juce::Path body;
        body.startNewSubPath(cx + bodyW * 0.5f, cy); // snout point
        body.cubicTo(cx + bodyW * 0.30f, cy - bodyH * 0.5f,
                     cx - bodyW * 0.10f, cy - bodyH * 0.52f,
                     cx - bodyW * 0.5f,  cy - bodyH * 0.30f);
        body.cubicTo(cx - bodyW * 0.65f, cy,
                     cx - bodyW * 0.5f,  cy + bodyH * 0.30f,
                     cx + bodyW * 0.5f, cy);
        body.closeSubPath();
        g.setColour(accent.withAlpha(0.70f));
        g.fillPath(body);

        // 5 circular spots on body
        const float spotX[] = {-0.25f, -0.02f, 0.18f, -0.18f, 0.05f};
        const float spotY[] = {-0.08f,  0.06f, -0.06f,  0.10f, -0.14f};
        float spotR = bodyH * 0.14f;
        g.setColour(accent.darker(0.35f).withAlpha(0.70f));
        for (int s = 0; s < 5; ++s)
        {
            float sx = cx + spotX[s] * bodyW;
            float sy = cy + spotY[s] * bodyH;
            g.fillEllipse(sx - spotR, sy - spotR, spotR * 2.0f, spotR * 2.0f);
        }

        // Eye — upper right area of body
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillEllipse(cx + bodyW * 0.28f - 2.0f, cy - bodyH * 0.15f - 2.0f, 4.0f, 4.0f);

        // Forked tail — two prongs from left end
        float tailBaseX = cx - bodyW * 0.48f;
        float tailTipX  = cx - bodyW * 0.72f;
        g.setColour(accent.withAlpha(0.65f));
        juce::Path tail;
        tail.startNewSubPath(tailBaseX, cy);
        tail.lineTo(tailTipX, cy - bodyH * 0.38f);
        tail.startNewSubPath(tailBaseX, cy);
        tail.lineTo(tailTipX, cy + bodyH * 0.38f);
        g.strokePath(tail, juce::PathStrokeType(2.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
    }

    //--------------------------------------------------------------------------
    // OWLFISH — Deep-sea owl-faced fish
    // Round body, two very large owl eyes (40% of face), downward beak, tiny fins.
    // breathScale scales Y of eye circles to simulate slow blinking.
    // couplingLean shifts the whole creature.
    static void drawOwlfish(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.08f;
        float cy = b.getCentreY();
        float bodyR = b.getWidth() * 0.30f;

        // Round body
        g.setColour(accent.withAlpha(0.65f));
        g.fillEllipse(cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);

        // Two large owl eyes — each takes ~20% of body diameter
        float eyeR = bodyR * 0.40f;
        float blinkScaleY = juce::jlimit(0.3f, 1.0f, (breathScale - 0.95f) * 20.0f + 0.75f); // blink

        // Left eye
        float leftEyeCx = cx - bodyR * 0.38f;
        float leftEyeCy = cy - bodyR * 0.10f;
        g.setColour(accent.brighter(0.5f).withAlpha(0.92f));
        g.fillEllipse(leftEyeCx - eyeR, leftEyeCy - eyeR * blinkScaleY, eyeR * 2.0f, eyeR * 2.0f * blinkScaleY);
        // Eye pupil
        g.setColour(juce::Colours::black.withAlpha(0.75f));
        g.fillEllipse(leftEyeCx - eyeR * 0.38f, leftEyeCy - eyeR * blinkScaleY * 0.38f,
                      eyeR * 0.76f, eyeR * blinkScaleY * 0.76f);

        // Right eye
        float rightEyeCx = cx + bodyR * 0.38f;
        float rightEyeCy = cy - bodyR * 0.10f;
        g.setColour(accent.brighter(0.5f).withAlpha(0.92f));
        g.fillEllipse(rightEyeCx - eyeR, rightEyeCy - eyeR * blinkScaleY, eyeR * 2.0f, eyeR * 2.0f * blinkScaleY);
        g.setColour(juce::Colours::black.withAlpha(0.75f));
        g.fillEllipse(rightEyeCx - eyeR * 0.38f, rightEyeCy - eyeR * blinkScaleY * 0.38f,
                      eyeR * 0.76f, eyeR * blinkScaleY * 0.76f);

        // Small downward beak (triangle pointing down)
        float beakCx = cx;
        float beakTopY = cy + bodyR * 0.18f;
        float beakSize = bodyR * 0.22f;
        juce::Path beak;
        beak.startNewSubPath(beakCx - beakSize * 0.5f, beakTopY);
        beak.lineTo(beakCx + beakSize * 0.5f, beakTopY);
        beak.lineTo(beakCx, beakTopY + beakSize);
        beak.closeSubPath();
        g.setColour(accent.darker(0.2f).withAlpha(0.85f));
        g.fillPath(beak);

        // Tiny side fins
        g.setColour(accent.withAlpha(0.50f));
        g.fillEllipse(cx + bodyR * 0.85f - 2.5f, cy - 2.5f, 7.0f, 5.0f);
        g.fillEllipse(cx - bodyR * 0.85f - 4.5f, cy - 2.5f, 7.0f, 5.0f);
    }

    //--------------------------------------------------------------------------
    // ORCA — Stylized killer whale
    // Accent body with a lighter ellipse eye patch on the side.
    // Tall triangular dorsal fin, fluked tail.
    // breathScale drives gentle rise/fall. couplingLean tilts the body.
    static void drawOrca(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                         float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.08f;
        float cy = b.getCentreY() + (breathScale - 1.0f) * b.getHeight() * 0.14f; // rise/fall

        float bodyW = b.getWidth() * 0.68f;
        float bodyH = b.getHeight() * 0.35f;

        // Main body — dark ellipse
        g.setColour(accent.withAlpha(0.85f));
        g.fillEllipse(cx - bodyW * 0.5f, cy - bodyH * 0.5f, bodyW, bodyH);

        // White/light eye patch — distinctive orca marking (upper right side)
        float patchW = bodyW * 0.28f;
        float patchH = bodyH * 0.40f;
        float patchCx = cx + bodyW * 0.22f;
        float patchCy = cy - bodyH * 0.12f;
        g.setColour(accent.brighter(0.8f).withAlpha(0.90f));
        g.fillEllipse(patchCx - patchW * 0.5f, patchCy - patchH * 0.5f, patchW, patchH);

        // Eye — small dark dot within the eye patch
        g.setColour(juce::Colours::black.withAlpha(0.80f));
        g.fillEllipse(patchCx - 1.5f, patchCy - 1.5f, 3.0f, 3.0f);

        // Tall dorsal fin — triangle rising from top of body
        float finBaseX = cx + bodyW * 0.08f;
        float finTopX  = cx + bodyW * 0.18f;
        float finTopY  = cy - bodyH * 0.5f - bodyH * 0.75f; // rises tall above body
        juce::Path dorsalFin;
        dorsalFin.startNewSubPath(finBaseX - bodyW * 0.10f, cy - bodyH * 0.48f);
        dorsalFin.lineTo(finTopX, finTopY);
        dorsalFin.lineTo(finBaseX + bodyW * 0.08f, cy - bodyH * 0.48f);
        dorsalFin.closeSubPath();
        g.setColour(accent.withAlpha(0.90f));
        g.fillPath(dorsalFin);

        // Fluked tail — two lobes on the right
        float tailCx = cx + bodyW * 0.52f;
        float tailCy = cy;
        float flukeW = bodyW * 0.22f;
        float flukeH = bodyH * 0.45f;
        g.setColour(accent.withAlpha(0.75f));
        // Upper fluke
        juce::Path fluke;
        fluke.startNewSubPath(tailCx, tailCy);
        fluke.cubicTo(tailCx + flukeW * 0.5f, tailCy - flukeH * 0.2f,
                      tailCx + flukeW,        tailCy - flukeH,
                      tailCx + flukeW * 0.8f, tailCy - flukeH * 0.5f);
        // Lower fluke
        fluke.startNewSubPath(tailCx, tailCy);
        fluke.cubicTo(tailCx + flukeW * 0.5f, tailCy + flukeH * 0.2f,
                      tailCx + flukeW,        tailCy + flukeH,
                      tailCx + flukeW * 0.8f, tailCy + flukeH * 0.5f);
        g.strokePath(fluke, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    //--------------------------------------------------------------------------
    // OSTINATO — Rhythmic pulsing anemone
    // Rounded rect base with 10 tentacles rising upward, each a sine curve.
    // breathScale drives tentacle pulse (phase shifts rhythmically).
    // couplingLean sways the whole anemone.
    static void drawOstinato(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.10f;
        float cy = b.getCentreY();
        float baseW = b.getWidth() * 0.58f;
        float baseH = b.getHeight() * 0.20f;
        float baseY = cy + b.getHeight() * 0.22f;

        // Base — rounded rectangle
        g.setColour(accent.withAlpha(0.70f));
        g.fillRoundedRectangle(cx - baseW * 0.5f, baseY - baseH * 0.5f, baseW, baseH, baseH * 0.4f);

        // 10 tentacles rising upward from the base
        const int numTentacles = 10;
        float tentacleSpan = baseW * 0.90f;
        float tentacleLen = b.getHeight() * 0.52f;
        float waveAmp = b.getWidth() * 0.055f;

        g.setColour(accent.withAlpha(0.78f));
        for (int t = 0; t < numTentacles; ++t)
        {
            float frac = (float)t / (float)(numTentacles - 1);
            float tx = cx - tentacleSpan * 0.5f + frac * tentacleSpan;
            float topY = baseY - baseH * 0.5f;

            // Phase offset per tentacle — rhythmic pulse from breathScale
            float phase = (float)t * juce::MathConstants<float>::pi * 0.4f
                          + breathScale * juce::MathConstants<float>::twoPi * 3.0f;

            juce::Path tentacle;
            tentacle.startNewSubPath(tx, topY);
            const int segs = 7;
            for (int s = 1; s <= segs; ++s)
            {
                float segFrac = (float)s / (float)segs;
                float segY = topY - segFrac * tentacleLen;
                float segX = tx + waveAmp * std::sin(segFrac * juce::MathConstants<float>::pi * 2.0f + phase);
                tentacle.lineTo(segX, segY);
            }
            g.strokePath(tentacle, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Tentacle tip — small circle
            float tipFrac = 1.0f;
            float tipY = topY - tipFrac * tentacleLen;
            float tipX = tx + waveAmp * std::sin(tipFrac * juce::MathConstants<float>::pi * 2.0f + phase);
            g.fillEllipse(tipX - 1.8f, tipY - 1.8f, 3.6f, 3.6f);
        }
    }

    //--------------------------------------------------------------------------
    // OBESE — Round pufferfish
    // Very round circle body, stubby fins, tiny mouth, spine dots around perimeter.
    // breathScale inflates/deflates the body circle.
    // couplingLean shifts the fish.
    static void drawObese(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                          float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.07f;
        float cy = b.getCentreY();
        float bodyR = b.getWidth() * 0.32f * breathScale; // inflate/deflate

        // Round body
        g.setColour(accent.withAlpha(0.72f));
        g.fillEllipse(cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);

        // Spine dots around perimeter — 12 evenly spaced
        const int numSpines = 12;
        float spineR = 2.0f;
        float spineDist = bodyR * 1.08f;
        g.setColour(accent.darker(0.25f).withAlpha(0.80f));
        for (int s = 0; s < numSpines; ++s)
        {
            float angle = (float)s * juce::MathConstants<float>::twoPi / (float)numSpines;
            float sx = cx + spineDist * std::cos(angle);
            float sy = cy + spineDist * std::sin(angle);
            g.fillEllipse(sx - spineR, sy - spineR, spineR * 2.0f, spineR * 2.0f);
        }

        // Small stubby fins (top and sides)
        g.setColour(accent.withAlpha(0.58f));
        // Top fin
        g.fillEllipse(cx - 5.0f, cy - bodyR - 6.0f, 10.0f, 7.0f);
        // Side fins
        g.fillEllipse(cx + bodyR - 2.0f, cy - 4.0f, 9.0f, 8.0f);
        g.fillEllipse(cx - bodyR - 7.0f, cy - 4.0f, 9.0f, 8.0f);

        // Tiny mouth (small arc at right)
        float mouthCx = cx + bodyR * 0.72f;
        float mouthCy = cy + bodyR * 0.05f;
        juce::Path mouth;
        mouth.addCentredArc(mouthCx, mouthCy, 3.0f, 2.0f, 0.0f,
                            -juce::MathConstants<float>::pi * 0.4f,
                             juce::MathConstants<float>::pi * 0.4f, true);
        g.setColour(accent.darker(0.3f).withAlpha(0.80f));
        g.strokePath(mouth, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Eye
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillEllipse(cx + bodyR * 0.42f - 2.0f, cy - bodyR * 0.22f - 2.0f, 4.0f, 4.0f);
    }

    //--------------------------------------------------------------------------
    // ODDFELIX — Neon tetra with glowing stripe
    // Small fish body (ellipse) with a horizontal stripe through the middle.
    // Stripe drawn twice (different alpha) for glow effect.
    // breathScale pulses the glow. couplingLean tilts direction.
    static void drawOddfelix(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.10f;
        float cy = b.getCentreY() + (breathScale - 1.0f) * b.getHeight() * 0.08f;

        float bodyW = b.getWidth() * 0.58f;
        float bodyH = b.getHeight() * 0.30f;

        // Body — small fish ellipse
        g.setColour(accent.withAlpha(0.55f));
        g.fillEllipse(cx - bodyW * 0.5f, cy - bodyH * 0.5f, bodyW, bodyH);

        // Horizontal stripe — drawn thick with glow (two passes)
        float stripeH = bodyH * 0.32f;
        float stripeStartX = cx - bodyW * 0.42f;
        float stripeEndX   = cx + bodyW * 0.32f;
        float stripeCy = cy - bodyH * 0.04f; // slightly above centre

        // Glow pass — wider, lower alpha
        float glowAlpha = 0.30f + (breathScale - 1.0f) * 3.0f;
        glowAlpha = juce::jlimit(0.20f, 0.55f, glowAlpha);
        g.setColour(accent.withAlpha(glowAlpha));
        g.fillRect(stripeStartX, stripeCy - stripeH, stripeEndX - stripeStartX, stripeH * 2.0f);

        // Main stripe — crisp, higher alpha
        float mainAlpha = 0.78f + (breathScale - 1.0f) * 2.0f;
        mainAlpha = juce::jlimit(0.65f, 0.95f, mainAlpha);
        g.setColour(accent.withAlpha(mainAlpha));
        g.fillRect(stripeStartX, stripeCy - stripeH * 0.55f, stripeEndX - stripeStartX, stripeH * 1.10f);

        // Eye — small, on upper right
        float eyeX = cx + bodyW * 0.26f;
        float eyeY = cy - bodyH * 0.14f;
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillEllipse(eyeX - 2.0f, eyeY - 2.0f, 4.0f, 4.0f);

        // Tail fin — forked from left end
        float tailBaseX = cx - bodyW * 0.48f;
        float tailTipX  = cx - bodyW * 0.70f;
        g.setColour(accent.withAlpha(0.65f));
        juce::Path tail;
        tail.startNewSubPath(tailBaseX, cy - bodyH * 0.28f);
        tail.lineTo(tailTipX, cy - bodyH * 0.06f);
        tail.startNewSubPath(tailBaseX, cy + bodyH * 0.28f);
        tail.lineTo(tailTipX, cy + bodyH * 0.06f);
        g.strokePath(tail, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    //--------------------------------------------------------------------------
    // Default — enhanced blob + eye + particle dots
    // Improves on the plain blob by adding 2-3 small accent-coloured particles.
    static void drawDefault(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent, float breathScale,
                            float couplingLean)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        float radius = b.getWidth() * 0.28f * breathScale;

        // Soft glow halo
        g.setColour(accent.withAlpha(0.20f));
        g.fillEllipse(cx - radius * 2.0f, cy - radius * 2.0f, radius * 4.0f, radius * 4.0f);

        // Body
        g.setColour(accent.withAlpha(0.70f));
        g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

        // Eye — shifts with couplingLean
        float eyeOffsetX = radius * 0.30f * couplingLean;
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillEllipse(cx + eyeOffsetX - 2.0f, cy - 2.0f, 4.0f, 4.0f);

        // Particle dots — 3 small accent circles orbiting around the body
        const float particleAngles[] = {0.4f, 2.1f, 4.5f}; // radians
        const float particleDist = radius * 1.5f;
        g.setColour(accent.withAlpha(0.50f));
        for (int p = 0; p < 3; ++p)
        {
            float angle = particleAngles[p] + breathScale * 0.5f; // slight rotation
            float px = cx + particleDist * std::cos(angle);
            float py = cy + particleDist * std::sin(angle);
            g.fillEllipse(px - 1.5f, py - 1.5f, 3.0f, 3.0f);
        }
    }

    JUCE_DECLARE_NON_COPYABLE(CreatureRenderer)
};

} // namespace xoceanus
