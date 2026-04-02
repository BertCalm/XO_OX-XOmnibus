#pragma once
#include <juce_graphics/juce_graphics.h>
#include <cmath>
#include <random>

namespace xolokun {

//==============================================================================
// XPNCoverArt — Procedural cover art generator for MPC expansion packs.
//
// Pure C++ port of Tools/xpn_cover_art.py. Uses JUCE Graphics for rendering
// so the plugin ships with zero external dependencies.
//
// Outputs 2000x2000 (social/web) and 1000x1000 (MPC standard) PNG artwork
// with engine-specific procedural visual styles and branded text overlay.
//
class XPNCoverArt {
public:

    //==========================================================================
    // Engine visual definitions
    //==========================================================================

    struct EngineDef {
        juce::Colour accent;
        juce::Colour bgBase;
        const char*  styleName;    // which procedural pattern
        const char*  label;        // displayed on cover
    };

    static EngineDef getEngineDef(const juce::String& engineId)
    {
        auto id = engineId.toUpperCase();

        if (id == "ONSET"     || id == "XONSET")
            return { juce::Colour(0xFF0066FF), juce::Colour(0xFF00081C), "transient_spikes", "ONSET" };
        if (id == "OVERWORLD" || id == "XOVERWORLD")
            return { juce::Colour(0xFF39FF14), juce::Colour(0xFF000C00), "pixel_grid", "OVERWORLD" };
        if (id == "SNAP"      || id == "ODDFELIX")
            return { juce::Colour(0xFF00A6D6), juce::Colour(0xFF140805), "angular_cuts", "OddfeliX" };
        if (id == "MORPH"     || id == "ODDOSCAR")
            return { juce::Colour(0xFFE8839B), juce::Colour(0xFF000C0C), "wave_morph", "OddOscar" };
        if (id == "DUB"       || id == "OVERDUB")
            return { juce::Colour(0xFF6B7B3A), juce::Colour(0xFF080A02), "tape_streaks", "OVERDUB" };
        if (id == "DRIFT"     || id == "ODYSSEY")
            return { juce::Colour(0xFF7B2D8B), juce::Colour(0xFF080012), "lissajous", "ODYSSEY" };
        if (id == "BOB"       || id == "OBLONG")
            return { juce::Colour(0xFFE9A84A), juce::Colour(0xFF140C00), "freq_bands", "OBLONG" };
        if (id == "FAT"       || id == "OBESE")
            return { juce::Colour(0xFFFF1493), juce::Colour(0xFF12000A), "dense_blocks", "OBESE" };
        if (id == "OPAL"      || id == "XOPAL")
            return { juce::Colour(0xFFA78BFA), juce::Colour(0xFF080414), "particle_scatter", "OPAL" };
        if (id == "ORGANON")
            return { juce::Colour(0xFF00CED1), juce::Colour(0xFF000C0C), "wave_morph", "ORGANON" };
        if (id == "OUROBOROS")
            return { juce::Colour(0xFFFF2D2D), juce::Colour(0xFF140000), "lissajous", "OUROBOROS" };
        if (id == "OBSIDIAN")
            return { juce::Colour(0xFFE8E0D8), juce::Colour(0xFF0A0A0A), "freq_bands", "OBSIDIAN" };
        if (id == "OVERBITE")
            return { juce::Colour(0xFFF0EDE8), juce::Colour(0xFF0A0808), "transient_spikes", "OVERBITE" };
        if (id == "ORIGAMI")
            return { juce::Colour(0xFFE63946), juce::Colour(0xFF140408), "angular_cuts", "ORIGAMI" };
        if (id == "ORACLE")
            return { juce::Colour(0xFF4B0082), juce::Colour(0xFF080014), "particle_scatter", "ORACLE" };
        if (id == "OBSCURA")
            return { juce::Colour(0xFF8A9BA8), juce::Colour(0xFF080A0C), "tape_streaks", "OBSCURA" };
        if (id == "OCEANIC")
            return { juce::Colour(0xFF00B4A0), juce::Colour(0xFF000E0C), "wave_morph", "OCEANIC" };
        if (id == "OPTIC"    || id == "XOPTIC")
            return { juce::Colour(0xFF00FF41), juce::Colour(0xFF000A00), "particle_scatter", "OPTIC" };
        if (id == "OBLIQUE"  || id == "XOBLIQUE")
            return { juce::Colour(0xFFBF40FF), juce::Colour(0xFF0A0014), "prism_fractal", "OBLIQUE" };
        if (id == "ORBITAL"   || id == "XORBITAL")
            return { juce::Colour(0xFFFF6B6B), juce::Colour(0xFF140404), "lissajous", "ORBITAL" };
        if (id == "OCELOT"   || id == "XOCELOT")
            return { juce::Colour(0xFFC5832B), juce::Colour(0xFF0C0800), "freq_bands", "OCELOT" };
        if (id == "OSPREY"   || id == "XOSPREY")
            return { juce::Colour(0xFF1B4F8A), juce::Colour(0xFF000810), "wave_morph", "OSPREY" };
        if (id == "OSTERIA"  || id == "XOSTERIA")
            return { juce::Colour(0xFF722F37), juce::Colour(0xFF0A0404), "tape_streaks", "OSTERIA" };
        if (id == "OWLFISH"  || id == "XOWLFISH")
            return { juce::Colour(0xFFB8860B), juce::Colour(0xFF0C0A02), "freq_bands", "OWLFISH" };
        if (id == "OHM"      || id == "XOHM")
            return { juce::Colour(0xFF87AE73), juce::Colour(0xFF060A04), "wave_morph", "OHM" };
        if (id == "ORPHICA"  || id == "XORPHICA")
            return { juce::Colour(0xFF7FDBCA), juce::Colour(0xFF000C0A), "particle_scatter", "ORPHICA" };
        if (id == "OBBLIGATO" || id == "XOBBLIGATO")
            return { juce::Colour(0xFFFF8A7A), juce::Colour(0xFF100604), "angular_cuts", "OBBLIGATO" };
        if (id == "OTTONI"   || id == "XOTTONI")
            return { juce::Colour(0xFF5B8A72), juce::Colour(0xFF040A06), "dense_blocks", "OTTONI" };
        if (id == "OLE"      || id == "XOLE")
            return { juce::Colour(0xFFC9377A), juce::Colour(0xFF0C0208), "transient_spikes", "OLE" };
        if (id == "OVERLAP"  || id == "XOVERLAP")
            return { juce::Colour(0xFF00FFB4), juce::Colour(0xFF000C08), "wave_morph", "OVERLAP" };
        if (id == "OUTWIT"   || id == "XOUTWIT")
            return { juce::Colour(0xFFCC6600), juce::Colour(0xFF0C0800), "angular_cuts", "OUTWIT" };
        if (id == "OMBRE"    || id == "XOMBRE")
            return { juce::Colour(0xFF7B6B8A), juce::Colour(0xFF080610), "lissajous", "OMBRE" };
        if (id == "ORCA"     || id == "XORCA")
            return { juce::Colour(0xFF1B2838), juce::Colour(0xFF040608), "dense_blocks", "ORCA" };
        if (id == "OCTOPUS"  || id == "XOCTOPUS")
            return { juce::Colour(0xFFE040FB), juce::Colour(0xFF0C0010), "particle_scatter", "OCTOPUS" };

        // Default: XO Gold
        return { juce::Colour(0xFFE9C46A), juce::Colour(0xFF0C0A08), "freq_bands", "XO_OX" };
    }

