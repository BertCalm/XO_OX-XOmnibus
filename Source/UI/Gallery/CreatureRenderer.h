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
        // Chef Quad (Organs)
        else if (id == "OTO")
            drawOto(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OCTAVE")
            drawOctave(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OLEG")
            drawOleg(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OTIS")
            drawOtis(g, bounds, accent, breathScale, couplingLean);
        // Kitchen Quad (Pianos)
        else if (id == "OVEN")
            drawOven(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OCHRE")
            drawOchre(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OBELISK")
            drawObelisk(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OPALINE")
            drawOpaline(g, bounds, accent, breathScale, couplingLean);
        // Cellar Quad (Bass)
        else if (id == "OGRE")
            drawOgre(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OLATE")
            drawOlate(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OAKEN")
            drawOaken(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OMEGA")
            drawOmega(g, bounds, accent, breathScale, couplingLean);
        // Garden Quad (Strings)
        else if (id == "ORCHARD")
            drawOrchard(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OVERGROW")
            drawOvergrow(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OSIER")
            drawOsier(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OXALIS")
            drawOxalis(g, bounds, accent, breathScale, couplingLean);
        // Broth Quad (Pads)
        else if (id == "OVERWASH")
            drawOverwash(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OVERWORN")
            drawOverworn(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OVERFLOW")
            drawOverflow(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OVERCAST")
            drawOvercast(g, bounds, accent, breathScale, couplingLean);
        // Fusion Quad (EP)
        else if (id == "OASIS")
            drawOasis(g, bounds, accent, breathScale, couplingLean);
        else if (id == "ODDFELLOW")
            drawOddfellow(g, bounds, accent, breathScale, couplingLean);
        else if (id == "ONKOLO")
            drawOnkolo(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OPCODE")
            drawOpcode(g, bounds, accent, breathScale, couplingLean);
        // Synthesis / Character Engines (batch B)
        else if (id == "ODDOSCAR")
            drawOddOscar(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OVERDUB")
            drawOverdub(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OVERBITE")
            drawOverbite(g, bounds, accent, breathScale, couplingLean);
        else if (id == "ORGANON")
            drawOrganon(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OBSCURA")
            drawObscura(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OCEANIC")
            drawOceanic(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OPTIC")
            drawOptic(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OBLIQUE")
            drawOblique(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OSPREY")
            drawOsprey(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OSTERIA")
            drawOsteria(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OHM")
            drawOhm(g, bounds, accent, breathScale, couplingLean);
        else if (id == "ORPHICA")
            drawOrphica(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OBBLIGATO")
            drawObbligato(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OTTONI")
            drawOttoni(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OLE")
            drawOle(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OMBRE")
            drawOmbre(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OCTOPUS")
            drawOctopus(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OPENSKY")
            drawOpensky(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OCEANDEEP")
            drawOceandeep(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OUIE")
            drawOuie(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OVERLAP")
            drawOverlap(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OUTWIT")
            drawOutwit(g, bounds, accent, breathScale, couplingLean);
        else if (id == "ORBWEAVE")
            drawOrbweave(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OVERTONE")
            drawOvertone(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OFFERING")
            drawOffering(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OSMOSIS")
            drawOsmosis(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OGIVE")
            drawOgive(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OLVIDO")
            drawOlvido(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OSTRACON")
            drawOstracon(g, bounds, accent, breathScale, couplingLean);
        // Specialty Engines
        else if (id == "ORRERY")
            drawOrrery(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OPSIN")
            drawOpsin(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OORT")
            drawOort(g, bounds, accent, breathScale, couplingLean);
        else if (id == "ONDINE")
            drawOndine(g, bounds, accent, breathScale, couplingLean);
        else if (id == "ORTOLAN")
            drawOrtolan(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OCTANT")
            drawOctant(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OVERTIDE")
            drawOvertide(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OOBLECK")
            drawOobleck(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OOZE")
            drawOoze(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OUTCROP")
            drawOutcrop(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OXIDIZE")
            drawOxidize(g, bounds, accent, breathScale, couplingLean);
        else if (id == "ONRUSH")
            drawOnrush(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OMNISTEREO")
            drawOmnistereo(g, bounds, accent, breathScale, couplingLean);
        // Effect / Utility Engines
        else if (id == "OBLITERATE")
            drawObliterate(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OBSCURITY")
            drawObscurity(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OUBLIETTE")
            drawOubliette(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OSMIUM")
            drawOsmium(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OROGEN")
            drawOrogen(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OCULUS")
            drawOculus(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OUTAGE")
            drawOutage(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OVERRIDE")
            drawOverride(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OCCLUSION")
            drawOcclusion(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OBDURATE")
            drawObdurate(g, bounds, accent, breathScale, couplingLean);
        else if (id == "ORISON")
            drawOrison(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OVERSHOOT")
            drawOvershoot(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OBVERSE")
            drawObverse(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OXYMORON")
            drawOxymoron(g, bounds, accent, breathScale, couplingLean);
        else if (id == "ORNATE")
            drawOrnate(g, bounds, accent, breathScale, couplingLean);
        else if (id == "ORATION")
            drawOration(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OFFCUT")
            drawOffcut(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OMEN")
            drawOmen(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OPUS")
            drawOpus(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OUTLAW")
            drawOutlaw(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OUTBREAK")
            drawOutbreak(g, bounds, accent, breathScale, couplingLean);
        else if (id == "OBSERVANDUM")
            drawObservandum(g, bounds, accent, breathScale, couplingLean);
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
    // OTO — Drawbar organist
    // 9 vertical bars of varying heights (harmonic series). breathScale extends bars.
    // couplingLean biases the cluster position.
    static void drawOto(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                        float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.10f;
        float cy = b.getCentreY();
        float w  = b.getWidth();
        float h  = b.getHeight();

        const float heightFracs[] = {0.9f, 0.7f, 0.8f, 0.6f, 0.5f, 0.4f, 0.3f, 0.5f, 0.2f};
        const int numBars = 9;
        float barW    = w * 0.065f;
        float spacing = w * 0.082f;
        float totalSpan = spacing * (numBars - 1);
        float startX  = cx - totalSpan * 0.5f;
        float baseY   = cy + h * 0.30f;

        for (int i = 0; i < numBars; ++i)
        {
            float barH = h * 0.60f * heightFracs[i] * breathScale;
            float bx   = startX + i * spacing;
            float topY = baseY - barH;
            float alpha = 0.55f + heightFracs[i] * 0.35f;
            g.setColour(accent.withAlpha(alpha));
            g.fillRect(bx - barW * 0.5f, topY, barW, barH);
            g.setColour(accent.withAlpha(0.92f));
            g.fillEllipse(bx - barW * 0.5f, topY - barW * 0.5f, barW, barW);
        }

        g.setColour(accent.withAlpha(0.55f));
        g.fillRect(startX - barW * 0.5f, baseY, totalSpan + barW, h * 0.030f);
    }

    //--------------------------------------------------------------------------
    // OCTAVE — Tonewheel
    // Central filled circle + 12 gear-tooth rectangles radiating outward.
    // breathScale acts as rotation phase offset. couplingLean shifts horizontally.
    static void drawOctave(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.10f;
        float cy = b.getCentreY();
        float w  = b.getWidth();

        float hubR   = w * 0.10f;
        float toothR = w * 0.26f;
        float toothW = w * 0.048f;
        float toothH = w * 0.10f;

        g.setColour(accent.withAlpha(0.40f));
        g.drawEllipse(cx - toothR, cy - toothR, toothR * 2.0f, toothR * 2.0f, 1.2f);

        float rotOffset = (breathScale - 1.0f) * juce::MathConstants<float>::twoPi * 2.0f;
        g.setColour(accent.withAlpha(0.82f));
        for (int i = 0; i < 12; ++i)
        {
            float angle = rotOffset + (float)i * juce::MathConstants<float>::twoPi / 12.0f;
            float tx = cx + toothR * std::cos(angle);
            float ty = cy + toothR * std::sin(angle);
            juce::Path tooth;
            tooth.addRectangle(-toothW * 0.5f, -toothH * 0.5f, toothW, toothH);
            g.fillPath(tooth, juce::AffineTransform::rotation(angle).translated(tx, ty));
        }

        g.setColour(accent.withAlpha(0.90f));
        g.fillEllipse(cx - hubR, cy - hubR, hubR * 2.0f, hubR * 2.0f);
        float axleR = hubR * 0.30f;
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillEllipse(cx - axleR, cy - axleR, axleR * 2.0f, axleR * 2.0f);
    }

    //--------------------------------------------------------------------------
    // OLEG — Pipe organ
    // 4 vertical filled rectangles of different heights. Connector bar at bottom.
    // breathScale extends pipes upward. couplingLean shifts the cluster.
    static void drawOleg(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                         float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.08f;
        float cy = b.getCentreY();
        float w  = b.getWidth();
        float h  = b.getHeight();

        const float heightFracs[] = {0.55f, 0.78f, 0.72f, 0.48f};
        const float xOffsets[]    = {-w * 0.22f, -w * 0.07f, w * 0.07f, w * 0.22f};
        float pipeW = w * 0.12f;
        float baseY = cy + h * 0.28f;

        for (int i = 0; i < 4; ++i)
        {
            float pipeH = h * heightFracs[i] * breathScale;
            float px    = cx + xOffsets[i];
            float topY  = baseY - pipeH;
            float mouthH = pipeH * 0.12f;

            g.setColour(accent.withAlpha(0.72f));
            g.fillRect(px - pipeW * 0.5f, topY, pipeW, pipeH);
            g.setColour(accent.withAlpha(0.95f));
            g.fillRect(px - pipeW * 0.5f, baseY - mouthH, pipeW, mouthH);
            g.setColour(accent.withAlpha(0.55f));
            g.fillEllipse(px - pipeW * 0.5f, topY - pipeW * 0.3f, pipeW, pipeW * 0.6f);
        }

        float barLeft  = cx + xOffsets[0] - pipeW * 0.5f;
        float barRight = cx + xOffsets[3] + pipeW * 0.5f;
        g.setColour(accent.withAlpha(0.60f));
        g.fillRect(barLeft, baseY, barRight - barLeft, h * 0.035f);
    }

    //--------------------------------------------------------------------------
    // OTIS — Soul drummer
    // Oval body, two small eyes near top, wide curved smile below.
    // Small side fins. breathScale widens the smile slightly.
    static void drawOtis(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                         float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.10f;
        float cy = b.getCentreY();
        float w  = b.getWidth();
        float h  = b.getHeight();

        float bodyW = w * 0.55f;
        float bodyH = h * 0.62f;

        g.setColour(accent.withAlpha(0.70f));
        g.fillEllipse(cx - bodyW * 0.5f, cy - bodyH * 0.5f, bodyW, bodyH);

        float eyeY   = cy - bodyH * 0.22f;
        float eyeR   = w * 0.038f;
        float eyeSep = bodyW * 0.22f;
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.92f));
        g.fillEllipse(cx - eyeSep - eyeR, eyeY - eyeR, eyeR * 2.0f, eyeR * 2.0f);
        g.fillEllipse(cx + eyeSep - eyeR, eyeY - eyeR, eyeR * 2.0f, eyeR * 2.0f);

        float smileAperture = juce::MathConstants<float>::pi * (0.35f + (breathScale - 0.9f) * 1.5f);
        smileAperture = juce::jlimit(0.25f, juce::MathConstants<float>::pi * 0.65f, smileAperture);
        float smileR  = bodyW * 0.32f;
        float smileCy = cy - bodyH * 0.02f;

        juce::Path smile;
        smile.addCentredArc(cx, smileCy, smileR, smileR * 0.55f, 0.0f,
                            juce::MathConstants<float>::pi - smileAperture,
                            juce::MathConstants<float>::pi + smileAperture, true);
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.80f));
        g.strokePath(smile, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour(accent.withAlpha(0.55f));
        g.fillEllipse(cx - bodyW * 0.5f - w * 0.08f, cy - h * 0.04f, w * 0.11f, h * 0.14f);
        g.fillEllipse(cx + bodyW * 0.5f - w * 0.03f, cy - h * 0.04f, w * 0.11f, h * 0.14f);
    }

    //--------------------------------------------------------------------------
    // OVEN — Piano hammer
    // Rectangular hammer head + felt circle on face + narrow handle stem.
    // breathScale drives a downward strike motion.
    static void drawOven(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                         float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.08f;
        float cy = b.getCentreY();
        float w  = b.getWidth();
        float h  = b.getHeight();

        float strikeOffset = (breathScale - 1.0f) * h * 0.18f;
        float headW = w * 0.40f;
        float headH = h * 0.25f;
        float headY = cy - headH * 0.5f + strikeOffset - h * 0.10f;

        g.setColour(accent.withAlpha(0.80f));
        g.fillRect(cx - headW * 0.5f, headY, headW, headH);

        float feltR = headW * 0.25f;
        g.setColour(accent.brighter(0.3f).withAlpha(0.90f));
        g.fillEllipse(cx - feltR, headY + headH - feltR * 0.8f, feltR * 2.0f, feltR * 1.6f);

        float stemW = w * 0.065f;
        float stemH = h * 0.38f;
        float stemY = headY + headH;
        g.setColour(accent.withAlpha(0.60f));
        g.fillRect(cx - stemW * 0.5f, stemY, stemW, stemH);

        g.setColour(accent.withAlpha(0.35f));
        float stringY = stemY + stemH + h * 0.02f;
        for (int i = 0; i < 3; ++i)
        {
            float sx = cx - w * 0.25f + i * w * 0.12f;
            g.drawLine(sx, stringY, sx, stringY + h * 0.10f, 0.8f);
        }
    }

    //--------------------------------------------------------------------------
    // OCHRE — Wooden resonator box
    // Rounded rectangle outline. 5 horizontal strings inside. Circular sound hole.
    static void drawOchre(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                          float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.08f;
        float cy = b.getCentreY();
        float w  = b.getWidth();
        float h  = b.getHeight();

        float boxW    = w * 0.72f;
        float boxH    = h * 0.55f;
        float cornerR = boxH * 0.12f;

        g.setColour(accent.withAlpha(0.18f));
        g.fillRoundedRectangle(cx - boxW * 0.5f, cy - boxH * 0.5f, boxW, boxH, cornerR);
        g.setColour(accent.withAlpha(0.60f));
        g.drawRoundedRectangle(cx - boxW * 0.5f, cy - boxH * 0.5f, boxW, boxH, cornerR, 1.8f);

        float strSpacing = boxH / 6.0f;
        float strLeft    = cx - boxW * 0.38f;
        float strRight   = cx + boxW * 0.38f;
        g.setColour(accent.withAlpha(0.75f * breathScale));
        for (int i = 0; i < 5; ++i)
        {
            float sy = cy - boxH * 0.5f + strSpacing * (i + 0.8f);
            g.drawLine(strLeft, sy, strRight, sy, 0.8f);
        }

        float holeR = boxW * 0.11f;
        g.setColour(accent.withAlpha(0.85f));
        g.drawEllipse(cx - holeR, cy - holeR, holeR * 2.0f, holeR * 2.0f, 1.5f);
        float roseR = holeR * 0.65f;
        g.setColour(accent.withAlpha(0.50f));
        g.drawEllipse(cx - roseR, cy - roseR, roseR * 2.0f, roseR * 2.0f, 0.8f);
    }

    //--------------------------------------------------------------------------
    // OBELISK — Crystal obelisk
    // Tapered quadrilateral (wide base, pointed top). 3 horizontal facet lines.
    // breathScale oscillates the tip. couplingLean tilts the obelisk.
    static void drawObelisk(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        float w  = b.getWidth();
        float h  = b.getHeight();

        float baseW  = w * 0.32f;
        float topW   = w * 0.04f;
        float obelH  = h * 0.78f;
        float baseY  = cy + obelH * 0.5f;
        float topY   = cy - obelH * 0.5f;
        float tiltX  = couplingLean * w * 0.08f;
        float tipOsc = (breathScale - 1.0f) * h * 0.06f;

        juce::Path obelisk;
        obelisk.startNewSubPath(cx - baseW * 0.5f,           baseY);
        obelisk.lineTo        (cx + baseW * 0.5f,            baseY);
        obelisk.lineTo        (cx + topW * 0.5f + tiltX,     topY + tipOsc);
        obelisk.lineTo        (cx - topW * 0.5f + tiltX,     topY + tipOsc);
        obelisk.closeSubPath();

        g.setColour(accent.withAlpha(0.65f));
        g.fillPath(obelisk);
        g.setColour(accent.withAlpha(0.90f));
        g.strokePath(obelisk, juce::PathStrokeType(1.2f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));

        const float facetFracs[] = {0.25f, 0.50f, 0.75f};
        g.setColour(accent.withAlpha(0.38f));
        for (int i = 0; i < 3; ++i)
        {
            float fy   = baseY - obelH * facetFracs[i];
            float frac = facetFracs[i];
            float fw   = (baseW * (1.0f - frac) + topW * frac) * 0.5f;
            float ft   = tiltX * frac;
            g.drawLine(cx - fw + ft, fy, cx + fw + ft, fy, 0.9f);
        }
    }

    //--------------------------------------------------------------------------
    // OPALINE — Prepared piano strings
    // 5 horizontal strings with small circle obstacles (preparations).
    // breathScale makes strings vibrate (sinusoidal midpoint offset).
    static void drawOpaline(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.08f;
        float cy = b.getCentreY();
        float w  = b.getWidth();
        float h  = b.getHeight();

        float strLeft  = cx - w * 0.38f;
        float strRight = cx + w * 0.38f;
        float strSpan  = strRight - strLeft;
        float strSpacingY = h * 0.12f;
        float startY      = cy - strSpacingY * 2.0f;

        const float obstacleX[] = {0.25f, 0.55f, 0.40f, 0.70f, 0.35f};

        for (int s = 0; s < 5; ++s)
        {
            float baseStrY = startY + s * strSpacingY;
            float phase    = (float)s * juce::MathConstants<float>::pi * 0.4f;
            float vibAmp   = (breathScale - 1.0f) * h * 0.08f;
            float midOsc   = vibAmp * std::sin(phase * 3.0f + (float)s);

            juce::Path str;
            str.startNewSubPath(strLeft, baseStrY);
            str.quadraticTo(cx, baseStrY + midOsc, strRight, baseStrY);
            g.setColour(accent.withAlpha(0.75f));
            g.strokePath(str, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            float obsX = strLeft + obstacleX[s] * strSpan;
            float obsY = baseStrY + midOsc * obstacleX[s];
            float obsR = w * 0.028f;
            g.setColour(accent.withAlpha(0.90f));
            g.fillEllipse(obsX - obsR, obsY - obsR, obsR * 2.0f, obsR * 2.0f);
            g.setColour(accent.withAlpha(0.40f));
            g.drawEllipse(obsX - obsR * 1.5f, obsY - obsR * 1.5f, obsR * 3.0f, obsR * 3.0f, 0.7f);
        }
    }

    //--------------------------------------------------------------------------
    // OGRE — Deep sea monster
    // Large ellipse body, oversized white eyes with pupils, jagged toothed mouth,
    // 3 tentacles below. breathScale drives jaw movement.
    static void drawOgre(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                         float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.08f;
        float cy = b.getCentreY();
        float w  = b.getWidth();
        float h  = b.getHeight();

        float bodyW = w * 0.70f;
        float bodyH = h * 0.65f;

        g.setColour(accent.withAlpha(0.72f));
        g.fillEllipse(cx - bodyW * 0.5f, cy - bodyH * 0.5f, bodyW, bodyH);

        float eyeR    = bodyW * 0.18f;
        float eyeSepX = bodyW * 0.22f;
        float eyeY    = cy - bodyH * 0.18f;

        g.setColour(juce::Colours::white.withAlpha(0.88f));
        g.fillEllipse(cx - eyeSepX - eyeR, eyeY - eyeR, eyeR * 2.0f, eyeR * 2.0f);
        g.fillEllipse(cx + eyeSepX - eyeR, eyeY - eyeR, eyeR * 2.0f, eyeR * 2.0f);

        float pupilR = eyeR * 0.50f;
        g.setColour(juce::Colours::black.withAlpha(0.88f));
        g.fillEllipse(cx - eyeSepX - pupilR, eyeY - pupilR, pupilR * 2.0f, pupilR * 2.0f);
        g.fillEllipse(cx + eyeSepX - pupilR, eyeY - pupilR, pupilR * 2.0f, pupilR * 2.0f);

        float jawDrop  = (breathScale - 1.0f) * h * 0.12f;
        float mouthY   = cy + bodyH * 0.12f;
        float mouthW   = bodyW * 0.62f;
        const int numTeeth = 5;
        float toothW   = mouthW / numTeeth;
        float toothH   = h * 0.06f + jawDrop;

        juce::Path mouth;
        mouth.startNewSubPath(cx - mouthW * 0.5f, mouthY);
        mouth.lineTo(cx + mouthW * 0.5f, mouthY);
        mouth.startNewSubPath(cx - mouthW * 0.5f, mouthY + jawDrop * 0.3f);
        for (int t = 0; t < numTeeth; ++t)
        {
            float tx = cx - mouthW * 0.5f + t * toothW;
            mouth.lineTo(tx + toothW * 0.5f, mouthY + toothH);
            mouth.lineTo(tx + toothW, mouthY + jawDrop * 0.3f);
        }
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.85f));
        g.strokePath(mouth, juce::PathStrokeType(1.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));

        float tentBaseY = cy + bodyH * 0.48f;
        float tentLen   = h * 0.28f;
        g.setColour(accent.withAlpha(0.65f));
        for (int t = 0; t < 3; ++t)
        {
            float tx    = cx + (t - 1) * bodyW * 0.28f;
            float phase = (float)t * juce::MathConstants<float>::pi * 0.7f;
            float waveX = 4.0f * std::sin(breathScale * juce::MathConstants<float>::pi + phase);
            juce::Path tent;
            tent.startNewSubPath(tx, tentBaseY);
            tent.cubicTo(tx + waveX, tentBaseY + tentLen * 0.4f,
                         tx - waveX, tentBaseY + tentLen * 0.7f,
                         tx + waveX * 0.5f, tentBaseY + tentLen);
            g.strokePath(tent, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    //--------------------------------------------------------------------------
    // OLATE — Fretless eel
    // Sinuous S-curve body (no markings). Small head circle at top.
    // breathScale drives S-curve amplitude. couplingLean offsets the curve.
    static void drawOlate(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                          float breathScale, float couplingLean)
    {
        float cx    = b.getCentreX() + couplingLean * b.getWidth() * 0.08f;
        float cy    = b.getCentreY();
        float w     = b.getWidth();
        float h     = b.getHeight();
        float amp   = w * 0.18f * breathScale;
        float topY  = cy - h * 0.42f;
        float botY  = cy + h * 0.42f;
        float midY  = cy;

        juce::Path eel;
        eel.startNewSubPath(cx, topY);
        eel.cubicTo(cx + amp, topY + (midY - topY) * 0.4f,
                    cx - amp, topY + (midY - topY) * 0.7f,
                    cx, midY);
        eel.cubicTo(cx + amp, midY + (botY - midY) * 0.3f,
                    cx - amp, midY + (botY - midY) * 0.65f,
                    cx, botY);

        g.setColour(accent.withAlpha(0.60f));
        g.strokePath(eel, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(accent.withAlpha(0.90f));
        g.strokePath(eel, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        float headR = w * 0.08f;
        g.setColour(accent.withAlpha(0.92f));
        g.fillEllipse(cx - headR, topY - headR * 0.7f, headR * 2.0f, headR * 1.4f);
        float eyeR = headR * 0.30f;
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillEllipse(cx + headR * 0.30f - eyeR, topY - eyeR * 0.5f - eyeR, eyeR * 2.0f, eyeR * 2.0f);
    }

    //--------------------------------------------------------------------------
    // OAKEN — Oak tree creature
    // Broad trunk + large rounded canopy. 5 acorns in canopy.
    // breathScale pulses canopy size. couplingLean shifts the whole tree.
    static void drawOaken(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                          float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float w  = b.getWidth();
        float h  = b.getHeight();

        float trunkW = w * 0.14f;
        float trunkH = h * 0.40f;
        float trunkTopY = cy + h * 0.10f - trunkH;

        g.setColour(accent.withAlpha(0.70f));
        g.fillRect(cx - trunkW * 0.5f, trunkTopY, trunkW, trunkH);

        float canopyScale = 0.90f + (breathScale - 1.0f) * 0.8f;
        float canopyW = w * 0.70f * canopyScale;
        float canopyH = h * 0.55f * canopyScale;
        float canopyCy = cy - h * 0.15f;

        g.setColour(accent.withAlpha(0.62f));
        g.fillEllipse(cx - canopyW * 0.5f, canopyCy - canopyH * 0.5f, canopyW, canopyH);
        g.setColour(accent.withAlpha(0.85f));
        g.drawEllipse(cx - canopyW * 0.5f, canopyCy - canopyH * 0.5f, canopyW, canopyH, 1.2f);

        const float acornFX[] = {-0.22f, 0.18f, -0.05f, -0.30f, 0.28f};
        const float acornFY[] = {-0.18f, 0.10f,  0.20f,  0.08f,-0.08f};
        float acornR = w * 0.052f;
        g.setColour(accent.brighter(0.4f).withAlpha(0.88f));
        for (int i = 0; i < 5; ++i)
            g.fillEllipse(cx + acornFX[i] * canopyW - acornR,
                          canopyCy + acornFY[i] * canopyH - acornR,
                          acornR * 2.0f, acornR * 2.0f);
    }

    //--------------------------------------------------------------------------
    // OMEGA — Copper distillation still
    // Round flask body + narrow neck + coiled tube. Drip circle at coil end.
    // breathScale brightens the coil glow.
    static void drawOmega(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                          float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.08f;
        float cy = b.getCentreY();
        float w  = b.getWidth();
        float h  = b.getHeight();

        float flaskR  = w * 0.26f;
        float flaskCy = cy + h * 0.12f;
        g.setColour(accent.withAlpha(0.75f));
        g.fillEllipse(cx - flaskR, flaskCy - flaskR, flaskR * 2.0f, flaskR * 2.0f);
        g.setColour(accent.withAlpha(0.92f));
        g.drawEllipse(cx - flaskR, flaskCy - flaskR, flaskR * 2.0f, flaskR * 2.0f, 1.5f);

        float neckW    = w * 0.085f;
        float neckH    = h * 0.24f;
        float neckTopY = flaskCy - flaskR - neckH;
        g.setColour(accent.withAlpha(0.80f));
        g.fillRect(cx - neckW * 0.5f, neckTopY, neckW, neckH);

        float glowAlpha = juce::jlimit(0.45f, 0.90f, 0.55f + (breathScale - 1.0f) * 2.0f);
        float coilStartX = cx + neckW * 0.5f;
        float coilStartY = neckTopY + neckH * 0.3f;
        g.setColour(accent.withAlpha(glowAlpha));
        for (int coil = 0; coil < 3; ++coil)
        {
            float coilR  = (w * 0.18f) * (1.0f - coil * 0.25f);
            float coilCx = coilStartX + coilR * 0.5f;
            float coilCy = coilStartY + coil * h * 0.12f;
            juce::Path arc;
            arc.addCentredArc(coilCx, coilCy, coilR, coilR * 0.45f, 0.0f,
                              -juce::MathConstants<float>::pi * 0.6f,
                               juce::MathConstants<float>::pi * 1.1f, true);
            g.strokePath(arc, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        float dripX = coilStartX + w * 0.28f;
        float dripY = coilStartY + h * 0.28f;
        float dripR = w * 0.038f;
        g.setColour(accent.brighter(0.2f).withAlpha(0.85f));
        g.fillEllipse(dripX - dripR, dripY - dripR, dripR * 2.0f, dripR * 2.0f);
    }

    //--------------------------------------------------------------------------
    // ORCHARD — Apple tree
    // Trunk line + round canopy + 3 apple circles.
    // breathScale sways the canopy cx slightly.
    static void drawOrchard(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float w  = b.getWidth();
        float h  = b.getHeight();

        float trunkBotY = cy + h * 0.40f;
        float trunkTopY = cy + h * 0.05f;
        g.setColour(accent.withAlpha(0.72f));
        g.drawLine(cx, trunkBotY, cx, trunkTopY, w * 0.065f);

        float swayCx   = cx + (breathScale - 1.0f) * w * 0.10f;
        float canopyW  = w * 0.65f;
        float canopyH  = h * 0.52f;
        float canopyCy = cy - h * 0.12f;

        g.setColour(accent.withAlpha(0.65f));
        g.fillEllipse(swayCx - canopyW * 0.5f, canopyCy - canopyH * 0.5f, canopyW, canopyH);

        const float appleX[] = {-0.20f, 0.05f, 0.22f};
        const float appleY[] = { 0.05f,-0.12f, 0.10f};
        float appleR = w * 0.068f;
        g.setColour(accent.brighter(0.5f).withAlpha(0.90f));
        for (int i = 0; i < 3; ++i)
            g.fillEllipse(swayCx + appleX[i] * canopyW - appleR,
                          canopyCy + appleY[i] * canopyH - appleR,
                          appleR * 2.0f, appleR * 2.0f);

        g.setColour(accent.withAlpha(0.85f));
        g.drawEllipse(swayCx - canopyW * 0.5f, canopyCy - canopyH * 0.5f, canopyW, canopyH, 1.0f);
    }

    //--------------------------------------------------------------------------
    // OVERGROW — Vine creature
    // Central vertical stem + 4 pairs of spiral vine shoots. Leaf ellipses at tips.
    // breathScale drives spiral radius.
    static void drawOvergrow(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float w  = b.getWidth();
        float h  = b.getHeight();

        float stemTopY = cy - h * 0.42f;
        float stemBotY = cy + h * 0.42f;
        g.setColour(accent.withAlpha(0.78f));
        g.drawLine(cx, stemBotY, cx, stemTopY, w * 0.045f);

        const float branchFracs[] = {0.22f, 0.42f, 0.60f, 0.78f};
        float curlR = juce::jlimit(w * 0.08f, w * 0.22f,
                                   w * 0.14f * (0.8f + (breathScale - 1.0f) * 1.5f));

        for (int i = 0; i < 4; ++i)
        {
            float branchY = stemBotY - (stemBotY - stemTopY) * branchFracs[i];
            float side    = (i % 2 == 0) ? 1.0f : -1.0f;

            juce::Path curl;
            curl.addCentredArc(cx + side * curlR * 0.5f, branchY,
                               curlR * 0.5f, curlR * 0.4f, 0.0f,
                               juce::MathConstants<float>::pi,
                               -juce::MathConstants<float>::pi * 0.2f, true);
            g.setColour(accent.withAlpha(0.70f));
            g.strokePath(curl, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            float leafX = cx + side * curlR * 1.0f;
            float leafY = branchY - curlR * 0.3f;
            g.setColour(accent.brighter(0.3f).withAlpha(0.80f));
            g.fillEllipse(leafX - w * 0.05f, leafY - h * 0.033f, w * 0.10f, h * 0.065f);
        }
    }

    //--------------------------------------------------------------------------
    // OSIER — Willow
    // Short trunk + 6 drooping branch lines curving outward and downward.
    // breathScale oscillates endpoint Y (branches sway). Dot at each tip.
    static void drawOsier(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                          float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float w  = b.getWidth();
        float h  = b.getHeight();

        float trunkW   = w * 0.085f;
        float trunkH   = h * 0.32f;
        float trunkTopY = cy - h * 0.05f;
        g.setColour(accent.withAlpha(0.75f));
        g.fillRect(cx - trunkW * 0.5f, trunkTopY, trunkW, trunkH);

        const float branchAngles[] = {-0.55f, -0.28f, -0.08f, 0.08f, 0.28f, 0.55f};
        float branchLen = h * 0.55f;
        float sway      = (breathScale - 1.0f) * h * 0.10f;

        g.setColour(accent.withAlpha(0.72f));
        for (int i = 0; i < 6; ++i)
        {
            float angle      = branchAngles[i];
            float branchTopX = cx + std::sin(angle) * trunkW * 0.5f;
            float branchTopY = trunkTopY + trunkH * 0.1f;
            float endX       = cx + std::sin(angle) * branchLen * 0.52f;
            float endY       = trunkTopY + branchLen + sway * ((float)i / 5.0f - 0.5f) * 2.0f;

            juce::Path branch;
            branch.startNewSubPath(branchTopX, branchTopY);
            branch.cubicTo(branchTopX + std::sin(angle) * branchLen * 0.3f, branchTopY + branchLen * 0.2f,
                           endX - std::sin(angle) * branchLen * 0.1f, endY - branchLen * 0.2f,
                           endX, endY);
            g.strokePath(branch, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.fillEllipse(endX - 2.0f, endY - 2.0f, 4.0f, 4.0f);
        }
    }

    //--------------------------------------------------------------------------
    // OXALIS — Wood sorrel
    // Small stem + 3 heart-shaped leaves (two overlapping circles) at 120° angles.
    // breathScale splays leaves. couplingLean rotates the arrangement.
    static void drawOxalis(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        float w  = b.getWidth();
        float h  = b.getHeight();

        float stemBotY  = cy + h * 0.38f;
        float stemTopY  = cy + h * 0.05f;
        float baseRot   = couplingLean * juce::MathConstants<float>::pi * 0.20f;

        g.setColour(accent.withAlpha(0.72f));
        g.drawLine(cx, stemBotY, cx, stemTopY, w * 0.045f);

        float leafDist = juce::jlimit(h * 0.14f, h * 0.30f,
                                      h * 0.22f * (0.85f + (breathScale - 1.0f) * 1.2f));
        float leafR    = w * 0.14f;

        for (int i = 0; i < 3; ++i)
        {
            float angle  = baseRot + (float)i * juce::MathConstants<float>::twoPi / 3.0f
                           - juce::MathConstants<float>::halfPi;
            float leafCx = cx + leafDist * std::cos(angle);
            float leafCy = stemTopY + leafDist * std::sin(angle);
            float perpX  = -std::sin(angle);
            float perpY  =  std::cos(angle);
            float lobe   = leafR * 0.48f;

            g.setColour(accent.withAlpha(0.70f));
            g.fillEllipse(leafCx + perpX * lobe - leafR * 0.7f,
                          leafCy + perpY * lobe - leafR * 0.7f,
                          leafR * 1.4f, leafR * 1.4f);
            g.fillEllipse(leafCx - perpX * lobe - leafR * 0.7f,
                          leafCy - perpY * lobe - leafR * 0.7f,
                          leafR * 1.4f, leafR * 1.4f);
        }

        float nodeR = w * 0.045f;
        g.setColour(accent.withAlpha(0.90f));
        g.fillEllipse(cx - nodeR, stemTopY - nodeR, nodeR * 2.0f, nodeR * 2.0f);
    }

    //--------------------------------------------------------------------------
    // OVERWASH — Tide wave
    // Horizontal wave path cresting in the middle, filled to the bottom.
    // Foam dots along crest. breathScale drives crest height.
    static void drawOverwash(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float w  = b.getWidth();
        float h  = b.getHeight();

        float crestH = juce::jlimit(h * 0.12f, h * 0.42f, h * 0.28f * breathScale);
        float baseY  = cy + h * 0.28f;
        float crestY = cy - crestH * 0.5f;

        juce::Path wave;
        wave.startNewSubPath(b.getX(), baseY);
        wave.lineTo(b.getX(), crestY + crestH * 0.8f);
        wave.cubicTo(cx - w * 0.30f, crestY + crestH * 0.2f,
                     cx - w * 0.10f, crestY - crestH * 0.1f,
                     cx, crestY);
        wave.cubicTo(cx + w * 0.10f, crestY - crestH * 0.1f,
                     cx + w * 0.30f, crestY + crestH * 0.3f,
                     b.getRight(), crestY + crestH * 0.6f);
        wave.lineTo(b.getRight(), baseY);
        wave.closeSubPath();

        g.setColour(accent.withAlpha(0.55f));
        g.fillPath(wave);

        juce::Path crestLine;
        crestLine.startNewSubPath(b.getX(), crestY + crestH * 0.8f);
        crestLine.cubicTo(cx - w * 0.30f, crestY + crestH * 0.2f,
                          cx - w * 0.10f, crestY - crestH * 0.1f,
                          cx, crestY);
        crestLine.cubicTo(cx + w * 0.10f, crestY - crestH * 0.1f,
                          cx + w * 0.30f, crestY + crestH * 0.3f,
                          b.getRight(), crestY + crestH * 0.6f);
        g.setColour(accent.withAlpha(0.88f));
        g.strokePath(crestLine, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float foamFracs[] = {0.20f, 0.40f, 0.62f, 0.80f};
        g.setColour(juce::Colours::white.withAlpha(0.65f));
        float dotR = w * 0.025f;
        for (int i = 0; i < 4; ++i)
        {
            float fx   = b.getX() + foamFracs[i] * w;
            float frac = foamFracs[i];
            float localY = (frac < 0.5f)
                ? (crestY + crestH * 0.8f) + (frac / 0.5f) * (crestY - (crestY + crestH * 0.8f))
                : crestY + (frac - 0.5f) / 0.5f * ((crestY + crestH * 0.6f) - crestY);
            g.fillEllipse(fx - dotR, localY - dotR, dotR * 2.0f, dotR * 2.0f);
        }
    }

    //--------------------------------------------------------------------------
    // OVERWORN — Worn fabric
    // Rounded rectangle outline. Dot clusters + angled thread lines.
    // breathScale subtly shifts the thread lines.
    static void drawOverworn(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float w  = b.getWidth();
        float h  = b.getHeight();

        float fabW    = w * 0.70f;
        float fabH    = h * 0.58f;
        float cornerR = fabH * 0.10f;

        g.setColour(accent.withAlpha(0.20f));
        g.fillRoundedRectangle(cx - fabW * 0.5f, cy - fabH * 0.5f, fabW, fabH, cornerR);
        g.setColour(accent.withAlpha(0.65f));
        g.drawRoundedRectangle(cx - fabW * 0.5f, cy - fabH * 0.5f, fabW, fabH, cornerR, 1.8f);

        float shift = (breathScale - 1.0f) * h * 0.06f;
        const float threadY[] = {-0.28f, -0.12f, 0.04f, 0.18f, 0.32f};
        g.setColour(accent.withAlpha(0.55f));
        for (int i = 0; i < 5; ++i)
        {
            float ty   = cy + threadY[i] * fabH + shift * ((i % 2 == 0) ? 1.0f : -1.0f);
            float tilt = (i % 2 == 0) ? fabH * 0.03f : -fabH * 0.025f;
            g.drawLine(cx - fabW * 0.40f, ty + tilt, cx + fabW * 0.40f, ty - tilt, 0.8f);
        }

        const float patchCX[] = {-0.22f,  0.18f, -0.10f, 0.28f};
        const float patchCY[] = { 0.15f, -0.20f,  0.35f, 0.22f};
        g.setColour(accent.withAlpha(0.72f));
        for (int p = 0; p < 4; ++p)
        {
            float pcx = cx + patchCX[p] * fabW;
            float pcy = cy + patchCY[p] * fabH;
            for (int d = 0; d < 3; ++d)
            {
                float offX = (d - 1) * w * 0.038f;
                float offY = (d == 1) ? -h * 0.030f : h * 0.012f;
                float dotR = w * 0.022f;
                g.fillEllipse(pcx + offX - dotR, pcy + offY - dotR, dotR * 2.0f, dotR * 2.0f);
            }
        }
    }

    //--------------------------------------------------------------------------
    // OVERFLOW — River current
    // 5 horizontal curved flow lines. 3 eddy circles scattered.
    // breathScale moves lines in opposite phase (flowing animation).
    static void drawOverflow(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float w  = b.getWidth();
        float h  = b.getHeight();

        float lineSpacing = h * 0.14f;
        float startY      = cy - lineSpacing * 2.0f;
        float flowShift   = (breathScale - 1.0f) * w * 0.12f;

        for (int i = 0; i < 5; ++i)
        {
            float lineY  = startY + i * lineSpacing;
            float phase  = (i % 2 == 0) ? 1.0f : -1.0f;
            float shiftX = flowShift * phase;
            float left   = cx - w * 0.40f + shiftX;
            float right  = cx + w * 0.40f + shiftX;
            float bulgY  = lineY + h * 0.04f * (i % 2 == 0 ? 1.0f : -1.0f);
            float alpha  = juce::jlimit(0.45f, 0.85f, 0.50f + (float)i * 0.09f);

            juce::Path flow;
            flow.startNewSubPath(left, lineY);
            flow.quadraticTo(cx + shiftX, bulgY, right, lineY);
            g.setColour(accent.withAlpha(alpha));
            g.strokePath(flow, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        const float eddyCX[] = { 0.12f, -0.20f,  0.30f};
        const float eddyCY[] = {-0.15f,  0.18f,  0.08f};
        float eddyR = w * 0.058f;
        g.setColour(accent.withAlpha(0.45f));
        for (int e = 0; e < 3; ++e)
        {
            float ex = cx + eddyCX[e] * w;
            float ey = cy + eddyCY[e] * h;
            g.drawEllipse(ex - eddyR, ey - eddyR, eddyR * 2.0f, eddyR * 2.0f, 1.0f);
            g.drawEllipse(ex - eddyR * 0.55f, ey - eddyR * 0.55f, eddyR * 1.1f, eddyR * 1.1f, 0.6f);
        }
    }

    //--------------------------------------------------------------------------
    // OVERCAST — Cloud
    // 5 overlapping circles forming a cloud silhouette.
    // breathScale scales the whole cloud. couplingLean shifts horizontally.
    static void drawOvercast(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.08f;
        float cy = b.getCentreY();
        float w  = b.getWidth();

        float cloudScale = juce::jlimit(0.75f, 1.10f, 0.88f + (breathScale - 1.0f) * 0.8f);

        struct CloudCircle { float relX, relY, r; };
        const CloudCircle circles[] = {
            {-0.22f,  0.10f, 0.16f},
            { 0.00f,  0.12f, 0.20f},
            { 0.22f,  0.10f, 0.16f},
            {-0.10f, -0.08f, 0.20f},
            { 0.12f, -0.10f, 0.22f},
        };

        g.setColour(accent.withAlpha(0.52f));
        for (const auto& c : circles)
        {
            float cr = w * c.r * cloudScale;
            g.fillEllipse(cx + c.relX * w * cloudScale - cr,
                          cy + c.relY * w * cloudScale - cr,
                          cr * 2.0f, cr * 2.0f);
        }
        g.setColour(accent.withAlpha(0.80f));
        for (const auto& c : circles)
        {
            float cr = w * c.r * cloudScale;
            g.drawEllipse(cx + c.relX * w * cloudScale - cr,
                          cy + c.relY * w * cloudScale - cr,
                          cr * 2.0f, cr * 2.0f, 1.0f);
        }
    }

    //--------------------------------------------------------------------------
    // OASIS — Spring oasis fish
    // Small oval fish body emerging from a pool ellipse below.
    // 3 bubble circles rising above. breathScale makes bubbles larger.
    static void drawOasis(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                          float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.08f;
        float cy = b.getCentreY();
        float w  = b.getWidth();
        float h  = b.getHeight();

        float poolW  = w * 0.62f;
        float poolH  = h * 0.22f;
        float poolCy = cy + h * 0.22f;
        g.setColour(accent.withAlpha(0.60f));
        g.drawEllipse(cx - poolW * 0.5f, poolCy - poolH * 0.5f, poolW, poolH, 1.5f);

        float fishW  = w * 0.42f;
        float fishH  = h * 0.28f;
        float fishCy = poolCy - fishH * 0.5f;
        g.setColour(accent.withAlpha(0.75f));
        g.fillEllipse(cx - fishW * 0.5f, fishCy - fishH * 0.5f, fishW, fishH);

        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillEllipse(cx + fishW * 0.22f - 2.0f, fishCy - fishH * 0.14f - 2.0f, 4.0f, 4.0f);

        float tailBaseX = cx - fishW * 0.45f;
        float tailTipX  = cx - fishW * 0.68f;
        g.setColour(accent.withAlpha(0.65f));
        juce::Path tail;
        tail.startNewSubPath(tailBaseX, fishCy - fishH * 0.22f);
        tail.lineTo(tailTipX, fishCy);
        tail.startNewSubPath(tailBaseX, fishCy + fishH * 0.22f);
        tail.lineTo(tailTipX, fishCy);
        g.strokePath(tail, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        float bubbleR = juce::jlimit(w * 0.018f, w * 0.055f,
                                     w * 0.028f * (0.85f + (breathScale - 1.0f) * 1.5f));
        const float bubXOff[] = {-0.15f, 0.02f, 0.18f};
        const float bubYOff[] = {-0.28f, -0.42f, -0.32f};
        g.setColour(accent.withAlpha(0.55f));
        for (int i = 0; i < 3; ++i)
            g.drawEllipse(cx + bubXOff[i] * w - bubbleR,
                          poolCy + bubYOff[i] * h - bubbleR,
                          bubbleR * 2.0f, bubbleR * 2.0f, 1.0f);
    }

    //--------------------------------------------------------------------------
    // ODDFELLOW — Odd jazz fish
    // Angular asymmetric fish body. One eye much larger than the other.
    // Angular dorsal fin. Bow-tie on body. couplingLean tilts dramatically.
    static void drawOddfellow(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                              float breathScale, float couplingLean)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY() + (breathScale - 1.0f) * b.getHeight() * 0.08f;
        float w  = b.getWidth();
        float h  = b.getHeight();
        float tilt = couplingLean * juce::MathConstants<float>::pi * 0.18f;

        float bW = w * 0.58f;
        float bH = h * 0.32f;
        juce::Path body;
        body.startNewSubPath(cx + bW * 0.5f,  cy + bH * 0.10f);
        body.lineTo(cx + bW * 0.20f,           cy - bH * 0.50f);
        body.lineTo(cx - bW * 0.48f,           cy - bH * 0.30f);
        body.lineTo(cx - bW * 0.50f,           cy + bH * 0.42f);
        body.lineTo(cx + bW * 0.22f,           cy + bH * 0.50f);
        body.closeSubPath();
        juce::AffineTransform bodyTilt = juce::AffineTransform::rotation(tilt, cx, cy);
        g.setColour(accent.withAlpha(0.72f));
        g.fillPath(body, bodyTilt);
        g.setColour(accent.withAlpha(0.90f));
        g.strokePath(body, juce::PathStrokeType(1.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt), bodyTilt);

        // Large left eye
        float bigEyeR = w * 0.085f;
        juce::Point<float> bigEyePt = juce::Point<float>(cx - bW * 0.10f, cy - bH * 0.15f).transformedBy(bodyTilt);
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.92f));
        g.fillEllipse(bigEyePt.x - bigEyeR, bigEyePt.y - bigEyeR, bigEyeR * 2.0f, bigEyeR * 2.0f);
        g.setColour(juce::Colours::black.withAlpha(0.70f));
        g.fillEllipse(bigEyePt.x - bigEyeR * 0.45f, bigEyePt.y - bigEyeR * 0.45f,
                      bigEyeR * 0.90f, bigEyeR * 0.90f);

        // Small right eye
        float smEyeR = w * 0.025f;
        juce::Point<float> smEyePt = juce::Point<float>(cx + bW * 0.28f, cy - bH * 0.10f).transformedBy(bodyTilt);
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillEllipse(smEyePt.x - smEyeR, smEyePt.y - smEyeR, smEyeR * 2.0f, smEyeR * 2.0f);

        // Angular dorsal fin
        juce::Path dorsalFin;
        dorsalFin.startNewSubPath(cx - bW * 0.05f, cy - bH * 0.48f);
        dorsalFin.lineTo(cx + bW * 0.25f, cy - bH * 1.0f);
        dorsalFin.lineTo(cx + bW * 0.30f, cy - bH * 0.48f);
        dorsalFin.closeSubPath();
        g.setColour(accent.withAlpha(0.85f));
        g.fillPath(dorsalFin, bodyTilt);

        // Bow-tie
        juce::Point<float> bowPt = juce::Point<float>(cx - bW * 0.10f, cy + bH * 0.12f).transformedBy(bodyTilt);
        float bowW = w * 0.10f;
        float bowH = h * 0.065f;
        juce::Path bowTie;
        bowTie.startNewSubPath(bowPt.x - bowW * 0.5f, bowPt.y - bowH * 0.5f);
        bowTie.lineTo(bowPt.x + bowW * 0.5f, bowPt.y + bowH * 0.5f);
        bowTie.lineTo(bowPt.x + bowW * 0.5f, bowPt.y - bowH * 0.5f);
        bowTie.lineTo(bowPt.x - bowW * 0.5f, bowPt.y + bowH * 0.5f);
        bowTie.closeSubPath();
        g.setColour(accent.brighter(0.4f).withAlpha(0.88f));
        g.fillPath(bowTie);
    }

    //--------------------------------------------------------------------------
    // ONKOLO — Nuclear resonating core
    // Central bright filled circle (core) + 6 hexagons in a honeycomb ring.
    // Faint radial lines from center. breathScale pulses the core.
    static void drawOnkolo(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float w  = b.getWidth();

        float coreR = w * 0.11f * breathScale;
        float ringR = w * 0.30f;
        float hexR  = w * 0.10f;

        // Radial lines
        g.setColour(accent.withAlpha(0.28f));
        for (int i = 0; i < 6; ++i)
        {
            float angle = (float)i * juce::MathConstants<float>::twoPi / 6.0f;
            g.drawLine(cx, cy,
                       cx + ringR * std::cos(angle),
                       cy + ringR * std::sin(angle), 0.8f);
        }

        // 6 hexagons in ring
        g.setColour(accent.withAlpha(0.72f));
        for (int i = 0; i < 6; ++i)
        {
            float hAngle = (float)i * juce::MathConstants<float>::twoPi / 6.0f;
            float hcx = cx + ringR * std::cos(hAngle);
            float hcy = cy + ringR * std::sin(hAngle);
            juce::Path hex;
            for (int v = 0; v < 6; ++v)
            {
                float vAngle = (float)v * juce::MathConstants<float>::twoPi / 6.0f;
                float vx = hcx + hexR * std::cos(vAngle);
                float vy = hcy + hexR * std::sin(vAngle);
                if (v == 0) hex.startNewSubPath(vx, vy);
                else        hex.lineTo(vx, vy);
            }
            hex.closeSubPath();
            g.strokePath(hex, juce::PathStrokeType(1.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
        }

        // Core
        g.setColour(accent.withAlpha(0.90f));
        g.fillEllipse(cx - coreR, cy - coreR, coreR * 2.0f, coreR * 2.0f);
        float innerR = coreR * 0.45f;
        g.setColour(accent.brighter(0.5f).withAlpha(0.95f));
        g.fillEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);
    }

    //--------------------------------------------------------------------------
    // OPCODE — Logic gate creature
    // Rectangular gate body. Two input lines (left) + output line (right).
    // Curved bump on right side (AND gate shape). breathScale pulses output length.
    static void drawOpcode(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float w  = b.getWidth();
        float h  = b.getHeight();

        float gateW    = w * 0.40f;
        float gateH    = h * 0.42f;
        float gateLeft = cx - gateW * 0.5f;
        float gateRight= cx + gateW * 0.5f;
        float gateTop  = cy - gateH * 0.5f;
        float gateBot  = cy + gateH * 0.5f;

        g.setColour(accent.withAlpha(0.60f));
        g.fillRect(gateLeft, gateTop, gateW, gateH);
        g.setColour(accent.withAlpha(0.90f));
        g.drawRect(gateLeft, gateTop, gateW, gateH, 1.5f);

        // Right bump (AND gate characteristic shape)
        juce::Path bump;
        bump.startNewSubPath(gateRight, gateTop);
        bump.cubicTo(gateRight + gateW * 0.60f, gateTop,
                     gateRight + gateW * 0.60f, gateBot,
                     gateRight, gateBot);
        g.setColour(accent.withAlpha(0.72f));
        g.fillPath(bump);
        g.setColour(accent.withAlpha(0.92f));
        g.strokePath(bump, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Input lines
        float inputLen = w * 0.20f;
        float input1Y  = cy - gateH * 0.22f;
        float input2Y  = cy + gateH * 0.22f;
        g.setColour(accent.withAlpha(0.80f));
        g.drawLine(gateLeft - inputLen, input1Y, gateLeft, input1Y, 1.2f);
        g.drawLine(gateLeft - inputLen, input2Y, gateLeft, input2Y, 1.2f);

        // Signal dots
        float dotR = w * 0.025f;
        g.setColour(accent.withAlpha(0.92f));
        g.fillEllipse(gateLeft - inputLen * 0.5f - dotR, input1Y - dotR, dotR * 2.0f, dotR * 2.0f);
        g.drawEllipse(gateLeft - inputLen * 0.5f - dotR, input2Y - dotR, dotR * 2.0f, dotR * 2.0f, 0.8f);

        // Output line — length pulses with breathScale
        float outputLen   = juce::jlimit(w * 0.08f, w * 0.32f, w * 0.18f * breathScale);
        float outputStart = gateRight + gateW * 0.55f;
        g.setColour(accent.withAlpha(0.85f));
        g.drawLine(outputStart, cy, outputStart + outputLen, cy, 1.5f);
        g.setColour(accent.brighter(0.4f).withAlpha(0.90f));
        g.fillEllipse(outputStart + outputLen - dotR, cy - dotR, dotR * 2.0f, dotR * 2.0f);
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

    //--------------------------------------------------------------------------
    // ODDOSCAR — tubby fish variant with eccentric crown spines
    static void drawOddOscar(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.08f;
        float cy = b.getCentreY();
        float bW = b.getWidth() * 0.52f * breathScale;
        float bH = b.getHeight() * 0.40f;
        g.setColour(accent.withAlpha(0.70f));
        g.fillEllipse(cx - bW * 0.5f, cy - bH * 0.5f, bW, bH);
        g.setColour(accent.withAlpha(0.90f));
        for (int i = 0; i < 3; ++i)
        {
            float sx = cx + (i - 1) * bW * 0.28f;
            float sh = bH * (0.30f + (float)(i == 1) * 0.15f);
            g.drawLine(sx, cy - bH * 0.50f, sx, cy - bH * 0.50f - sh, 1.5f);
            g.fillEllipse(sx - 2.0f, cy - bH * 0.50f - sh - 2.0f, 4.0f, 4.0f);
        }
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillEllipse(cx + bW * 0.18f - 2.0f, cy - bH * 0.08f - 2.0f, 4.0f, 4.0f);
    }

    //--------------------------------------------------------------------------
    // OVERDUB — layered tape waveforms
    static void drawOverdub(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float w = b.getWidth() * 0.72f;
        float startX = cx - w * 0.5f;
        float baseY = b.getCentreY() - b.getHeight() * 0.20f;
        for (int l = 0; l < 3; ++l)
        {
            float ly = baseY + (float)l * b.getHeight() * 0.20f;
            float amp = b.getHeight() * 0.07f * breathScale * (1.0f - (float)l * 0.22f);
            g.setColour(accent.withAlpha(0.80f - (float)l * 0.20f));
            juce::Path wave;
            wave.startNewSubPath(startX, ly);
            for (int p = 1; p <= 8; ++p)
            {
                float t = (float)p / 8.0f;
                wave.lineTo(startX + t * w,
                            ly + amp * std::sin(t * juce::MathConstants<float>::twoPi * 1.5f + (float)l * 0.9f));
            }
            g.strokePath(wave, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved,
                         juce::PathStrokeType::rounded));
        }
    }

    //--------------------------------------------------------------------------
    // OVERBITE — creature with prominent jagged teeth
    static void drawOverbite(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.08f;
        float cy = b.getCentreY();
        float bW = b.getWidth() * 0.56f;
        float bH = b.getHeight() * 0.32f * breathScale;
        g.setColour(accent.withAlpha(0.65f));
        g.fillEllipse(cx - bW * 0.5f, cy - bH * 0.5f, bW, bH);
        float jawY = cy + bH * 0.18f;
        float toothH = b.getHeight() * 0.14f;
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.85f));
        for (int t = 0; t < 4; ++t)
        {
            float tx = cx - bW * 0.32f + (float)t * bW * 0.21f;
            juce::Path tooth;
            tooth.startNewSubPath(tx, jawY);
            tooth.lineTo(tx + bW * 0.055f, jawY + toothH);
            tooth.lineTo(tx + bW * 0.11f, jawY);
            tooth.closeSubPath();
            g.fillPath(tooth);
        }
    }

    //--------------------------------------------------------------------------
    // ORGANON — organ pipes of varying height
    static void drawOrganon(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float baseY = b.getBottom() - b.getHeight() * 0.18f;
        const float heights[] = {0.50f, 0.65f, 0.80f, 0.72f, 0.58f, 0.45f};
        float spacing = b.getWidth() * 0.13f;
        float startX = cx - 3.0f * spacing;
        float pipeW = b.getWidth() * 0.09f;
        for (int i = 0; i < 6; ++i)
        {
            float px = startX + (float)i * spacing;
            float ph = b.getHeight() * heights[i] * breathScale;
            g.setColour(accent.withAlpha(0.55f + (float)i * 0.06f));
            g.fillRect(px - pipeW * 0.5f, baseY - ph, pipeW, ph);
            g.setColour(accent.withAlpha(0.88f));
            g.drawRect(px - pipeW * 0.5f, baseY - ph, pipeW, ph, 1.0f);
        }
    }

    //--------------------------------------------------------------------------
    // OBSCURA — camera obscura: box with pinhole and diverging rays
    static void drawObscura(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float bW = b.getWidth() * 0.45f;
        float bH = b.getHeight() * 0.35f;
        g.setColour(accent.withAlpha(0.60f));
        g.drawRect(cx - bW * 0.5f, cy - bH * 0.5f, bW, bH, 1.5f);
        g.setColour(accent.withAlpha(0.90f));
        g.fillEllipse(cx - 2.0f, cy - 2.0f, 4.0f, 4.0f);
        float rayLen = b.getWidth() * 0.22f * breathScale;
        g.setColour(accent.withAlpha(0.38f));
        for (int r = -1; r <= 1; r += 2)
        {
            g.drawLine(cx, cy, cx + (float)r * rayLen, cy - bH * 0.65f, 1.0f);
            g.drawLine(cx, cy, cx + (float)r * rayLen * 0.5f, cy + bH * 0.65f, 1.0f);
        }
    }

    //--------------------------------------------------------------------------
    // OCEANIC — rolling wave layers
    static void drawOceanic(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float w = b.getWidth() * 0.80f;
        float amp = b.getHeight() * 0.18f * breathScale;
        float startX = cx - w * 0.5f;
        for (int layer = 0; layer < 2; ++layer)
        {
            float ly = cy + (float)layer * b.getHeight() * 0.14f;
            g.setColour(accent.withAlpha(0.70f - (float)layer * 0.25f));
            juce::Path wave;
            wave.startNewSubPath(startX, ly);
            for (int p = 1; p <= 12; ++p)
            {
                float t = (float)p / 12.0f;
                wave.lineTo(startX + t * w,
                            ly - amp * std::sin(t * juce::MathConstants<float>::twoPi + (float)layer));
            }
            wave.lineTo(startX + w, ly + amp);
            wave.lineTo(startX, ly + amp);
            wave.closeSubPath();
            g.fillPath(wave);
        }
    }

    //--------------------------------------------------------------------------
    // OPTIC — eye with layered iris rings
    static void drawOptic(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                          float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float outerR = b.getWidth() * 0.30f * breathScale;
        const float rings[] = {1.0f, 0.65f, 0.38f};
        const float alphas[] = {0.40f, 0.65f, 0.85f};
        for (int r = 0; r < 3; ++r)
        {
            float rr = outerR * rings[r];
            g.setColour(accent.withAlpha(alphas[r]));
            g.drawEllipse(cx - rr, cy - rr * 0.72f, rr * 2.0f, rr * 1.44f, 1.2f);
        }
        float pupilR = outerR * 0.16f;
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillEllipse(cx - pupilR, cy - pupilR, pupilR * 2.0f, pupilR * 2.0f);
    }

    //--------------------------------------------------------------------------
    // OBLIQUE — tilted parallel lines
    static void drawOblique(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX();
        float cy = b.getCentreY();
        float span = b.getWidth() * 0.55f;
        float tilt = b.getHeight() * 0.35f * breathScale;
        for (int i = 0; i < 5; ++i)
        {
            float offset = ((float)i / 4.0f - 0.5f) * span;
            g.setColour(accent.withAlpha(0.85f - std::abs(offset / span) * 0.40f));
            g.drawLine(cx + offset - span * 0.35f + couplingLean * 4.0f, cy + tilt * 0.5f,
                       cx + offset + span * 0.35f + couplingLean * 4.0f, cy - tilt * 0.5f,
                       1.0f + (float)(i == 2) * 0.5f);
        }
    }

    //--------------------------------------------------------------------------
    // OSPREY — bird in swept-wing dive posture
    static void drawOsprey(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.08f;
        float cy = b.getCentreY();
        float ws = b.getWidth() * 0.50f * breathScale;
        float bodyH = b.getHeight() * 0.28f;
        g.setColour(accent.withAlpha(0.70f));
        g.fillEllipse(cx - ws * 0.12f, cy - bodyH * 0.5f, ws * 0.24f, bodyH);
        g.setColour(accent.withAlpha(0.60f));
        juce::Path wings;
        wings.startNewSubPath(cx, cy - bodyH * 0.1f);
        wings.lineTo(cx - ws * 0.5f, cy + bodyH * 0.3f);
        wings.lineTo(cx - ws * 0.25f, cy + bodyH * 0.12f);
        wings.closeSubPath();
        wings.startNewSubPath(cx, cy - bodyH * 0.1f);
        wings.lineTo(cx + ws * 0.5f, cy + bodyH * 0.3f);
        wings.lineTo(cx + ws * 0.25f, cy + bodyH * 0.12f);
        wings.closeSubPath();
        g.fillPath(wings);
        g.setColour(accent.withAlpha(0.80f));
        g.drawLine(cx, cy + bodyH * 0.5f, cx, cy + bodyH * 0.9f, 1.5f);
    }

    //--------------------------------------------------------------------------
    // OSTERIA — wine goblet
    static void drawOsteria(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float gW = b.getWidth() * 0.36f * breathScale;
        float gH = b.getHeight() * 0.42f;
        float topY = cy - gH * 0.5f;
        float midY = cy + gH * 0.12f;
        float botY = cy + gH * 0.5f;
        g.setColour(accent.withAlpha(0.55f));
        juce::Path bowl;
        bowl.startNewSubPath(cx - gW * 0.5f, topY);
        bowl.quadraticTo(cx - gW * 0.55f, midY, cx, midY + gH * 0.08f);
        bowl.quadraticTo(cx + gW * 0.55f, midY, cx + gW * 0.5f, topY);
        g.strokePath(bowl, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                     juce::PathStrokeType::rounded));
        g.setColour(accent.withAlpha(0.70f));
        g.drawLine(cx - gW * 0.5f, topY, cx + gW * 0.5f, topY, 1.5f);
        g.drawLine(cx, midY + gH * 0.08f, cx, botY - gH * 0.06f, 1.5f);
        g.drawLine(cx - gW * 0.35f, botY, cx + gW * 0.35f, botY, 1.5f);
    }

    //--------------------------------------------------------------------------
    // OHM — resistor zig-zag
    static void drawOhm(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                        float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float w = b.getWidth() * 0.56f * breathScale;
        float amp = b.getHeight() * 0.18f;
        float startX = cx - w * 0.5f;
        g.setColour(accent.withAlpha(0.85f));
        g.drawLine(startX - b.getWidth() * 0.10f, cy, startX, cy, 1.5f);
        g.drawLine(startX + w, cy, startX + w + b.getWidth() * 0.10f, cy, 1.5f);
        juce::Path zz;
        zz.startNewSubPath(startX, cy);
        for (int p = 0; p < 5; ++p)
        {
            float t0 = (float)p / 5.0f;
            float midT = t0 + 0.1f / 5.0f;
            zz.lineTo(startX + (t0 + 0.05f / 5.0f) * w * 5.0f,
                      cy - amp * ((p % 2 == 0) ? 1.0f : -1.0f));
            (void)midT;
            zz.lineTo(startX + ((float)(p + 1) / 5.0f) * w, cy);
        }
        g.strokePath(zz, juce::PathStrokeType(1.5f, juce::PathStrokeType::mitered,
                     juce::PathStrokeType::butt));
    }

    //--------------------------------------------------------------------------
    // ORPHICA — ancient lyre with strings
    static void drawOrphica(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float lW = b.getWidth() * 0.36f * breathScale;
        float lH = b.getHeight() * 0.52f;
        float topY = cy - lH * 0.5f;
        float botY = cy + lH * 0.5f;
        g.setColour(accent.withAlpha(0.80f));
        juce::Path arms;
        arms.startNewSubPath(cx, botY);
        arms.quadraticTo(cx - lW * 0.6f, cy, cx - lW * 0.5f, topY);
        arms.startNewSubPath(cx, botY);
        arms.quadraticTo(cx + lW * 0.6f, cy, cx + lW * 0.5f, topY);
        g.strokePath(arms, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                     juce::PathStrokeType::rounded));
        g.drawLine(cx - lW * 0.5f, topY, cx + lW * 0.5f, topY, 1.5f);
        g.setColour(accent.withAlpha(0.50f));
        for (int s = 0; s < 5; ++s)
        {
            float t = (float)s / 4.0f;
            float sx = cx - lW * 0.38f + t * lW * 0.76f;
            float strLen = lH * (0.58f + 0.14f * std::sin(t * juce::MathConstants<float>::pi));
            g.drawLine(sx, topY, sx, topY + strLen, 0.8f);
        }
    }

    //--------------------------------------------------------------------------
    // OBBLIGATO — beamed double notes (mandatory countermelody)
    static void drawObbligato(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                              float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float noteR = b.getWidth() * 0.09f * breathScale;
        float stemH = b.getHeight() * 0.38f;
        g.setColour(accent.withAlpha(0.80f));
        for (int n = 0; n < 2; ++n)
        {
            float nx = cx + (float)(n * 2 - 1) * b.getWidth() * 0.16f;
            float ny = cy + (float)(n * 2 - 1) * b.getHeight() * 0.08f;
            g.fillEllipse(nx - noteR, ny - noteR * 0.65f, noteR * 2.0f, noteR * 1.3f);
            g.drawLine(nx + noteR * 0.9f, ny, nx + noteR * 0.9f, ny - stemH, 1.5f);
        }
        g.setColour(accent.withAlpha(0.65f));
        float beamY = cy - stemH * 0.75f;
        g.fillRect(cx - b.getWidth() * 0.14f, beamY - 2.0f, b.getWidth() * 0.28f, 3.5f);
    }

    //--------------------------------------------------------------------------
    // OTTONI — coiled brass horn
    static void drawOttoni(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float r = b.getWidth() * 0.24f * breathScale;
        g.setColour(accent.withAlpha(0.80f));
        for (int loop = 0; loop < 3; ++loop)
        {
            float lr = r * (1.0f - (float)loop * 0.22f);
            juce::Path arc;
            arc.addCentredArc(cx, cy, lr, lr * 0.75f, 0.0f,
                              juce::MathConstants<float>::pi * 0.2f,
                              juce::MathConstants<float>::pi * 1.9f, true);
            g.strokePath(arc, juce::PathStrokeType(1.2f + (float)loop * 0.3f,
                         juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.drawLine(cx + r, cy - r * 0.35f, cx + r * 1.4f, cy - r * 0.55f, 2.0f);
        g.drawLine(cx + r, cy + r * 0.35f, cx + r * 1.4f, cy + r * 0.55f, 2.0f);
    }

    //--------------------------------------------------------------------------
    // OLE — bullfighter cape swirl
    static void drawOle(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                        float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.08f;
        float cy = b.getCentreY();
        float r = b.getWidth() * 0.30f * breathScale;
        g.setColour(accent.withAlpha(0.65f));
        juce::Path cape;
        cape.startNewSubPath(cx - r * 0.1f, cy - r * 0.6f);
        cape.quadraticTo(cx + r * 0.8f, cy - r * 0.2f, cx + r * 0.4f, cy + r * 0.5f);
        cape.quadraticTo(cx - r * 0.3f, cy + r * 0.9f, cx - r * 0.7f, cy + r * 0.3f);
        cape.quadraticTo(cx - r * 1.0f, cy - r * 0.3f, cx - r * 0.1f, cy - r * 0.6f);
        cape.closeSubPath();
        g.fillPath(cape);
        g.setColour(accent.withAlpha(0.90f));
        g.strokePath(cape, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved,
                     juce::PathStrokeType::rounded));
    }

    //--------------------------------------------------------------------------
    // OMBRE — vertically fading dot gradient
    static void drawOmbre(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                          float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.04f;
        float topY = b.getY() + b.getHeight() * 0.12f;
        float span = b.getHeight() * 0.76f;
        float dotR = b.getWidth() * 0.06f * breathScale;
        for (int row = 0; row < 7; ++row)
        {
            float t = (float)row / 6.0f;
            float ry = topY + t * span;
            float rw = dotR * (1.0f - t * 0.35f);
            g.setColour(accent.withAlpha(0.90f - t * 0.65f));
            g.fillEllipse(cx - rw, ry - rw, rw * 2.0f, rw * 2.0f);
        }
    }

    //--------------------------------------------------------------------------
    // OCTOPUS — central body with 8 curling tentacles
    static void drawOctopus(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY() - b.getHeight() * 0.06f;
        float headR = b.getWidth() * 0.18f * breathScale;
        g.setColour(accent.withAlpha(0.70f));
        g.fillEllipse(cx - headR, cy - headR * 1.2f, headR * 2.0f, headR * 2.4f);
        g.setColour(accent.withAlpha(0.55f));
        for (int t = 0; t < 8; ++t)
        {
            float baseAngle = juce::MathConstants<float>::halfPi
                              + (float)t * juce::MathConstants<float>::pi * 0.25f;
            float startX = cx + std::cos(baseAngle) * headR * 0.8f;
            float startY = cy + std::sin(baseAngle) * headR * 0.8f;
            float len = b.getHeight() * 0.28f;
            float curl = (t % 2 == 0) ? 1.0f : -1.0f;
            juce::Path tent;
            tent.startNewSubPath(startX, startY);
            tent.quadraticTo(startX + std::cos(baseAngle + curl * 0.6f) * len * 0.6f,
                             startY + std::sin(baseAngle + curl * 0.6f) * len * 0.6f,
                             startX + std::cos(baseAngle + curl * 1.1f) * len,
                             startY + std::sin(baseAngle + curl * 1.1f) * len);
            g.strokePath(tent, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved,
                         juce::PathStrokeType::rounded));
        }
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillEllipse(cx - headR * 0.25f - 2.0f, cy - headR * 0.4f - 2.0f, 4.0f, 4.0f);
        g.fillEllipse(cx + headR * 0.25f - 2.0f, cy - headR * 0.4f - 2.0f, 4.0f, 4.0f);
    }

    //--------------------------------------------------------------------------
    // OPENSKY — horizon line with sun and rays
    static void drawOpensky(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float horizY = b.getCentreY() + b.getHeight() * 0.12f;
        float w = b.getWidth() * 0.72f;
        g.setColour(accent.withAlpha(0.65f));
        g.drawLine(cx - w * 0.5f, horizY, cx + w * 0.5f, horizY, 1.5f);
        float sunR = b.getWidth() * 0.14f * breathScale;
        float sunY = horizY - b.getHeight() * 0.25f;
        g.setColour(accent.withAlpha(0.80f));
        g.drawEllipse(cx - sunR, sunY - sunR, sunR * 2.0f, sunR * 2.0f, 1.5f);
        g.setColour(accent.withAlpha(0.45f));
        for (int r = 0; r < 6; ++r)
        {
            float angle = (float)r * juce::MathConstants<float>::pi / 3.0f;
            float r0 = sunR * 1.35f, r1 = sunR * 1.80f;
            g.drawLine(cx + r0 * std::cos(angle), sunY + r0 * std::sin(angle),
                       cx + r1 * std::cos(angle), sunY + r1 * std::sin(angle), 1.0f);
        }
    }

    //--------------------------------------------------------------------------
    // OCEANDEEP — abyssal depth with concentric rings narrowing to abyss point
    static void drawOceandeep(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                              float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.04f;
        float cy = b.getCentreY();
        for (int r = 0; r < 5; ++r)
        {
            float t = (float)r / 4.0f;
            float rW = b.getWidth() * (0.65f - t * 0.55f) * breathScale;
            float rH = b.getHeight() * (0.52f - t * 0.45f);
            float ry = cy + t * b.getHeight() * 0.18f;
            g.setColour(accent.withAlpha(0.30f + t * 0.55f));
            g.drawEllipse(cx - rW * 0.5f, ry - rH * 0.5f, rW, rH, 1.0f);
        }
        g.setColour(accent.withAlpha(0.90f));
        g.fillEllipse(cx - 2.5f, cy + b.getHeight() * 0.22f - 2.5f, 5.0f, 5.0f);
    }

    //--------------------------------------------------------------------------
    // OUIE — ear profile (ouïe = hearing in French)
    static void drawOuie(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                         float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float eW = b.getWidth() * 0.28f * breathScale;
        float eH = b.getHeight() * 0.55f;
        g.setColour(accent.withAlpha(0.75f));
        juce::Path ear;
        ear.startNewSubPath(cx + eW * 0.2f, cy - eH * 0.5f);
        ear.quadraticTo(cx - eW * 0.8f, cy - eH * 0.5f, cx - eW * 0.8f, cy);
        ear.quadraticTo(cx - eW * 0.8f, cy + eH * 0.5f, cx + eW * 0.2f, cy + eH * 0.5f);
        ear.quadraticTo(cx + eW * 0.5f, cy + eH * 0.35f, cx + eW * 0.4f, cy);
        ear.quadraticTo(cx + eW * 0.5f, cy - eH * 0.35f, cx + eW * 0.2f, cy - eH * 0.5f);
        g.strokePath(ear, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                     juce::PathStrokeType::rounded));
        g.setColour(accent.withAlpha(0.45f));
        juce::Path canal;
        canal.startNewSubPath(cx + eW * 0.1f, cy - eH * 0.25f);
        canal.quadraticTo(cx - eW * 0.30f, cy, cx + eW * 0.1f, cy + eH * 0.25f);
        g.strokePath(canal, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved,
                     juce::PathStrokeType::rounded));
    }

    //--------------------------------------------------------------------------
    // OVERLAP — two partially overlapping circles
    static void drawOverlap(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float r = b.getWidth() * 0.22f * breathScale;
        float off = r * 0.70f;
        g.setColour(accent.withAlpha(0.45f));
        g.fillEllipse(cx - off - r, cy - r, r * 2.0f, r * 2.0f);
        g.fillEllipse(cx + off - r, cy - r, r * 2.0f, r * 2.0f);
        g.setColour(accent.withAlpha(0.80f));
        g.drawEllipse(cx - off - r, cy - r, r * 2.0f, r * 2.0f, 1.2f);
        g.drawEllipse(cx + off - r, cy - r, r * 2.0f, r * 2.0f, 1.2f);
    }

    //--------------------------------------------------------------------------
    // OUTWIT — fox: pointed ears, angular face
    static void drawOutwit(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.07f;
        float cy = b.getCentreY();
        float bW = b.getWidth() * 0.40f * breathScale;
        float bH = b.getHeight() * 0.42f;
        g.setColour(accent.withAlpha(0.70f));
        g.fillEllipse(cx - bW * 0.5f, cy - bH * 0.35f, bW, bH * 0.70f);
        g.setColour(accent.withAlpha(0.85f));
        juce::Path ears;
        for (int e = -1; e <= 1; e += 2)
        {
            float ex = cx + (float)e * bW * 0.28f;
            float earBase = cy - bH * 0.28f;
            ears.startNewSubPath(ex - bW * 0.12f, earBase);
            ears.lineTo(ex, earBase - bH * 0.30f);
            ears.lineTo(ex + bW * 0.12f, earBase);
            ears.closeSubPath();
        }
        g.fillPath(ears);
        g.setColour(accent.withAlpha(0.80f));
        g.fillRect(cx - bW * 0.12f, cy + bH * 0.05f, bW * 0.24f, bH * 0.18f);
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillEllipse(cx - bW * 0.20f - 2.0f, cy - bH * 0.05f - 2.0f, 4.0f, 4.0f);
        g.fillEllipse(cx + bW * 0.20f - 2.0f, cy - bH * 0.05f - 2.0f, 4.0f, 4.0f);
    }

    //--------------------------------------------------------------------------
    // ORBWEAVE — spider web: radial spokes + concentric rings
    static void drawOrbweave(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.04f;
        float cy = b.getCentreY();
        float maxR = b.getWidth() * 0.36f * breathScale;
        g.setColour(accent.withAlpha(0.55f));
        for (int s = 0; s < 8; ++s)
        {
            float angle = (float)s * juce::MathConstants<float>::twoPi / 8.0f;
            g.drawLine(cx, cy, cx + maxR * std::cos(angle), cy + maxR * std::sin(angle), 0.8f);
        }
        for (int r = 1; r <= 4; ++r)
        {
            float rr = maxR * (float)r / 4.0f;
            g.setColour(accent.withAlpha(0.40f + (float)r * 0.10f));
            g.drawEllipse(cx - rr, cy - rr, rr * 2.0f, rr * 2.0f, 0.8f);
        }
        g.setColour(accent.withAlpha(0.90f));
        g.fillEllipse(cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
    }

    //--------------------------------------------------------------------------
    // OVERTONE — harmonic stack: diminishing bars representing partials
    static void drawOvertone(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float baseY = b.getCentreY() + b.getHeight() * 0.30f;
        float barW = b.getWidth() * 0.07f;
        float spacing = b.getWidth() * 0.11f;
        float startX = cx - 3.5f * spacing;
        for (int p = 0; p < 7; ++p)
        {
            float partial = (float)(p + 1);
            float bH = b.getHeight() * 0.55f * breathScale / partial;
            float px = startX + (float)p * spacing;
            float alpha = 0.85f - (float)p * 0.09f;
            g.setColour(accent.withAlpha(juce::jmax(0.20f, alpha)));
            g.fillRect(px - barW * 0.5f, baseY - bH, barW, bH);
        }
    }

    //--------------------------------------------------------------------------
    // OFFERING — bowl with rising wisps
    static void drawOffering(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY() + b.getHeight() * 0.10f;
        float bW = b.getWidth() * 0.48f * breathScale;
        float bH = b.getHeight() * 0.22f;
        g.setColour(accent.withAlpha(0.65f));
        juce::Path bowl;
        bowl.startNewSubPath(cx - bW * 0.5f, cy - bH * 0.5f);
        bowl.quadraticTo(cx - bW * 0.52f, cy + bH * 0.5f, cx, cy + bH * 0.6f);
        bowl.quadraticTo(cx + bW * 0.52f, cy + bH * 0.5f, cx + bW * 0.5f, cy - bH * 0.5f);
        g.strokePath(bowl, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                     juce::PathStrokeType::rounded));
        g.drawLine(cx - bW * 0.5f, cy - bH * 0.5f, cx + bW * 0.5f, cy - bH * 0.5f, 1.5f);
        // Rising wisps
        g.setColour(accent.withAlpha(0.40f));
        for (int w = -1; w <= 1; ++w)
        {
            float wx = cx + (float)w * bW * 0.22f;
            float wispLen = b.getHeight() * 0.28f * breathScale;
            juce::Path wisp;
            wisp.startNewSubPath(wx, cy - bH * 0.5f);
            wisp.quadraticTo(wx + (float)w * 4.0f, cy - bH * 0.5f - wispLen * 0.5f,
                             wx - (float)w * 3.0f, cy - bH * 0.5f - wispLen);
            g.strokePath(wisp, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved,
                         juce::PathStrokeType::rounded));
        }
    }

    //--------------------------------------------------------------------------
    // OSMOSIS — membrane (line) with dots diffusing through it
    static void drawOsmosis(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.04f;
        float cy = b.getCentreY();
        float mH = b.getHeight() * 0.60f;
        // Membrane
        g.setColour(accent.withAlpha(0.70f));
        g.drawLine(cx, cy - mH * 0.5f, cx, cy + mH * 0.5f, 2.0f);
        // Dots left side (large → small, moving right through membrane)
        const float leftX[] = {-0.35f, -0.22f, -0.10f};
        const float dotSizes[] = {5.0f, 3.5f, 2.0f};
        for (int d = 0; d < 3; ++d)
        {
            float dx = cx + leftX[d] * b.getWidth() * breathScale;
            float dy = cy + ((float)d - 1.0f) * mH * 0.28f;
            float ds = dotSizes[d];
            g.setColour(accent.withAlpha(0.75f - (float)d * 0.18f));
            g.fillEllipse(dx - ds * 0.5f, dy - ds * 0.5f, ds, ds);
        }
        // Dots right side (small → large, emerged)
        const float rightX[] = {0.10f, 0.22f, 0.35f};
        for (int d = 0; d < 3; ++d)
        {
            float dx = cx + rightX[d] * b.getWidth() * breathScale;
            float dy = cy + ((float)d - 1.0f) * mH * 0.28f;
            float ds = dotSizes[2 - d];
            g.setColour(accent.withAlpha(0.40f + (float)d * 0.18f));
            g.fillEllipse(dx - ds * 0.5f, dy - ds * 0.5f, ds, ds);
        }
    }

    //--------------------------------------------------------------------------
    // OGIVE — scanned synthesis: pointed arch / bullet nose
    static void drawOgive(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                          float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float bW = b.getWidth() * 0.38f * breathScale;
        float bH = b.getHeight() * 0.58f;
        g.setColour(accent.withAlpha(0.70f));
        juce::Path ogive;
        ogive.startNewSubPath(cx - bW * 0.5f, cy + bH * 0.5f);
        ogive.quadraticTo(cx - bW * 0.5f, cy - bH * 0.2f, cx, cy - bH * 0.5f);
        ogive.quadraticTo(cx + bW * 0.5f, cy - bH * 0.2f, cx + bW * 0.5f, cy + bH * 0.5f);
        ogive.closeSubPath();
        g.fillPath(ogive);
        g.setColour(accent.withAlpha(0.90f));
        g.strokePath(ogive, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved,
                     juce::PathStrokeType::rounded));
        // Scan lines
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.35f));
        for (int sl = 0; sl < 4; ++sl)
        {
            float sy = cy - bH * 0.35f + (float)sl * bH * 0.25f;
            float sw = bW * (0.30f + (float)sl * 0.12f);
            g.drawLine(cx - sw, sy, cx + sw, sy, 0.7f);
        }
    }

    //--------------------------------------------------------------------------
    // OLVIDO — spectral erosion ("The Forgetting Engine"): fading scattered dots
    static void drawOlvido(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        // Band of dots that fade left to right
        const int kDots = 12;
        float w = b.getWidth() * 0.70f;
        float h = b.getHeight() * 0.45f;
        for (int d = 0; d < kDots; ++d)
        {
            float t = (float)d / (float)(kDots - 1);
            float dx = cx - w * 0.5f + t * w;
            float dsize = (3.5f - t * 2.5f) * breathScale;
            float alpha = (1.0f - t) * 0.85f;
            // Slight scatter vertically
            float dy = cy + (((d * 7 + 3) % 5) - 2.0f) * h * 0.12f;
            if (dsize > 0.3f)
            {
                g.setColour(accent.withAlpha(alpha));
                g.fillEllipse(dx - dsize * 0.5f, dy - dsize * 0.5f, dsize, dsize);
            }
        }
    }

    //--------------------------------------------------------------------------
    // OSTRACON — corpus-buffer synthesis ("The Remembering Engine"): tape shard
    static void drawOstracon(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float sW = b.getWidth() * 0.52f * breathScale;
        float sH = b.getHeight() * 0.42f;
        // Irregular shard outline
        g.setColour(accent.withAlpha(0.60f));
        juce::Path shard;
        shard.startNewSubPath(cx - sW * 0.5f, cy - sH * 0.3f);
        shard.lineTo(cx - sW * 0.3f, cy - sH * 0.5f);
        shard.lineTo(cx + sW * 0.4f, cy - sH * 0.4f);
        shard.lineTo(cx + sW * 0.5f, cy + sH * 0.2f);
        shard.lineTo(cx + sW * 0.2f, cy + sH * 0.5f);
        shard.lineTo(cx - sW * 0.4f, cy + sH * 0.4f);
        shard.closeSubPath();
        g.fillPath(shard);
        g.setColour(accent.withAlpha(0.85f));
        g.strokePath(shard, juce::PathStrokeType(1.2f, juce::PathStrokeType::mitered,
                     juce::PathStrokeType::butt));
        // Oxide lines (tape tracks)
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.30f));
        for (int track = 0; track < 3; ++track)
        {
            float ty = cy - sH * 0.15f + (float)track * sH * 0.18f;
            g.drawLine(cx - sW * 0.28f, ty, cx + sW * 0.28f, ty, 0.7f);
        }
    }

    //--------------------------------------------------------------------------
    // ORRERY — nested planetary orbits
    static void drawOrrery(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        const float radii[] = {0.12f, 0.20f, 0.30f};
        const float alphas[] = {0.85f, 0.65f, 0.45f};
        float maxR = b.getWidth() * 0.32f * breathScale;
        g.setColour(accent.withAlpha(0.90f));
        g.fillEllipse(cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
        for (int o = 0; o < 3; ++o)
        {
            float or_ = maxR * (radii[o] / radii[2]);
            g.setColour(accent.withAlpha(alphas[o] * 0.5f));
            g.drawEllipse(cx - or_, cy - or_, or_ * 2.0f, or_ * 2.0f, 0.8f);
            float angle = (float)o * 2.2f + breathScale * 0.4f;
            float px = cx + or_ * std::cos(angle);
            float py = cy + or_ * std::sin(angle);
            float pr = 2.5f + (float)(2 - o) * 0.8f;
            g.setColour(accent.withAlpha(alphas[o]));
            g.fillEllipse(px - pr, py - pr, pr * 2.0f, pr * 2.0f);
        }
    }

    //--------------------------------------------------------------------------
    // OPSIN — retinal photoreceptor cell shape
    static void drawOpsin(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                          float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float rW = b.getWidth() * 0.22f * breathScale;
        float rH = b.getHeight() * 0.55f;
        // Outer segment (elongated tip)
        g.setColour(accent.withAlpha(0.75f));
        g.fillEllipse(cx - rW * 0.5f, cy - rH * 0.5f, rW, rH * 0.55f);
        // Inner segment (bulbous base)
        float innerW = rW * 1.4f;
        float innerH = rH * 0.35f;
        float innerY = cy - rH * 0.5f + rH * 0.55f;
        g.setColour(accent.withAlpha(0.55f));
        g.fillEllipse(cx - innerW * 0.5f, innerY, innerW, innerH);
        // Cilium
        g.setColour(accent.withAlpha(0.50f));
        g.drawLine(cx, innerY, cx, innerY + innerH * 1.1f, 1.0f);
        // Disc stacks
        g.setColour(accent.withAlpha(0.30f));
        for (int d = 0; d < 5; ++d)
        {
            float dy = cy - rH * 0.45f + (float)d * rH * 0.09f;
            g.drawLine(cx - rW * 0.38f, dy, cx + rW * 0.38f, dy, 0.7f);
        }
    }

    //--------------------------------------------------------------------------
    // OORT — distant comet cloud: scattered tiny dots in sparse halo
    static void drawOort(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                         float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.04f;
        float cy = b.getCentreY();
        float maxR = b.getWidth() * 0.38f * breathScale;
        // Central mass
        g.setColour(accent.withAlpha(0.80f));
        g.fillEllipse(cx - 3.5f, cy - 3.5f, 7.0f, 7.0f);
        // Sparse comet dots
        const float angles[] = {0.3f, 1.1f, 1.8f, 2.6f, 3.4f, 4.1f, 4.9f, 5.7f};
        const float dists[]  = {0.70f, 0.85f, 0.60f, 0.95f, 0.75f, 0.88f, 0.65f, 0.80f};
        for (int c = 0; c < 8; ++c)
        {
            float dist = maxR * dists[c];
            float px = cx + dist * std::cos(angles[c]);
            float py = cy + dist * std::sin(angles[c]);
            float alpha = 0.25f + dists[c] * 0.40f;
            g.setColour(accent.withAlpha(alpha));
            g.fillEllipse(px - 1.5f, py - 1.5f, 3.0f, 3.0f);
        }
    }

    //--------------------------------------------------------------------------
    // ONDINE — water nymph: sinuous flowing body
    static void drawOndine(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float amp = b.getWidth() * 0.22f * breathScale;
        float len = b.getHeight() * 0.65f;
        float topY = cy - len * 0.5f;
        g.setColour(accent.withAlpha(0.70f));
        juce::Path body;
        body.startNewSubPath(cx, topY);
        body.cubicTo(cx + amp, topY + len * 0.25f,
                     cx - amp, topY + len * 0.50f,
                     cx + amp * 0.5f, topY + len * 0.75f);
        body.cubicTo(cx + amp * 1.2f, topY + len * 0.85f,
                     cx - amp * 0.5f, topY + len * 0.95f,
                     cx, topY + len);
        g.strokePath(body, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved,
                     juce::PathStrokeType::rounded));
        // Head
        g.setColour(accent.withAlpha(0.80f));
        g.fillEllipse(cx - 4.0f, topY - 5.0f, 8.0f, 8.0f);
    }

    //--------------------------------------------------------------------------
    // ORTOLAN — small songbird on a perch
    static void drawOrtolan(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.07f;
        float cy = b.getCentreY();
        float bR = b.getWidth() * 0.15f * breathScale;
        // Body
        g.setColour(accent.withAlpha(0.70f));
        g.fillEllipse(cx - bR, cy - bR * 0.85f, bR * 2.0f, bR * 1.7f);
        // Head
        float headR = bR * 0.65f;
        g.setColour(accent.withAlpha(0.80f));
        g.fillEllipse(cx - headR, cy - bR * 0.85f - headR * 1.5f, headR * 2.0f, headR * 2.0f);
        // Beak
        g.setColour(accent.withAlpha(0.90f));
        g.drawLine(cx + headR * 0.9f, cy - bR * 0.85f - headR * 0.9f,
                   cx + headR * 1.8f, cy - bR * 0.85f - headR * 0.7f, 1.2f);
        // Perch
        g.setColour(accent.withAlpha(0.55f));
        g.drawLine(cx - b.getWidth() * 0.28f, cy + bR * 0.85f,
                   cx + b.getWidth() * 0.28f, cy + bR * 0.85f, 1.5f);
        // Legs
        g.drawLine(cx - bR * 0.3f, cy + bR * 0.8f, cx - bR * 0.3f, cy + bR * 0.85f, 1.2f);
        g.drawLine(cx + bR * 0.3f, cy + bR * 0.8f, cx + bR * 0.3f, cy + bR * 0.85f, 1.2f);
        // Eye
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.90f));
        g.fillEllipse(cx + headR * 0.35f - 1.5f, cy - bR * 0.85f - headR * 1.2f - 1.5f, 3.0f, 3.0f);
    }

    //--------------------------------------------------------------------------
    // OCTANT — navigation octant: 8-sector divided circle
    static void drawOctant(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float r = b.getWidth() * 0.30f * breathScale;
        g.setColour(accent.withAlpha(0.55f));
        g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 1.2f);
        g.setColour(accent.withAlpha(0.40f));
        for (int s = 0; s < 8; ++s)
        {
            float angle = (float)s * juce::MathConstants<float>::pi * 0.25f;
            g.drawLine(cx, cy, cx + r * std::cos(angle), cy + r * std::sin(angle), 0.8f);
        }
        // Compass rose dot at N
        g.setColour(accent.withAlpha(0.90f));
        g.fillEllipse(cx - 2.0f, cy - r - 2.0f, 4.0f, 4.0f);
        // Bearing arc
        g.setColour(accent.withAlpha(0.75f));
        juce::Path arc;
        arc.addCentredArc(cx, cy, r * 0.55f, r * 0.55f, 0.0f,
                          -juce::MathConstants<float>::halfPi,
                          -juce::MathConstants<float>::halfPi + breathScale * 1.8f, true);
        g.strokePath(arc, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                     juce::PathStrokeType::rounded));
    }

    //--------------------------------------------------------------------------
    // OVERTIDE — tidal wave with directional arrow
    static void drawOvertide(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float w = b.getWidth() * 0.72f;
        float amp = b.getHeight() * 0.20f * breathScale;
        float startX = cx - w * 0.5f;
        g.setColour(accent.withAlpha(0.70f));
        juce::Path wave;
        wave.startNewSubPath(startX, cy);
        for (int p = 1; p <= 10; ++p)
        {
            float t = (float)p / 10.0f;
            wave.lineTo(startX + t * w,
                        cy - amp * std::sin(t * juce::MathConstants<float>::pi * 2.0f));
        }
        wave.lineTo(startX + w, cy + amp * 0.5f);
        wave.lineTo(startX, cy + amp * 0.5f);
        wave.closeSubPath();
        g.fillPath(wave);
        // Arrow
        g.setColour(accent.withAlpha(0.90f));
        float arrowX = cx + w * 0.3f;
        float arrowY = cy - amp * 1.3f;
        g.drawLine(arrowX - b.getWidth() * 0.12f, arrowY, arrowX + b.getWidth() * 0.12f, arrowY, 1.5f);
        g.drawLine(arrowX + b.getWidth() * 0.06f, arrowY - 3.0f,
                   arrowX + b.getWidth() * 0.12f, arrowY, 1.5f);
        g.drawLine(arrowX + b.getWidth() * 0.06f, arrowY + 3.0f,
                   arrowX + b.getWidth() * 0.12f, arrowY, 1.5f);
    }

    //--------------------------------------------------------------------------
    // OOBLECK — non-Newtonian blob: irregular filled shape
    static void drawOobleck(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float r = b.getWidth() * 0.28f * breathScale;
        g.setColour(accent.withAlpha(0.65f));
        juce::Path blob;
        const float radii[] = {1.0f, 0.75f, 1.15f, 0.85f, 1.05f, 0.70f, 1.10f, 0.90f};
        const int kPts = 8;
        blob.startNewSubPath(cx + r * radii[0], cy);
        for (int p = 1; p <= kPts; ++p)
        {
            float angle = (float)p * juce::MathConstants<float>::twoPi / (float)kPts;
            float rr = r * radii[p % kPts];
            blob.lineTo(cx + rr * std::cos(angle), cy + rr * std::sin(angle));
        }
        blob.closeSubPath();
        g.fillPath(blob);
        g.setColour(accent.withAlpha(0.85f));
        g.strokePath(blob, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved,
                     juce::PathStrokeType::rounded));
    }

    //--------------------------------------------------------------------------
    // OOZE — slow drip / tear drop
    static void drawOoze(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                         float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY() - b.getHeight() * 0.06f;
        float dropR = b.getWidth() * 0.18f * breathScale;
        // Main drop
        g.setColour(accent.withAlpha(0.70f));
        juce::Path drop;
        drop.startNewSubPath(cx, cy - dropR * 1.4f);
        drop.quadraticTo(cx + dropR, cy - dropR * 0.3f, cx + dropR, cy + dropR * 0.5f);
        drop.quadraticTo(cx + dropR, cy + dropR * 1.2f, cx, cy + dropR * 1.5f);
        drop.quadraticTo(cx - dropR, cy + dropR * 1.2f, cx - dropR, cy + dropR * 0.5f);
        drop.quadraticTo(cx - dropR, cy - dropR * 0.3f, cx, cy - dropR * 1.4f);
        drop.closeSubPath();
        g.fillPath(drop);
        // Drip trail
        g.setColour(accent.withAlpha(0.35f));
        for (int d = 1; d <= 3; ++d)
        {
            float dy = cy + dropR * 1.5f + (float)d * b.getHeight() * 0.08f;
            float ds = dropR * 0.25f * (1.0f - (float)d * 0.25f);
            if (ds > 0.5f)
                g.fillEllipse(cx - ds, dy - ds, ds * 2.0f, ds * 2.0f);
        }
    }

    //--------------------------------------------------------------------------
    // OUTCROP — rock formation: jagged peaks
    static void drawOutcrop(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float baseY = b.getCentreY() + b.getHeight() * 0.28f;
        float w = b.getWidth() * 0.68f * breathScale;
        g.setColour(accent.withAlpha(0.70f));
        juce::Path rocks;
        rocks.startNewSubPath(cx - w * 0.5f, baseY);
        rocks.lineTo(cx - w * 0.38f, baseY - b.getHeight() * 0.20f);
        rocks.lineTo(cx - w * 0.18f, baseY - b.getHeight() * 0.12f);
        rocks.lineTo(cx - w * 0.05f, baseY - b.getHeight() * 0.38f);
        rocks.lineTo(cx + w * 0.10f, baseY - b.getHeight() * 0.15f);
        rocks.lineTo(cx + w * 0.28f, baseY - b.getHeight() * 0.28f);
        rocks.lineTo(cx + w * 0.50f, baseY - b.getHeight() * 0.08f);
        rocks.lineTo(cx + w * 0.5f, baseY);
        rocks.closeSubPath();
        g.fillPath(rocks);
        g.setColour(accent.withAlpha(0.85f));
        g.strokePath(rocks, juce::PathStrokeType(1.0f, juce::PathStrokeType::mitered,
                     juce::PathStrokeType::butt));
    }

    //--------------------------------------------------------------------------
    // OXIDIZE — rust/corrosion: irregular patch dots
    static void drawOxidize(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float r = b.getWidth() * 0.28f * breathScale;
        g.setColour(accent.withAlpha(0.50f));
        g.drawEllipse(cx - r, cy - r * 0.85f, r * 2.0f, r * 1.7f, 1.5f);
        // Rust spots
        const float spotX[] = {-0.25f, 0.10f, -0.40f, 0.35f, 0.0f, -0.15f, 0.42f};
        const float spotY[] = {-0.20f, 0.30f,  0.10f, -0.15f, 0.0f, 0.40f, 0.25f};
        const float spotR[] = {0.12f, 0.08f, 0.10f, 0.07f, 0.14f, 0.06f, 0.09f};
        for (int s = 0; s < 7; ++s)
        {
            float sx = cx + spotX[s] * r * 1.8f;
            float sy = cy + spotY[s] * r * 1.5f;
            float sr = spotR[s] * r;
            float alpha = 0.50f + (float)s * 0.04f;
            g.setColour(accent.withAlpha(juce::jmin(0.80f, alpha)));
            g.fillEllipse(sx - sr, sy - sr, sr * 2.0f, sr * 2.0f);
        }
    }

    //--------------------------------------------------------------------------
    // ONRUSH — rushing arrows
    static void drawOnrush(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float arrowLen = b.getWidth() * 0.28f * breathScale;
        float arrowH = b.getHeight() * 0.12f;
        const float yOffsets[] = {-0.22f, 0.0f, 0.22f};
        for (int a = 0; a < 3; ++a)
        {
            float ay = cy + yOffsets[a] * b.getHeight();
            float alpha = (a == 1) ? 0.85f : 0.55f;
            float xOff = (float)a * arrowLen * 0.20f;
            float startX = cx - arrowLen * 0.5f + xOff;
            g.setColour(accent.withAlpha(alpha));
            g.drawLine(startX, ay, startX + arrowLen, ay, 1.5f);
            g.drawLine(startX + arrowLen - arrowH * 0.7f, ay - arrowH * 0.5f,
                       startX + arrowLen, ay, 1.5f);
            g.drawLine(startX + arrowLen - arrowH * 0.7f, ay + arrowH * 0.5f,
                       startX + arrowLen, ay, 1.5f);
        }
    }

    //--------------------------------------------------------------------------
    // OMNISTEREO — stereo field: L/R circles with shared centre overlap
    static void drawOmnistereo(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                               float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.04f;
        float cy = b.getCentreY();
        float r = b.getWidth() * 0.20f * breathScale;
        float off = r * 1.10f;
        g.setColour(accent.withAlpha(0.35f));
        g.fillEllipse(cx - off - r, cy - r, r * 2.0f, r * 2.0f);
        g.fillEllipse(cx + off - r, cy - r, r * 2.0f, r * 2.0f);
        g.setColour(accent.withAlpha(0.70f));
        g.drawEllipse(cx - off - r, cy - r, r * 2.0f, r * 2.0f, 1.2f);
        g.drawEllipse(cx + off - r, cy - r, r * 2.0f, r * 2.0f, 1.2f);
        g.setColour(accent.withAlpha(0.55f));
        g.drawLine(cx - off, cy - r * 0.4f, cx - off, cy + r * 0.4f, 0.8f);
        g.drawLine(cx + off, cy - r * 0.4f, cx + off, cy + r * 0.4f, 0.8f);
    }

    //--------------------------------------------------------------------------
    // OBLITERATE — scattered shards/fragments
    static void drawObliterate(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                               float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.04f;
        float cy = b.getCentreY();
        float spread = b.getWidth() * 0.38f * breathScale;
        const float shardX[] = {-0.5f, -0.2f, 0.1f, 0.45f, -0.35f, 0.3f, -0.05f};
        const float shardY[] = {-0.3f, -0.5f, -0.4f, -0.1f, 0.2f, 0.35f, 0.5f};
        const float shardA[] = {0.3f, 0.8f, 0.5f, 1.1f, 0.6f, 0.9f, 0.4f};
        for (int s = 0; s < 7; ++s)
        {
            float sx = cx + shardX[s] * spread;
            float sy = cy + shardY[s] * spread;
            float sa = shardA[s];
            float size = spread * 0.14f;
            g.setColour(accent.withAlpha(0.55f + (float)s * 0.04f));
            juce::Path shard;
            shard.startNewSubPath(sx + size * std::cos(sa), sy + size * std::sin(sa));
            shard.lineTo(sx + size * std::cos(sa + 2.2f), sy + size * std::sin(sa + 2.2f));
            shard.lineTo(sx + size * std::cos(sa + 4.0f), sy + size * std::sin(sa + 4.0f));
            shard.closeSubPath();
            g.fillPath(shard);
        }
    }

    //--------------------------------------------------------------------------
    // OBSCURITY — dark veil / shroud
    static void drawObscurity(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                              float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float vW = b.getWidth() * 0.50f * breathScale;
        float vH = b.getHeight() * 0.55f;
        g.setColour(accent.withAlpha(0.25f));
        g.fillEllipse(cx - vW * 0.5f, cy - vH * 0.5f, vW, vH);
        // Shroud drape lines
        g.setColour(accent.withAlpha(0.55f));
        for (int l = -2; l <= 2; ++l)
        {
            float lx = cx + (float)l * vW * 0.18f;
            float topY = cy - vH * 0.45f + std::abs((float)l) * vH * 0.06f;
            float botY = cy + vH * 0.5f;
            g.drawLine(lx, topY, lx + (float)l * vW * 0.08f, botY, 0.9f);
        }
        g.setColour(accent.withAlpha(0.70f));
        g.drawEllipse(cx - vW * 0.5f, cy - vH * 0.5f, vW, vH * 0.25f, 1.2f);
    }

    //--------------------------------------------------------------------------
    // OUBLIETTE — dungeon pit: downward funnel
    static void drawOubliette(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                              float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float topW = b.getWidth() * 0.52f * breathScale;
        float botW = b.getWidth() * 0.12f * breathScale;
        float fH = b.getHeight() * 0.55f;
        g.setColour(accent.withAlpha(0.60f));
        juce::Path funnel;
        funnel.startNewSubPath(cx - topW * 0.5f, cy - fH * 0.5f);
        funnel.lineTo(cx - botW * 0.5f, cy + fH * 0.5f);
        funnel.lineTo(cx + botW * 0.5f, cy + fH * 0.5f);
        funnel.lineTo(cx + topW * 0.5f, cy - fH * 0.5f);
        funnel.closeSubPath();
        g.fillPath(funnel);
        g.setColour(accent.withAlpha(0.85f));
        g.strokePath(funnel, juce::PathStrokeType(1.2f, juce::PathStrokeType::mitered,
                     juce::PathStrokeType::butt));
        // Dark pit
        g.setColour(accent.withAlpha(0.90f));
        g.fillEllipse(cx - botW * 0.5f, cy + fH * 0.4f, botW, botW * 0.5f);
    }

    //--------------------------------------------------------------------------
    // OSMIUM — dense heavy element: tight concentric rings
    static void drawOsmium(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.04f;
        float cy = b.getCentreY();
        float coreR = b.getWidth() * 0.10f * breathScale;
        g.setColour(accent.withAlpha(0.90f));
        g.fillEllipse(cx - coreR, cy - coreR, coreR * 2.0f, coreR * 2.0f);
        for (int r = 1; r <= 4; ++r)
        {
            float rr = coreR + (float)r * coreR * 0.70f;
            g.setColour(accent.withAlpha(0.75f - (float)r * 0.12f));
            g.drawEllipse(cx - rr, cy - rr, rr * 2.0f, rr * 2.0f, 1.5f - (float)r * 0.25f);
        }
    }

    //--------------------------------------------------------------------------
    // OROGEN — mountain formation with peaks
    static void drawOrogen(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float baseY = b.getCentreY() + b.getHeight() * 0.25f;
        float w = b.getWidth() * 0.72f * breathScale;
        g.setColour(accent.withAlpha(0.70f));
        juce::Path mountains;
        mountains.startNewSubPath(cx - w * 0.5f, baseY);
        mountains.lineTo(cx - w * 0.28f, baseY - b.getHeight() * 0.30f);
        mountains.lineTo(cx - w * 0.06f, baseY - b.getHeight() * 0.15f);
        mountains.lineTo(cx + w * 0.08f, baseY - b.getHeight() * 0.48f);
        mountains.lineTo(cx + w * 0.22f, baseY - b.getHeight() * 0.20f);
        mountains.lineTo(cx + w * 0.50f, baseY - b.getHeight() * 0.32f);
        mountains.lineTo(cx + w * 0.5f, baseY);
        mountains.closeSubPath();
        g.fillPath(mountains);
        g.setColour(accent.withAlpha(0.88f));
        g.strokePath(mountains, juce::PathStrokeType(1.0f, juce::PathStrokeType::mitered,
                     juce::PathStrokeType::butt));
    }

    //--------------------------------------------------------------------------
    // OCULUS — open oculus circle (Pantheon window)
    static void drawOculus(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float outerR = b.getWidth() * 0.30f * breathScale;
        float innerR = outerR * 0.60f;
        g.setColour(accent.withAlpha(0.55f));
        juce::Path ring;
        ring.addEllipse(cx - outerR, cy - outerR, outerR * 2.0f, outerR * 2.0f);
        ring.addEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);
        ring.setUsingNonZeroWinding(false);
        g.fillPath(ring);
        g.setColour(accent.withAlpha(0.90f));
        g.drawEllipse(cx - outerR, cy - outerR, outerR * 2.0f, outerR * 2.0f, 1.5f);
        g.drawEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f, 1.0f);
    }

    //--------------------------------------------------------------------------
    // OUTAGE — interrupted connection
    static void drawOutage(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float w = b.getWidth() * 0.68f * breathScale;
        float gap = w * 0.18f;
        g.setColour(accent.withAlpha(0.70f));
        g.drawLine(cx - w * 0.5f, cy, cx - gap * 0.5f, cy, 2.0f);
        g.drawLine(cx + gap * 0.5f, cy, cx + w * 0.5f, cy, 2.0f);
        // Break markers
        g.setColour(accent.withAlpha(0.85f));
        float sparks = gap * 0.3f;
        g.drawLine(cx - gap * 0.5f, cy, cx - gap * 0.5f - sparks, cy - sparks * 1.5f, 1.2f);
        g.drawLine(cx - gap * 0.5f, cy, cx - gap * 0.5f - sparks * 0.5f, cy + sparks * 1.2f, 1.2f);
        g.drawLine(cx + gap * 0.5f, cy, cx + gap * 0.5f + sparks, cy - sparks * 1.5f, 1.2f);
        g.drawLine(cx + gap * 0.5f, cy, cx + gap * 0.5f + sparks * 0.5f, cy + sparks * 1.2f, 1.2f);
    }

    //--------------------------------------------------------------------------
    // OVERRIDE — toggle lever with override arrow
    static void drawOverride(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float sw = b.getWidth() * 0.46f * breathScale;
        float sh = b.getHeight() * 0.18f;
        // Slot
        g.setColour(accent.withAlpha(0.45f));
        g.drawRoundedRectangle(cx - sw * 0.5f, cy - sh * 0.5f, sw, sh, sh * 0.5f, 1.5f);
        // Thumb (overridden position — far right)
        float thumbX = cx + sw * 0.25f;
        g.setColour(accent.withAlpha(0.85f));
        g.fillEllipse(thumbX - sh * 0.55f, cy - sh * 0.55f, sh * 1.1f, sh * 1.1f);
        // Override down-arrow above slot
        float arrowCx = cx;
        float arrowY = cy - sh * 1.0f;
        g.setColour(accent.withAlpha(0.70f));
        g.drawLine(arrowCx, arrowY - b.getHeight() * 0.14f, arrowCx, arrowY, 1.5f);
        g.drawLine(arrowCx - 4.0f, arrowY - 4.0f, arrowCx, arrowY, 1.5f);
        g.drawLine(arrowCx + 4.0f, arrowY - 4.0f, arrowCx, arrowY, 1.5f);
    }

    //--------------------------------------------------------------------------
    // OCCLUSION — one circle partly blocking another
    static void drawOcclusion(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                              float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float r = b.getWidth() * 0.22f * breathScale;
        // Background circle (occluded)
        g.setColour(accent.withAlpha(0.35f));
        g.fillEllipse(cx - r * 0.4f - r, cy - r, r * 2.0f, r * 2.0f);
        g.setColour(accent.withAlpha(0.60f));
        g.drawEllipse(cx - r * 0.4f - r, cy - r, r * 2.0f, r * 2.0f, 1.0f);
        // Foreground circle (occluder)
        g.setColour(accent.withAlpha(0.70f));
        g.fillEllipse(cx + r * 0.2f - r, cy - r, r * 2.0f, r * 2.0f);
        g.setColour(accent.withAlpha(0.90f));
        g.drawEllipse(cx + r * 0.2f - r, cy - r, r * 2.0f, r * 2.0f, 1.2f);
    }

    //--------------------------------------------------------------------------
    // OBDURATE — hard stone diamond / cube
    static void drawObdurate(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float s = b.getWidth() * 0.26f * breathScale;
        g.setColour(accent.withAlpha(0.65f));
        juce::Path diamond;
        diamond.startNewSubPath(cx, cy - s);
        diamond.lineTo(cx + s, cy);
        diamond.lineTo(cx, cy + s);
        diamond.lineTo(cx - s, cy);
        diamond.closeSubPath();
        g.fillPath(diamond);
        g.setColour(accent.withAlpha(0.90f));
        g.strokePath(diamond, juce::PathStrokeType(1.5f, juce::PathStrokeType::mitered,
                     juce::PathStrokeType::butt));
        // Face lines
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.30f));
        g.drawLine(cx, cy - s, cx, cy + s, 0.7f);
        g.drawLine(cx - s, cy, cx + s, cy, 0.7f);
    }

    //--------------------------------------------------------------------------
    // ORISON — ascending beam / prayer
    static void drawOrison(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.04f;
        float cy = b.getCentreY();
        float beamH = b.getHeight() * 0.60f * breathScale;
        float beamW = b.getWidth() * 0.08f;
        // Beam column with fade
        for (int seg = 0; seg < 8; ++seg)
        {
            float t = (float)seg / 7.0f;
            float sy = cy + beamH * 0.5f - t * beamH;
            float sw = beamW * (1.0f - t * 0.6f);
            g.setColour(accent.withAlpha(t * 0.70f));
            g.fillRect(cx - sw * 0.5f, sy - beamH / 16.0f, sw, beamH / 8.0f);
        }
        // Base glow
        g.setColour(accent.withAlpha(0.40f));
        g.fillEllipse(cx - beamW * 1.5f, cy + beamH * 0.4f, beamW * 3.0f, beamW * 1.2f);
    }

    //--------------------------------------------------------------------------
    // OVERSHOOT — arrow flying past a target mark
    static void drawOvershoot(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                              float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float w = b.getWidth() * 0.70f * breathScale;
        // Target line
        float targetX = cx + w * 0.15f;
        g.setColour(accent.withAlpha(0.55f));
        g.drawLine(targetX, cy - b.getHeight() * 0.22f, targetX, cy + b.getHeight() * 0.22f, 1.5f);
        // Arrow (past the target)
        float arrowTip = cx + w * 0.5f;
        g.setColour(accent.withAlpha(0.85f));
        g.drawLine(cx - w * 0.5f, cy, arrowTip, cy, 1.5f);
        g.drawLine(arrowTip - 5.0f, cy - 4.0f, arrowTip, cy, 1.5f);
        g.drawLine(arrowTip - 5.0f, cy + 4.0f, arrowTip, cy, 1.5f);
    }

    //--------------------------------------------------------------------------
    // OBVERSE — coin face profile
    static void drawObverse(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float coinR = b.getWidth() * 0.28f * breathScale;
        g.setColour(accent.withAlpha(0.55f));
        g.fillEllipse(cx - coinR, cy - coinR, coinR * 2.0f, coinR * 2.0f);
        g.setColour(accent.withAlpha(0.85f));
        g.drawEllipse(cx - coinR, cy - coinR, coinR * 2.0f, coinR * 2.0f, 1.5f);
        // Profile (head silhouette — simple rounded bump)
        g.setColour(accent.withAlpha(0.70f));
        juce::Path profile;
        profile.startNewSubPath(cx - coinR * 0.1f, cy - coinR * 0.55f);
        profile.quadraticTo(cx + coinR * 0.5f, cy - coinR * 0.4f,
                            cx + coinR * 0.5f, cy + coinR * 0.35f);
        profile.quadraticTo(cx + coinR * 0.2f, cy + coinR * 0.55f,
                            cx - coinR * 0.1f, cy + coinR * 0.40f);
        g.strokePath(profile, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved,
                     juce::PathStrokeType::rounded));
    }

    //--------------------------------------------------------------------------
    // OXYMORON — two opposing arrows (contradiction symbol)
    static void drawOxymoron(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.04f;
        float cy = b.getCentreY();
        float len = b.getHeight() * 0.32f * breathScale;
        float hw = b.getWidth() * 0.20f;
        g.setColour(accent.withAlpha(0.80f));
        // Up arrow
        g.drawLine(cx, cy, cx, cy - len, 1.5f);
        g.drawLine(cx - hw * 0.4f, cy - len + 5.0f, cx, cy - len, 1.5f);
        g.drawLine(cx + hw * 0.4f, cy - len + 5.0f, cx, cy - len, 1.5f);
        // Down arrow (offset right)
        g.setColour(accent.withAlpha(0.55f));
        float ox = hw * 0.7f;
        g.drawLine(cx + ox, cy, cx + ox, cy + len, 1.5f);
        g.drawLine(cx + ox - hw * 0.4f, cy + len - 5.0f, cx + ox, cy + len, 1.5f);
        g.drawLine(cx + ox + hw * 0.4f, cy + len - 5.0f, cx + ox, cy + len, 1.5f);
    }

    //--------------------------------------------------------------------------
    // ORNATE — decorative filigree swirl
    static void drawOrnate(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float r = b.getWidth() * 0.26f * breathScale;
        g.setColour(accent.withAlpha(0.70f));
        juce::Path swirl;
        swirl.startNewSubPath(cx, cy);
        const int kSteps = 48;
        for (int s = 1; s <= kSteps; ++s)
        {
            float t = (float)s / (float)kSteps;
            float angle = t * juce::MathConstants<float>::twoPi * 2.5f;
            float rr = r * t;
            swirl.lineTo(cx + rr * std::cos(angle), cy + rr * std::sin(angle));
        }
        g.strokePath(swirl, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved,
                     juce::PathStrokeType::rounded));
        // Small flourish dots at tip and centre
        g.setColour(accent.withAlpha(0.85f));
        g.fillEllipse(cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
    }

    //--------------------------------------------------------------------------
    // ORATION — sound waves from a podium
    static void drawOration(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                            float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float podW = b.getWidth() * 0.16f;
        float podH = b.getHeight() * 0.20f;
        float podY = cy + b.getHeight() * 0.18f;
        g.setColour(accent.withAlpha(0.65f));
        g.fillRect(cx - podW * 0.5f, podY - podH, podW, podH);
        // Sound waves arcing above podium
        for (int w = 1; w <= 3; ++w)
        {
            float wr = b.getWidth() * 0.10f * (float)w * breathScale;
            float wa = 0.80f - (float)w * 0.18f;
            g.setColour(accent.withAlpha(wa));
            juce::Path arc;
            arc.addCentredArc(cx, podY - podH,
                              wr, wr * 0.70f, 0.0f,
                              -juce::MathConstants<float>::pi * 0.85f,
                              -juce::MathConstants<float>::pi * 0.15f, true);
            g.strokePath(arc, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved,
                         juce::PathStrokeType::rounded));
        }
    }

    //--------------------------------------------------------------------------
    // OFFCUT — wedge slice
    static void drawOffcut(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.06f;
        float cy = b.getCentreY();
        float s = b.getWidth() * 0.36f * breathScale;
        g.setColour(accent.withAlpha(0.65f));
        juce::Path wedge;
        wedge.startNewSubPath(cx - s * 0.5f, cy - s * 0.45f);
        wedge.lineTo(cx + s * 0.5f, cy - s * 0.45f);
        wedge.lineTo(cx + s * 0.5f, cy + s * 0.45f);
        wedge.lineTo(cx - s * 0.5f, cy - s * 0.45f);
        wedge.closeSubPath();
        g.fillPath(wedge);
        g.setColour(accent.withAlpha(0.88f));
        g.strokePath(wedge, juce::PathStrokeType(1.2f, juce::PathStrokeType::mitered,
                     juce::PathStrokeType::butt));
        // Cut line
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.50f));
        g.drawLine(cx - s * 0.5f, cy - s * 0.45f, cx + s * 0.5f, cy + s * 0.45f, 0.8f);
    }

    //--------------------------------------------------------------------------
    // OMEN — ominous eye in shadow
    static void drawOmen(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                         float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float outerR = b.getWidth() * 0.30f * breathScale;
        // Dark halo
        g.setColour(accent.withAlpha(0.15f));
        g.fillEllipse(cx - outerR, cy - outerR * 0.65f, outerR * 2.0f, outerR * 1.3f);
        // Eye whites
        g.setColour(accent.withAlpha(0.50f));
        juce::Path eye;
        eye.startNewSubPath(cx - outerR, cy);
        eye.quadraticTo(cx, cy - outerR * 0.45f, cx + outerR, cy);
        eye.quadraticTo(cx, cy + outerR * 0.45f, cx - outerR, cy);
        eye.closeSubPath();
        g.fillPath(eye);
        // Iris
        float irisR = outerR * 0.32f;
        g.setColour(accent.withAlpha(0.80f));
        g.fillEllipse(cx - irisR, cy - irisR, irisR * 2.0f, irisR * 2.0f);
        // Pupil
        float pupilR = irisR * 0.45f;
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.05f));
        g.fillEllipse(cx - pupilR, cy - pupilR, pupilR * 2.0f, pupilR * 2.0f);
    }

    //--------------------------------------------------------------------------
    // OPUS — music stave with notes
    static void drawOpus(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                         float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.04f;
        float cy = b.getCentreY();
        float w = b.getWidth() * 0.72f;
        float lineSpacing = b.getHeight() * 0.10f;
        float startX = cx - w * 0.5f;
        // 5 stave lines
        g.setColour(accent.withAlpha(0.40f));
        for (int l = 0; l < 5; ++l)
        {
            float ly = cy - lineSpacing * 2.0f + (float)l * lineSpacing;
            g.drawLine(startX, ly, startX + w, ly, 0.8f);
        }
        // Notes
        const float noteX[] = {0.12f, 0.38f, 0.62f, 0.82f};
        const float noteY[] = {0.0f, -1.0f, 0.5f, -0.5f};
        g.setColour(accent.withAlpha(0.85f));
        for (int n = 0; n < 4; ++n)
        {
            float nx = startX + noteX[n] * w;
            float ny = cy + noteY[n] * lineSpacing;
            float nr = lineSpacing * 0.38f * breathScale;
            g.fillEllipse(nx - nr, ny - nr * 0.70f, nr * 2.0f, nr * 1.4f);
            g.drawLine(nx + nr * 0.9f, ny, nx + nr * 0.9f, ny - lineSpacing * 2.8f, 1.2f);
        }
    }

    //--------------------------------------------------------------------------
    // OUTLAW — sheriff star badge with a break
    static void drawOutlaw(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                           float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY();
        float r = b.getWidth() * 0.26f * breathScale;
        const int kPoints = 6;
        g.setColour(accent.withAlpha(0.65f));
        juce::Path star;
        for (int p = 0; p < kPoints; ++p)
        {
            float a0 = (float)p * juce::MathConstants<float>::twoPi / (float)kPoints
                       - juce::MathConstants<float>::halfPi;
            float a1 = a0 + juce::MathConstants<float>::pi / (float)kPoints;
            float outerX = cx + r * std::cos(a0);
            float outerY = cy + r * std::sin(a0);
            float innerX = cx + r * 0.45f * std::cos(a1);
            float innerY = cy + r * 0.45f * std::sin(a1);
            if (p == 0) star.startNewSubPath(outerX, outerY);
            else star.lineTo(outerX, outerY);
            star.lineTo(innerX, innerY);
        }
        star.closeSubPath();
        g.fillPath(star);
        g.setColour(accent.withAlpha(0.90f));
        g.strokePath(star, juce::PathStrokeType(1.0f, juce::PathStrokeType::mitered,
                     juce::PathStrokeType::butt));
        // X over badge
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.50f));
        g.drawLine(cx - r * 0.40f, cy - r * 0.40f, cx + r * 0.40f, cy + r * 0.40f, 1.2f);
        g.drawLine(cx + r * 0.40f, cy - r * 0.40f, cx - r * 0.40f, cy + r * 0.40f, 1.2f);
    }

    //--------------------------------------------------------------------------
    // OUTBREAK — radial burst from centre
    static void drawOutbreak(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                             float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.04f;
        float cy = b.getCentreY();
        float maxR = b.getWidth() * 0.40f * breathScale;
        const int kRays = 12;
        g.setColour(accent.withAlpha(0.75f));
        for (int r = 0; r < kRays; ++r)
        {
            float angle = (float)r * juce::MathConstants<float>::twoPi / (float)kRays;
            float innerR = maxR * 0.22f;
            float outerR = maxR * (0.65f + ((r % 3 == 0) ? 0.35f : 0.0f));
            float alpha = (r % 3 == 0) ? 0.85f : 0.50f;
            g.setColour(accent.withAlpha(alpha));
            g.drawLine(cx + innerR * std::cos(angle), cy + innerR * std::sin(angle),
                       cx + outerR * std::cos(angle), cy + outerR * std::sin(angle),
                       (r % 3 == 0) ? 1.8f : 1.0f);
        }
        g.setColour(accent.withAlpha(0.90f));
        g.fillEllipse(cx - 4.0f, cy - 4.0f, 8.0f, 8.0f);
    }

    //--------------------------------------------------------------------------
    // OBSERVANDUM — magnifying glass
    static void drawObservandum(juce::Graphics& g, juce::Rectangle<float> b, const juce::Colour& accent,
                                float breathScale, float couplingLean)
    {
        float cx = b.getCentreX() + couplingLean * b.getWidth() * 0.05f;
        float cy = b.getCentreY() - b.getHeight() * 0.04f;
        float lensR = b.getWidth() * 0.22f * breathScale;
        float handleLen = lensR * 1.20f;
        float handleAngle = juce::MathConstants<float>::pi * 0.75f;
        g.setColour(accent.withAlpha(0.60f));
        g.fillEllipse(cx - lensR, cy - lensR, lensR * 2.0f, lensR * 2.0f);
        g.setColour(accent.withAlpha(0.90f));
        g.drawEllipse(cx - lensR, cy - lensR, lensR * 2.0f, lensR * 2.0f, 2.0f);
        // Handle
        float hStartX = cx + lensR * std::cos(handleAngle);
        float hStartY = cy + lensR * std::sin(handleAngle);
        g.setColour(accent.withAlpha(0.80f));
        g.drawLine(hStartX, hStartY,
                   hStartX + handleLen * std::cos(handleAngle),
                   hStartY + handleLen * std::sin(handleAngle), 2.5f);
        // Scan lines inside lens
        g.setColour(juce::Colour(GalleryColors::get(GalleryColors::t1())).withAlpha(0.25f));
        for (int sl = -2; sl <= 2; ++sl)
        {
            float slY = cy + (float)sl * lensR * 0.35f;
            float slHW = std::sqrt(juce::jmax(0.0f, lensR * lensR - (slY - cy) * (slY - cy)));
            if (slHW > 1.0f)
                g.drawLine(cx - slHW, slY, cx + slHW, slY, 0.7f);
        }
    }

    JUCE_DECLARE_NON_COPYABLE(CreatureRenderer)
};

} // namespace xoceanus
