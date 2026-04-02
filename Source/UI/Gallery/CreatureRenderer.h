#pragma once
#include <juce_graphics/juce_graphics.h>
#include <cmath>
#include "../GalleryColors.h"

namespace xoceanus {

//==============================================================================
// CreatureRenderer — procedural per-engine creature sprites for CompactEngineTile
//
// Draws a distinctive creature shape based on engine archetype.
// All creatures:
//   • Fit within the supplied bounds rectangle
//   • Use the accent colour
//   • React to breathScale (1.0 ± 0.1 typical range)
//   • React to couplingLean (-1 to +1) for directional lean
//   • Drawn with ~5-10 Graphics calls (simple, fast, no allocation)
//
// Engine archetypes:
//   OBRIX     — coral branch (3-4 vertical lines + tip circles, breathe = sway)
//   OPERA     — singing fish (oval body + open-mouth arc, breathe = mouth opens)
//   OXYTOCIN  — jellyfish (dome arc + 4 tentacles, breathe = tentacle wave)
//   ONSET     — pistol shrimp (rect body + oversized claw circle, breathe = claw)
//   default   — blob + eye + 2-3 particle dots
//
class CreatureRenderer
{
public:
    static void drawCreature (juce::Graphics& g,
                              juce::Rectangle<float> bounds,
                              const juce::String& engineId,
                              const juce::Colour& accent,
                              float breathScale,
                              float couplingLean)
    {
        auto id = engineId.toUpperCase();

        if (id == "OBRIX")
            drawCoral (g, bounds, accent, breathScale, couplingLean);
        else if (id == "OPERA")
            drawSingingFish (g, bounds, accent, breathScale, couplingLean);
        else if (id == "OXYTOCIN")
            drawJellyfish (g, bounds, accent, breathScale, couplingLean);
        else if (id == "ONSET")
            drawPistolShrimp (g, bounds, accent, breathScale, couplingLean);
        else
            drawDefault (g, bounds, accent, breathScale, couplingLean);
    }

private:
    //--------------------------------------------------------------------------
    // OBRIX — Coral branch
    // 3-4 vertical lines with small circles at the tips.
    // breathScale drives a left/right sway offset.
    // couplingLean biases the sway direction.
    static void drawCoral (juce::Graphics& g,
                           juce::Rectangle<float> b,
                           const juce::Colour& accent,
                           float breathScale,
                           float couplingLean)
    {
        float cx   = b.getCentreX();
        float cy   = b.getCentreY();
        float h    = b.getHeight();
        float w    = b.getWidth();

        // Sway: breathScale oscillates around 1.0; map to ±3px lateral offset
        float sway = (breathScale - 1.0f) * 20.0f + couplingLean * 3.0f;

        // Branch positions (relative to centre)
        const float branchXOffsets[] = { -w * 0.28f, -w * 0.08f, w * 0.10f, w * 0.28f };
        const float branchHeights[]  = { h * 0.55f,  h * 0.70f,  h * 0.60f, h * 0.50f };

        g.setColour(accent.withAlpha(0.80f));

        for (int i = 0; i < 4; ++i)
        {
            float bx  = cx + branchXOffsets[i] + sway * (0.5f + (float)i * 0.15f);
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
        g.drawLine(cx + branchXOffsets[0] + sway * 0.5f,
                   baseY,
                   cx + branchXOffsets[3] + sway * 0.8f,
                   baseY, 1.5f);
    }

    //--------------------------------------------------------------------------
    // OPERA — Singing fish
    // Oval body with an open mouth (arc). breathScale drives mouth aperture.
    // couplingLean tilts the fish horizontally.
    static void drawSingingFish (juce::Graphics& g,
                                  juce::Rectangle<float> b,
                                  const juce::Colour& accent,
                                  float breathScale,
                                  float couplingLean)
    {
        float cx  = b.getCentreX() + couplingLean * b.getWidth() * 0.12f;
        float cy  = b.getCentreY();
        float bW  = b.getWidth()  * 0.65f * breathScale;
        float bH  = b.getHeight() * 0.45f;

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
        float mouthAperture = juce::MathConstants<float>::pi
                              * 0.25f * (breathScale - 0.9f) * 8.0f;
        mouthAperture = juce::jlimit(0.05f, juce::MathConstants<float>::pi * 0.45f, mouthAperture);

        float mouthR = bH * 0.22f;
        float mouthCx = cx + bW * 0.38f;
        float mouthCy = cy + bH * 0.08f;

        juce::Path mouth;
        mouth.addCentredArc(mouthCx, mouthCy, mouthR, mouthR * 0.6f,
                            0.0f,
                            -mouthAperture,
                             mouthAperture, true);
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.70f));
        g.strokePath(mouth, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

        // Tail — simple triangle chevron on left
        float tailTipX = cx - bW * 0.60f;
        float tailBaseX = cx - bW * 0.38f;
        g.setColour(accent.withAlpha(0.60f));
        juce::Path tail;
        tail.startNewSubPath(tailBaseX, cy - bH * 0.25f);
        tail.lineTo(tailTipX, cy);
        tail.lineTo(tailBaseX, cy + bH * 0.25f);
        g.strokePath(tail, juce::PathStrokeType(1.5f, juce::PathStrokeType::mitered,
                                                  juce::PathStrokeType::butt));
    }