    //==========================================================================
    // Public generation API
    //==========================================================================

    struct CoverResult {
        juce::File cover1000;
        juce::File cover2000;
        bool success = false;
    };

    static CoverResult generate(
        const juce::String& engineId,
        const juce::String& packName,
        const juce::File&   outputDir,
        int presetCount = 0,
        const juce::String& version = "1.0",
        int seed = 0)
    {
        CoverResult result;

        auto eng = getEngineDef(engineId);
        std::mt19937 rng(static_cast<unsigned>(seed));

        constexpr int SIZE = 2000;

        // 1. Render procedural artwork to an Image
        juce::Image img(juce::Image::ARGB, SIZE, SIZE, true);

        {
            juce::Graphics g(img);
            // Background: radial vignette
            paintBase(g, SIZE, eng.bgBase);
            // Engine-specific style layer
            paintStyle(g, SIZE, eng.accent, eng.styleName, rng);
            // Text overlay
            paintTextOverlay(g, SIZE, eng, packName, presetCount, version);
        }

        // 2. Save 2000x2000
        outputDir.createDirectory();
        result.cover2000 = outputDir.getChildFile("artwork_2000.png");
        savePNG(img, result.cover2000);

        // 3. Downscale to 1000x1000 and save
        auto img1000 = img.rescaled(1000, 1000, juce::Graphics::highResamplingQuality);
        result.cover1000 = outputDir.getChildFile("artwork.png");
        savePNG(img1000, result.cover1000);

        result.success = result.cover1000.existsAsFile() && result.cover2000.existsAsFile();
        return result;
    }

private:

    //==========================================================================
    // Brand constants
    //==========================================================================

    static juce::Colour xoGold()     { return juce::Colour(0xFFE9C46A); }
    static juce::Colour warmWhite()  { return juce::Colour(0xFFF8F6F3); }

    //==========================================================================
    // Background: radial vignette
    //==========================================================================

    static void paintBase(juce::Graphics& g, int size, juce::Colour bgBase)
    {
        float cx = size * 0.5f, cy = size * 0.5f;
        float radius = size * 0.75f;

        // Fill with darkened version (edge)
        g.fillAll(bgBase.darker(0.6f));

        // Radial gradient from center (brighter) to edge
        juce::ColourGradient gradient(
            bgBase, cx, cy,
            bgBase.darker(0.6f), cx + radius, cy, true);
        g.setGradientFill(gradient);
        g.fillRect(0, 0, size, size);
    }

    //==========================================================================
    // Style dispatcher
    //==========================================================================

    static void paintStyle(juce::Graphics& g, int size, juce::Colour accent,
                           const char* style, std::mt19937& rng)
    {
        juce::String s(style);
        if (s == "transient_spikes")  { styleTransientSpikes(g, size, accent, rng); return; }
        if (s == "pixel_grid")        { stylePixelGrid(g, size, accent, rng); return; }
        if (s == "angular_cuts")      { styleAngularCuts(g, size, accent, rng); return; }
        if (s == "wave_morph")        { styleWaveMorph(g, size, accent, rng); return; }
        if (s == "tape_streaks")      { styleTapeStreaks(g, size, accent, rng); return; }
        if (s == "lissajous")         { styleLissajous(g, size, accent, rng); return; }
        if (s == "freq_bands")        { styleFreqBands(g, size, accent, rng); return; }
        if (s == "dense_blocks")      { styleDenseBlocks(g, size, accent, rng); return; }
        if (s == "particle_scatter")  { styleParticleScatter(g, size, accent, rng); return; }
        if (s == "prism_fractal")    { stylePrismFractal(g, size, accent, rng); return; }
        // Fallback
        styleFreqBands(g, size, accent, rng);
    }

    //==========================================================================
    // RNG helpers
    //==========================================================================

    static int randInt(std::mt19937& rng, int lo, int hi)
    {
        std::uniform_int_distribution<int> dist(lo, hi);
        return dist(rng);
    }

    static float randFloat(std::mt19937& rng, float lo, float hi)
    {
        std::uniform_real_distribution<float> dist(lo, hi);
        return dist(rng);
    }

    //==========================================================================
    // Style: Transient Spikes (ONSET)
    // Sharp vertical bars rising from the bottom — percussive geometry.
    //==========================================================================

    static void styleTransientSpikes(juce::Graphics& g, int size, juce::Colour accent,
                                     std::mt19937& rng)
    {
        constexpr int nBars = 24;
        for (int i = 0; i < nBars; ++i)
        {
            int xCenter = randInt(rng, 20, size - 20);
            int width   = (i % 4 != 0) ? randInt(rng, 2, 8) : randInt(rng, 10, 24);
            float heightFrac = randFloat(rng, 0.3f, 1.0f);
            float brightness = randFloat(rng, 0.3f, 1.0f);

            int yStart = (int)(size * (1.0f - heightFrac));
            int x0 = juce::jmax(0, xCenter - width / 2);
            int x1 = juce::jmin(size, xCenter + width / 2 + 1);

            // Gradient bar: bright at bottom, fading at top
            juce::ColourGradient barGrad(
                accent.withAlpha(brightness * 0.7f), (float)(x0 + x1) / 2, (float)size,
                accent.withAlpha(brightness * 0.1f), (float)(x0 + x1) / 2, (float)yStart,
                false);
            g.setGradientFill(barGrad);
            g.fillRect(x0, yStart, x1 - x0, size - yStart);
        }
    }