    //--------------------------------------------------------------------------
    // OXYTOCIN — Jellyfish
    // Dome (arc) with 4 trailing tentacles (vertical sine curves).
    // breathScale drives tentacle wave amplitude.
    // couplingLean tilts the dome slightly.
    static void drawJellyfish (juce::Graphics& g,
                                juce::Rectangle<float> b,
                                const juce::Colour& accent,
                                float breathScale,
                                float couplingLean)
    {
        float cx     = b.getCentreX() + couplingLean * b.getWidth() * 0.10f;
        float cy     = b.getCentreY();
        float domeR  = b.getWidth()  * 0.34f * breathScale;
        float domeY  = cy - b.getHeight() * 0.05f;

        // Bell / dome — filled half-ellipse
        juce::Path dome;
        dome.addCentredArc(cx, domeY, domeR, domeR * 0.65f, 0.0f,
                           juce::MathConstants<float>::pi,
                           juce::MathConstants<float>::twoPi, true);
        dome.closeSubPath();
        g.setColour(accent.withAlpha(0.55f));
        g.fillPath(dome);

        // Dome outline
        g.setColour(accent.withAlpha(0.85f));
        g.strokePath(dome, juce::PathStrokeType(1.2f));

        // 4 tentacles — vertical sine curves hanging from dome base
        const float tentacleXOffsets[] = { -domeR * 0.6f, -domeR * 0.2f,
                                             domeR * 0.2f,  domeR * 0.6f };
        float tentacleLen = b.getHeight() * 0.38f;
        float waveAmp     = 3.0f * breathScale;  // breathing drives wave width

        g.setColour(accent.withAlpha(0.65f));
        for (int t = 0; t < 4; ++t)
        {
            float tx = cx + tentacleXOffsets[t];
            float startY = domeY + domeR * 0.55f;  // base of dome

            // Phase offset per tentacle so they wave out of phase
            float phaseOffset = (float)t * juce::MathConstants<float>::halfPi;

            juce::Path tentacle;
            const int kSegments = 6;
            tentacle.startNewSubPath(tx, startY);
            for (int s = 1; s <= kSegments; ++s)
            {
                float frac   = (float)s / (float)kSegments;
                float segY   = startY + frac * tentacleLen;
                float segX   = tx + waveAmp * std::sin(frac * juce::MathConstants<float>::twoPi
                                                         + phaseOffset);
                tentacle.lineTo(segX, segY);
            }
            g.strokePath(tentacle, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
        }
    }

    //--------------------------------------------------------------------------
    // ONSET — Pistol shrimp
    // Rectangular body with one oversized claw (large circle).
    // breathScale drives claw open/close (radius pulses).
    // couplingLean determines which side the large claw is on.
    static void drawPistolShrimp (juce::Graphics& g,
                                   juce::Rectangle<float> b,
                                   const juce::Colour& accent,
                                   float breathScale,
                                   float couplingLean)
    {
        float cx     = b.getCentreX();
        float cy     = b.getCentreY();
        float bodyW  = b.getWidth()  * 0.42f;
        float bodyH  = b.getHeight() * 0.32f;

        // Body — rounded rectangle
        g.setColour(accent.withAlpha(0.70f));
        g.fillRoundedRectangle(cx - bodyW * 0.5f, cy - bodyH * 0.5f,
                                bodyW, bodyH, bodyH * 0.3f);

        // Small eye on body
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillEllipse(cx + bodyW * 0.28f, cy - bodyH * 0.15f, 3.0f, 3.0f);

        // Claw side depends on couplingLean
        float clawSide    = (couplingLean >= 0.0f) ? 1.0f : -1.0f;
        float clawCx      = cx + clawSide * (bodyW * 0.5f + b.getWidth() * 0.18f);
        float clawCy      = cy - b.getHeight() * 0.08f;
        float clawR       = b.getWidth() * 0.18f * breathScale;  // pulse on breathScale

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
        float armEndX   = clawCx - clawSide * clawR;
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
    // Default — enhanced blob + eye + particle dots
    // Improves on the plain blob by adding 2-3 small accent-coloured particles.
    static void drawDefault (juce::Graphics& g,
                              juce::Rectangle<float> b,
                              const juce::Colour& accent,
                              float breathScale,
                              float couplingLean)
    {
        float cx     = b.getCentreX();
        float cy     = b.getCentreY();
        float radius = b.getWidth() * 0.28f * breathScale;

        // Soft glow halo
        g.setColour(accent.withAlpha(0.20f));
        g.fillEllipse(cx - radius * 2.0f, cy - radius * 2.0f,
                      radius * 4.0f, radius * 4.0f);

        // Body
        g.setColour(accent.withAlpha(0.70f));
        g.fillEllipse(cx - radius, cy - radius,
                      radius * 2.0f, radius * 2.0f);

        // Eye — shifts with couplingLean
        float eyeOffsetX = radius * 0.30f * couplingLean;
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillEllipse(cx + eyeOffsetX - 2.0f, cy - 2.0f, 4.0f, 4.0f);

        // Particle dots — 3 small accent circles orbiting around the body
        const float particleAngles[] = { 0.4f, 2.1f, 4.5f };  // radians
        const float particleDist     = radius * 1.5f;
        g.setColour(accent.withAlpha(0.50f));
        for (int p = 0; p < 3; ++p)
        {
            float angle = particleAngles[p] + breathScale * 0.5f;  // slight rotation
            float px = cx + particleDist * std::cos(angle);
            float py = cy + particleDist * std::sin(angle);
            g.fillEllipse(px - 1.5f, py - 1.5f, 3.0f, 3.0f);
        }
    }

    JUCE_DECLARE_NON_COPYABLE (CreatureRenderer)
};

} // namespace xoceanus