    //==========================================================================
    // Style: Pixel Grid (OVERWORLD)
    // 8-bit retro grid with scanlines.
    //==========================================================================

    static void stylePixelGrid(juce::Graphics& g, int size, juce::Colour accent,
                               std::mt19937& rng)
    {
        constexpr int pixelSize = 20;
        for (int py = 0; py < size; py += pixelSize)
        {
            for (int px = 0; px < size; px += pixelSize)
            {
                if (randFloat(rng, 0.0f, 1.0f) > 0.55f)
                {
                    float brightness = randFloat(rng, 0.1f, 0.65f);
                    g.setColour(accent.withAlpha(brightness));
                    g.fillRect(px, py, pixelSize - 1, pixelSize - 1);
                }
            }
        }

        // Scanlines
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        for (int y = 0; y < size; y += 4)
            g.fillRect(0, y, size, 1);
    }

    //==========================================================================
    // Style: Angular Cuts (SNAP / OddfeliX)
    // Sharp diagonal bands across the canvas.
    //==========================================================================

    static void styleAngularCuts(juce::Graphics& g, int size, juce::Colour accent,
                                 std::mt19937& rng)
    {
        // Draw multiple rotated band sets
        float angles[] = { randFloat(rng, 30.0f, 60.0f),
                           randFloat(rng, 120.0f, 150.0f),
                           randFloat(rng, -20.0f, 20.0f) };

        for (float angle : angles)
        {
            float rad = angle * juce::MathConstants<float>::pi / 180.0f;
            int period = randInt(rng, 200, 500);
            float brightness = randFloat(rng, 0.15f, 0.45f);

            // Render bands as a series of thin filled rects along the diagonal
            for (int band = -size; band < size * 2; band += period / 4)
            {
                float phase = std::sin((float)band / period * juce::MathConstants<float>::twoPi);
                float alpha = (phase + 1.0f) * 0.5f * brightness;
                if (alpha < 0.02f) continue;

                g.setColour(accent.withAlpha(alpha));

                // Rotated stripe
                juce::Path stripe;
                float w = (float)(period / 4);
                stripe.addRectangle(-size * 2.0f, (float)band, size * 4.0f, w);

                auto transform = juce::AffineTransform::rotation(rad, size * 0.5f, size * 0.5f);
                g.fillPath(stripe, transform);
            }
        }
    }

    //==========================================================================
    // Style: Wave Morph (MORPH / OddOscar)
    // Smooth sinusoidal wave blends.
    //==========================================================================

    static void styleWaveMorph(juce::Graphics& g, int size, juce::Colour accent,
                               std::mt19937& rng)
    {
        // Layered sine wave bands
        for (int i = 0; i < 5; ++i)
        {
            float freqX = randFloat(rng, 0.5f, 3.0f);
            float freqY = randFloat(rng, 0.5f, 3.0f);
            float phase = randFloat(rng, 0.0f, juce::MathConstants<float>::twoPi);
            float amp = randFloat(rng, 0.1f, 0.3f);

            // Render as horizontal strips with varying alpha
            for (int y = 0; y < size; y += 3)
            {
                for (int x = 0; x < size; x += 3)
                {
                    float wave = (std::sin(x * freqX / size * juce::MathConstants<float>::twoPi
                                         + y * freqY / size * juce::MathConstants<float>::pi
                                         + phase) + 1.0f) * 0.5f;
                    float alpha = wave * amp;
                    if (alpha < 0.02f) continue;

                    g.setColour(accent.withAlpha(alpha));
                    g.fillRect(x, y, 3, 3);
                }
            }
        }
    }

    //==========================================================================
    // Style: Tape Streaks (DUB / Overdub)
    // Diagonal echo trail streaks.
    //==========================================================================

    static void styleTapeStreaks(juce::Graphics& g, int size, juce::Colour accent,
                                std::mt19937& rng)
    {
        for (int s = 0; s < 12; ++s)
        {
            float xStart = (float)randInt(rng, 0, size);
            float yStart = (float)randInt(rng, 0, size / 2);
            int length   = randInt(rng, 200, size);
            float width  = (float)randInt(rng, 1, 5);
            float brightness = randFloat(rng, 0.2f, 0.6f);
            float dx = randFloat(rng, 0.7f, 1.0f);
            float dy = randFloat(rng, 0.1f, 0.5f);

            juce::Path streak;
            streak.startNewSubPath(xStart, yStart);
            for (int step = 1; step < length; step += 4)
            {
                float x = std::fmod(xStart + step * dx, (float)size);
                float y = std::fmod(yStart + step * dy, (float)size);
                streak.lineTo(x, y);
            }

            float fade = brightness * 0.65f;
            g.setColour(accent.withAlpha(fade));
            g.strokePath(streak, juce::PathStrokeType(width, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
        }
    }

    //==========================================================================
    // Style: Lissajous (DRIFT / Odyssey)
    // Orbital psychedelic curves.
    //==========================================================================

    static void styleLissajous(juce::Graphics& g, int size, juce::Colour accent,
                               std::mt19937& rng)
    {
        float cx = size * 0.5f, cy = size * 0.5f;
        float scale = size * 0.42f;

        struct Ratio { int a, b; };
        Ratio ratios[] = { {1,2}, {2,3}, {3,4}, {1,3}, {3,5} };

        for (auto [aFreq, bFreq] : ratios)
        {
            float phase = randFloat(rng, 0.0f, juce::MathConstants<float>::pi);
            float brightness = randFloat(rng, 0.4f, 0.75f);

            juce::Path curve;
            constexpr int nPts = 2000;
            for (int i = 0; i < nPts; ++i)
            {
                float t = (float)i / nPts * juce::MathConstants<float>::twoPi;
                float x = cx + scale * std::sin(aFreq * t + phase);
                float y = cy + scale * std::sin(bFreq * t);

                if (i == 0) curve.startNewSubPath(x, y);
                else curve.lineTo(x, y);
            }
            curve.closeSubPath();

            g.setColour(accent.withAlpha(brightness * 0.5f));
            g.strokePath(curve, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        }
    }

    //==========================================================================
    // Style: Frequency Bands (BOB / Oblong)
    // Horizontal spectrum bands, warm analog feel.
    //==========================================================================

    static void styleFreqBands(juce::Graphics& g, int size, juce::Colour accent,
                               std::mt19937& rng)
    {
        constexpr int nBands = 40;
        for (int i = 0; i < nBands; ++i)
        {
            int yCenter   = randInt(rng, 0, size);
            int bandH     = randInt(rng, 5, 40);
            float brightness = randFloat(rng, 0.08f, 0.4f);

            int y0 = juce::jmax(0, yCenter - bandH / 2);
            int y1 = juce::jmin(size, yCenter + bandH / 2);

            // Varying alpha across X to simulate amplitude envelope
            constexpr int segW = 8;
            for (int x = 0; x < size; x += segW)
            {
                float env = randFloat(rng, 0.2f, 1.0f) * brightness;
                g.setColour(accent.withAlpha(env * 0.65f));
                g.fillRect(x, y0, segW, y1 - y0);
            }
        }
    }

    //==========================================================================
    // Style: Dense Blocks (FAT / Obese)
    // Heavy overlapping rectangular masses.
    //==========================================================================

    static void styleDenseBlocks(juce::Graphics& g, int size, juce::Colour accent,
                                 std::mt19937& rng)
    {
        for (int i = 0; i < 30; ++i)
        {
            int x0 = randInt(rng, 0, size / 2);
            int y0 = randInt(rng, 0, size);
            int w  = randInt(rng, size / 4, size);
            int h  = randInt(rng, 10, 80);
            float brightness = randFloat(rng, 0.08f, 0.35f);

            g.setColour(accent.withAlpha(brightness));
            g.fillRect(x0, y0, w, h);
        }
    }

    //==========================================================================
    // Style: Particle Scatter (OPAL)
    // Granular cloud field — Gaussian cluster with outliers.
    //==========================================================================

    static void styleParticleScatter(juce::Graphics& g, int size, juce::Colour accent,
                                     std::mt19937& rng)
    {
        float cx = size * 0.5f, cy = size * 0.5f;
        std::normal_distribution<float> gaussDist(0.0f, size * 0.25f);

        constexpr int nParticles = 3000;
        for (int i = 0; i < nParticles; ++i)
        {
            float x, y;
            if (randFloat(rng, 0.0f, 1.0f) > 0.2f)
            {
                // Gaussian cluster around center
                x = cx + gaussDist(rng);
                y = cy + gaussDist(rng);
            }
            else
            {
                // Random outlier
                x = (float)randInt(rng, 0, size);
                y = (float)randInt(rng, 0, size);
            }

            // Wrap to canvas
            x = std::fmod(std::fmod(x, (float)size) + size, (float)size);
            y = std::fmod(std::fmod(y, (float)size) + size, (float)size);

            float radius = (float)randInt(rng, 1, 4);
            float brightness = randFloat(rng, 0.15f, 0.7f);

            g.setColour(accent.withAlpha(brightness));
            g.fillEllipse(x - radius, y - radius, radius * 2, radius * 2);
        }
    }

    //==========================================================================
    // Style: Prism Fractal (OBLIQUE)
    // Refractive prismatic geometry — angular shards with spectral color shifts.
    //==========================================================================

    static void stylePrismFractal(juce::Graphics& g, int size, juce::Colour accent,
                                  std::mt19937& rng)
    {
        float cx = size * 0.5f, cy = size * 0.5f;

        // Spectral palette derived from accent via hue rotation
        float hue, sat, bri;
        accent.getHSB(hue, sat, bri);

        // Layer 1: Large prismatic shards radiating from center
        for (int i = 0; i < 12; ++i)
        {
            float angle = (float)i / 12.0f * juce::MathConstants<float>::twoPi
                        + randFloat(rng, -0.15f, 0.15f);
            float length = randFloat(rng, size * 0.3f, size * 0.7f);
            float spread = randFloat(rng, 0.08f, 0.25f);

            // Each shard gets a hue-shifted variant of the accent
            float shardHue = std::fmod(hue + (float)i / 12.0f * 0.4f, 1.0f);
            auto shardColour = juce::Colour::fromHSV(shardHue, sat * 0.8f, bri, 1.0f);
            float alpha = randFloat(rng, 0.12f, 0.35f);

            juce::Path shard;
            shard.startNewSubPath(cx, cy);
            shard.lineTo(cx + std::cos(angle - spread) * length,
                        cy + std::sin(angle - spread) * length);
            shard.lineTo(cx + std::cos(angle + spread) * length * 0.85f,
                        cy + std::sin(angle + spread) * length * 0.85f);
            shard.closeSubPath();

            g.setColour(shardColour.withAlpha(alpha));
            g.fillPath(shard);

            // Edge highlight
            g.setColour(shardColour.brighter(0.3f).withAlpha(alpha * 0.6f));
            g.strokePath(shard, juce::PathStrokeType(1.5f));
        }

        // Layer 2: Smaller refractive triangles scattered across the canvas
        for (int i = 0; i < 40; ++i)
        {
            float x = randFloat(rng, 0.0f, (float)size);
            float y = randFloat(rng, 0.0f, (float)size);
            float triSize = randFloat(rng, size * 0.02f, size * 0.08f);
            float rotation = randFloat(rng, 0.0f, juce::MathConstants<float>::twoPi);
            float triHue = std::fmod(hue + randFloat(rng, -0.2f, 0.2f) + 1.0f, 1.0f);
            float alpha = randFloat(rng, 0.08f, 0.3f);

            juce::Path tri;
            for (int v = 0; v < 3; ++v)
            {
                float a = rotation + (float)v * juce::MathConstants<float>::twoPi / 3.0f;
                float px = x + std::cos(a) * triSize;
                float py = y + std::sin(a) * triSize;
                if (v == 0) tri.startNewSubPath(px, py);
                else tri.lineTo(px, py);
            }
            tri.closeSubPath();

            g.setColour(juce::Colour::fromHSV(triHue, sat, bri * 0.9f, 1.0f).withAlpha(alpha));
            g.fillPath(tri);
        }

        // Layer 3: Spectral dispersion lines — light passing through a prism
        for (int i = 0; i < 7; ++i)
        {
            float bandHue = std::fmod(hue + (float)i / 7.0f * 0.6f, 1.0f);
            auto bandColour = juce::Colour::fromHSV(bandHue, 0.9f, 0.9f, 1.0f);
            float yOffset = cy + (i - 3) * size * 0.04f + randFloat(rng, -10.0f, 10.0f);

            juce::Path band;
            band.startNewSubPath(0, yOffset + randFloat(rng, -20.0f, 20.0f));
            for (int x = 0; x <= size; x += 40)
            {
                float wave = std::sin((float)x / size * juce::MathConstants<float>::pi * 3.0f
                                     + (float)i * 0.8f) * size * 0.03f;
                band.lineTo((float)x, yOffset + wave);
            }

            g.setColour(bandColour.withAlpha(randFloat(rng, 0.06f, 0.18f)));
            g.strokePath(band, juce::PathStrokeType(randFloat(rng, 2.0f, 6.0f),
                                                     juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
        }
    }

    //==========================================================================
    // Text overlay — pack name, engine label, preset count, watermark
    //==========================================================================

    static void paintTextOverlay(juce::Graphics& g, int size, const EngineDef& eng,
                                 const juce::String& packName, int presetCount,
                                 const juce::String& version)
    {
        float pad  = size * 0.04f;

        // Font sizes (proportional to canvas)
        float titleSize  = size * 0.055f;
        float labelSize  = size * 0.035f;
        float metaSize   = size * 0.022f;
        float wmSize     = size * 0.028f;

        // --- Engine accent bar (left edge) ---
        float barW = size * 0.012f;
        g.setColour(eng.accent.withAlpha(0.7f));
        g.fillRect(juce::Rectangle<float>(pad, pad, barW, size - pad * 2));

        float textX = pad + barW + size * 0.02f;
        float textY = size * 0.06f;

        // --- Pack title (drop shadow + white) ---
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(titleSize)).boldened());
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.drawText(packName, (int)(textX + 3), (int)(textY + 3),
                   size - (int)textX - (int)pad, (int)titleSize + 4,
                   juce::Justification::topLeft);
        g.setColour(warmWhite());
        g.drawText(packName, (int)textX, (int)textY,
                   size - (int)textX - (int)pad, (int)titleSize + 4,
                   juce::Justification::topLeft);

        // --- Engine label pill ---
        float pillY = textY + size * 0.075f;
        float pillPadX = size * 0.015f;
        float pillPadY = size * 0.008f;
        juce::String label(eng.label);
        auto labelFont = juce::Font(juce::FontOptions{}.withHeight(labelSize)).boldened();
        g.setFont(labelFont);
        float labelW = static_cast<float>(labelFont.getStringWidth(label));
        float labelH = labelFont.getHeight();

        auto pillRect = juce::Rectangle<float>(
            textX - pillPadX, pillY - pillPadY,
            labelW + pillPadX * 2, labelH + pillPadY * 2);
        g.setColour(eng.accent);
        g.fillRoundedRectangle(pillRect, size * 0.01f);
        g.setColour(juce::Colours::black);
        g.drawText(label, (int)textX, (int)pillY,
                   (int)labelW + 2, (int)labelH,
                   juce::Justification::centredLeft);

        // --- Preset count + version (bottom left) ---
        float metaY = size - pad - size * 0.06f;
        auto metaFont = juce::Font(juce::FontOptions{}.withHeight(metaSize));
        g.setFont(metaFont);

        juce::String countText = juce::String(presetCount) + " PRESETS";
        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.drawText(countText, (int)(textX + 1), (int)(metaY + 1),
                   300, (int)metaSize + 4, juce::Justification::topLeft);
        g.setColour(xoGold());
        g.drawText(countText, (int)textX, (int)metaY,
                   300, (int)metaSize + 4, juce::Justification::topLeft);

        juce::String verText = "v" + version;
        float verY = metaY + size * 0.03f;
        g.setColour(juce::Colour(0xFFA09B96));
        g.drawText(verText, (int)textX, (int)verY,
                   300, (int)metaSize + 4, juce::Justification::topLeft);

        // --- XO_OX watermark (bottom right, subtle) ---
        auto wmFont = juce::Font(juce::FontOptions{}.withHeight(wmSize));
        g.setFont(wmFont);
        float wmW = static_cast<float>(wmFont.getStringWidth("XO_OX"));
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawText("XO_OX",
                   size - (int)pad - (int)wmW - 2, size - (int)pad - (int)wmSize - 2,
                   (int)wmW + 4, (int)wmSize + 4,
                   juce::Justification::centred);
    }

    //==========================================================================
    // PNG output
    //==========================================================================

    static bool savePNG(const juce::Image& img, const juce::File& file)
    {
        file.deleteFile();
        auto stream = file.createOutputStream();
        if (!stream) return false;

        juce::PNGImageFormat png;
        return png.writeImageToStream(img, *stream);
    }
};

} // namespace xolokun
