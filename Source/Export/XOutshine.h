#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <random>
#include <vector>

namespace xomnibus {

//==============================================================================
//  XOutshine — Sample Pack Upgrade Engine
//  "Outshine the original."
//
//  Integrated into the XOmnibus desktop application. Ingests any WAV samples
//  or XPN archives and upgrades them into production-quality, expressive,
//  dynamic MPC instruments — without leaving the plugin.
//
//  Pipeline: Ingest → Classify → Analyze → Enhance → Normalize → Map → Package
//
//==============================================================================

//------------------------------------------------------------------------------
// Sample classification
//------------------------------------------------------------------------------

enum class SampleCategory
{
    Kick, Snare, HiHatClosed, HiHatOpen, Clap, Tom, Percussion, FX,
    Bass, Pad, Lead, Keys, Pluck, String, Unknown
};

inline bool isDrumCategory (SampleCategory c)
{
    return c == SampleCategory::Kick || c == SampleCategory::Snare
        || c == SampleCategory::HiHatClosed || c == SampleCategory::HiHatOpen
        || c == SampleCategory::Clap || c == SampleCategory::Tom
        || c == SampleCategory::Percussion || c == SampleCategory::FX;
}

inline int gmNoteForCategory (SampleCategory c)
{
    switch (c) {
        case SampleCategory::Kick:         return 36;
        case SampleCategory::Snare:        return 38;
        case SampleCategory::HiHatClosed:  return 42;
        case SampleCategory::HiHatOpen:    return 46;
        case SampleCategory::Clap:         return 39;
        case SampleCategory::Tom:          return 41;
        case SampleCategory::Percussion:   return 43;
        case SampleCategory::FX:           return 49;
        default:                           return 60;
    }
}

inline const char* categoryName (SampleCategory c)
{
    switch (c) {
        case SampleCategory::Kick:         return "Kick";
        case SampleCategory::Snare:        return "Snare";
        case SampleCategory::HiHatClosed:  return "HiHat Closed";
        case SampleCategory::HiHatOpen:    return "HiHat Open";
        case SampleCategory::Clap:         return "Clap";
        case SampleCategory::Tom:          return "Tom";
        case SampleCategory::Percussion:   return "Percussion";
        case SampleCategory::FX:           return "FX";
        case SampleCategory::Bass:         return "Bass";
        case SampleCategory::Pad:          return "Pad";
        case SampleCategory::Lead:         return "Lead";
        case SampleCategory::Keys:         return "Keys";
        case SampleCategory::Pluck:        return "Pluck";
        case SampleCategory::String:       return "String";
        default:                           return "Unknown";
    }
}

//------------------------------------------------------------------------------
// Velocity curve presets
//------------------------------------------------------------------------------

struct VelocitySplit { int start; int end; float volume; };

enum class VelocityCurve { Musical, BoomBap, NeoSoul, TrapHard, Linear };

inline std::vector<VelocitySplit> getVelocitySplits (VelocityCurve curve)
{
    switch (curve) {
        case VelocityCurve::BoomBap:
            return { {1,15,0.25f}, {16,45,0.50f}, {46,85,0.78f}, {86,127,1.00f} };
        case VelocityCurve::NeoSoul:
            return { {1,30,0.35f}, {31,65,0.60f}, {66,95,0.80f}, {96,127,0.95f} };
        case VelocityCurve::TrapHard:
            return { {1,10,0.20f}, {11,35,0.55f}, {36,70,0.80f}, {71,127,1.00f} };
        case VelocityCurve::Linear:
            return { {1,31,0.25f}, {32,63,0.50f}, {64,95,0.75f}, {96,127,1.00f} };
        case VelocityCurve::Musical:
        default:
            return { {1,20,0.30f}, {21,50,0.55f}, {51,90,0.75f}, {91,127,0.95f} };
    }
}

//------------------------------------------------------------------------------
// Analyzed sample metadata
//------------------------------------------------------------------------------

struct AnalyzedSample
{
    juce::File   sourceFile;
    juce::String name;
    SampleCategory category = SampleCategory::Unknown;

    double sampleRate  = 44100.0;
    int    bitDepth    = 24;
    int    numChannels = 2;
    int    numSamples  = 0;
    double durationS   = 0.0;

    float  rmsDb       = -100.0f;
    float  peakDb      = -100.0f;
    float  dcOffset    = 0.0f;
    float  tailLengthS = 0.0f;

    bool   isLoopable  = false;
    int    loopStart   = 0;
    int    loopEnd     = 0;
};

//------------------------------------------------------------------------------
// Enhanced sample (velocity layer + round-robin variant)
//------------------------------------------------------------------------------

struct EnhancedLayer
{
    juce::File   file;
    juce::String filename;
    int          velLayer  = 0;
    int          rrIndex   = 0;
};

struct UpgradedProgram
{
    juce::String    name;
    SampleCategory  category;
    AnalyzedSample  sourceInfo;
    std::vector<EnhancedLayer> layers;
    int numVelocityLayers = 4;
    int numRoundRobin     = 4;
};

//------------------------------------------------------------------------------
// Progress callback
//------------------------------------------------------------------------------

using ProgressCallback = std::function<void (float progress, const juce::String& stage)>;

//------------------------------------------------------------------------------
// XOutshine configuration
//------------------------------------------------------------------------------

struct OutshineSettings
{
    int            velocityLayers = 4;
    int            roundRobin     = 4;
    VelocityCurve  velocityCurve  = VelocityCurve::Musical;
    float          lufsTarget     = -14.0f;
    bool           applyFadeGuards = true;
    bool           removeDC       = true;
    bool           applyDither    = true;
    bool           detectLoops    = true;
    juce::String   packName;      // empty = derived from input
};

//==============================================================================
// XOutshine — main upgrade engine
//==============================================================================

class XOutshine
{
public:
    XOutshine() = default;

    //--------------------------------------------------------------------------
    // Run the full upgrade pipeline.
    // inputPath: .xpn file or folder of WAVs
    // outputPath: .xpn file or folder
    // Returns true on success.
    //--------------------------------------------------------------------------
    bool run (const juce::File& inputPath,
              const juce::File& outputPath,
              const OutshineSettings& settings,
              ProgressCallback progress = nullptr)
    {
        settings_ = settings;
        progress_ = progress;

        if (settings_.packName.isEmpty())
            settings_.packName = inputPath.getFileNameWithoutExtension();

        auto workDir = juce::File::getSpecialLocation(
            juce::File::tempDirectory).getChildFile("xoutshine_" + juce::String(juce::Random::getSystemRandom().nextInt()));
        workDir.createDirectory();

        bool ok = true;

        // Stage 1: INGEST
        report(0.0f, "Ingesting samples...");
        auto samples = ingest(inputPath, workDir);
        if (samples.empty()) { workDir.deleteRecursively(); return false; }

        // Stage 2: CLASSIFY
        report(0.15f, "Classifying " + juce::String(samples.size()) + " samples...");
        classify(samples);

        // Stage 3: ANALYZE
        report(0.25f, "Analyzing audio...");
        analyze(samples);

        // Stage 4: ENHANCE
        report(0.35f, "Enhancing — velocity layers + round-robin...");
        auto programs = enhance(samples, workDir);

        // Stage 5: NORMALIZE
        report(0.60f, "LUFS normalizing...");
        normalize(programs);

        // Stage 6: MAP
        report(0.75f, "Building XPM programs...");
        buildPrograms(programs, workDir);

        // Stage 7: PACKAGE
        report(0.85f, "Packaging...");
        if (outputPath.getFileExtension().equalsIgnoreCase(".xpn"))
            ok = packageXPN(programs, workDir, outputPath);
        else
            ok = packageFolder(programs, workDir, outputPath);

        // Stage 8: VALIDATE
        report(0.95f, "Validating...");
        int issues = validate(programs);

        report(1.0f, "XOutshine complete — " + juce::String(programs.size()) +
               " programs, " + juce::String(issues) + " issues.");

        workDir.deleteRecursively();
        return ok && issues == 0;
    }

    //--------------------------------------------------------------------------
    // Access results after run()
    //--------------------------------------------------------------------------
    int getNumPrograms() const { return (int) programs_.size(); }
    const juce::StringArray& getErrors() const { return errors_; }

private:
    OutshineSettings settings_;
    ProgressCallback progress_;
    std::vector<UpgradedProgram> programs_;
    juce::StringArray errors_;

    void report (float p, const juce::String& msg)
    {
        if (progress_) progress_(p, msg);
    }

    //==========================================================================
    // Stage 1: INGEST
    //==========================================================================

    std::vector<AnalyzedSample> ingest (const juce::File& input, const juce::File& workDir)
    {
        std::vector<AnalyzedSample> samples;
        auto wavDir = workDir.getChildFile("ingested");
        wavDir.createDirectory();

        if (input.getFileExtension().equalsIgnoreCase(".xpn"))
        {
            // Unzip XPN
            juce::ZipFile zip(input);
            for (int i = 0; i < zip.getNumEntries(); ++i)
            {
                auto* entry = zip.getEntry(i);
                if (entry == nullptr) continue;
                if (entry->filename.endsWithIgnoreCase(".wav"))
                {
                    auto dest = wavDir.getChildFile(juce::File(entry->filename).getFileName());
                    zip.uncompressEntry(i, wavDir);
                    // ZipFile may extract with full path, find the actual file
                    auto extracted = wavDir.getChildFile(entry->filename);
                    if (extracted.existsAsFile() && extracted != dest)
                    {
                        extracted.moveFileTo(dest);
                    }
                    if (dest.existsAsFile())
                    {
                        AnalyzedSample s;
                        s.sourceFile = dest;
                        s.name = dest.getFileNameWithoutExtension();
                        samples.push_back(s);
                    }
                }
            }
        }
        else if (input.isDirectory())
        {
            for (const auto& child : juce::RangedDirectoryIterator(input, true, "*.wav"))
            {
                auto src = child.getFile();
                auto dest = wavDir.getChildFile(src.getFileName());
                src.copyFileTo(dest);
                AnalyzedSample s;
                s.sourceFile = dest;
                s.name = dest.getFileNameWithoutExtension();
                samples.push_back(s);
            }
        }
        return samples;
    }

    //==========================================================================
    // Stage 2: CLASSIFY
    //==========================================================================

    void classify (std::vector<AnalyzedSample>& samples)
    {
        for (auto& s : samples)
        {
            s.category = classifyByName(s.name);
            if (s.category == SampleCategory::Unknown)
                s.category = classifyByAudio(s);
        }
    }

    static SampleCategory classifyByName (const juce::String& name)
    {
        auto nl = name.toLowerCase();
        if (nl.contains("kick") || nl.contains("bd") || nl.contains("bassdrum"))
            return SampleCategory::Kick;
        if (nl.contains("snare") || nl.contains("snr") || nl.contains("rim"))
            return SampleCategory::Snare;
        if (nl.contains("hihat") || nl.contains("hh") || nl.contains("hat")) {
            if (nl.contains("open") || nl.contains("oh"))
                return SampleCategory::HiHatOpen;
            return SampleCategory::HiHatClosed;
        }
        if (nl.contains("clap") || nl.contains("clp"))
            return SampleCategory::Clap;
        if (nl.contains("tom") || nl.contains("floor"))
            return SampleCategory::Tom;
        if (nl.contains("perc") || nl.contains("shaker") || nl.contains("tamb"))
            return SampleCategory::Percussion;
        if (nl.contains("fx") || nl.contains("riser") || nl.contains("impact"))
            return SampleCategory::FX;
        if (nl.contains("bass") || nl.contains("sub") || nl.contains("808"))
            return SampleCategory::Bass;
        if (nl.contains("pad") || nl.contains("atmo") || nl.contains("ambient"))
            return SampleCategory::Pad;
        if (nl.contains("lead") || nl.contains("synth") || nl.contains("arp"))
            return SampleCategory::Lead;
        if (nl.contains("key") || nl.contains("piano") || nl.contains("rhodes"))
            return SampleCategory::Keys;
        if (nl.contains("pluck") || nl.contains("guitar"))
            return SampleCategory::Pluck;
        if (nl.contains("string") || nl.contains("violin") || nl.contains("cello"))
            return SampleCategory::String;
        return SampleCategory::Unknown;
    }

    SampleCategory classifyByAudio (const AnalyzedSample& s)
    {
        juce::AudioFormatManager fmgr;
        fmgr.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader(fmgr.createReaderFor(s.sourceFile));
        if (!reader) return SampleCategory::Unknown;

        double dur = (double) reader->lengthInSamples / reader->sampleRate;

        if (dur < 0.5)
        {
            // Short sample — classify by spectral content
            juce::AudioBuffer<float> buf(1, (int) reader->lengthInSamples);
            reader->read(&buf, 0, (int) reader->lengthInSamples, 0, true, false);
            auto* data = buf.getReadPointer(0);
            int n = buf.getNumSamples();

            // Zero-crossing rate
            int zc = 0;
            for (int i = 1; i < n; ++i)
                if ((data[i] >= 0) != (data[i - 1] >= 0)) ++zc;
            float zcr = (float) zc / n * (float) reader->sampleRate;

            if (zcr < 500)  return SampleCategory::Kick;
            if (zcr > 3000) return SampleCategory::HiHatClosed;
            return SampleCategory::Snare;
        }
        if (dur < 3.0) return SampleCategory::Pluck;
        return SampleCategory::Pad;
    }

    //==========================================================================
    // Stage 3: ANALYZE
    //==========================================================================

    void analyze (std::vector<AnalyzedSample>& samples)
    {
        juce::AudioFormatManager fmgr;
        fmgr.registerBasicFormats();

        for (auto& s : samples)
        {
            std::unique_ptr<juce::AudioFormatReader> reader(fmgr.createReaderFor(s.sourceFile));
            if (!reader) continue;

            s.sampleRate  = reader->sampleRate;
            s.bitDepth    = (int) reader->bitsPerSample;
            s.numChannels = (int) reader->numChannels;
            s.numSamples  = (int) reader->lengthInSamples;
            s.durationS   = s.numSamples / s.sampleRate;

            juce::AudioBuffer<float> buf((int) reader->numChannels, s.numSamples);
            reader->read(&buf, 0, s.numSamples, 0, true, true);

            auto* ch0 = buf.getReadPointer(0);
            int n = s.numSamples;

            // RMS + peak
            float sumSq = 0.0f, peak = 0.0f, sum = 0.0f;
            for (int i = 0; i < n; ++i)
            {
                float v = ch0[i];
                sumSq += v * v;
                peak = std::max(peak, std::abs(v));
                sum += v;
            }
            float rms = std::sqrt(sumSq / std::max(1, n));
            s.rmsDb  = 20.0f * std::log10(std::max(rms, 1e-10f));
            s.peakDb = 20.0f * std::log10(std::max(peak, 1e-10f));
            s.dcOffset = sum / std::max(1, n);

            // Tail length
            float threshold = std::pow(10.0f, -60.0f / 20.0f);
            int lastActive = n - 1;
            for (int i = n - 1; i >= 0; --i)
            {
                if (std::abs(ch0[i]) > threshold) { lastActive = i; break; }
            }
            s.tailLengthS = (float) (n - lastActive) / (float) s.sampleRate;

            // Loop detection for sustained sounds
            if (settings_.detectLoops && s.durationS > 2.0 && !isDrumCategory(s.category))
            {
                int startIdx = (int) (n * 0.2);
                int endIdx   = (int) (n * 0.8);
                int loopStart = startIdx;
                for (int i = startIdx; i < std::min(startIdx + (int) s.sampleRate, endIdx); ++i)
                    if (i > 0 && ch0[i] >= 0.0f && ch0[i - 1] < 0.0f) { loopStart = i; break; }
                int loopEnd = endIdx;
                for (int i = endIdx; i > std::max(endIdx - (int) s.sampleRate, startIdx); --i)
                    if (i > 0 && ch0[i] >= 0.0f && ch0[i - 1] < 0.0f) { loopEnd = i; break; }
                if (loopEnd - loopStart > (int) s.sampleRate)
                {
                    s.isLoopable = true;
                    s.loopStart = loopStart;
                    s.loopEnd = loopEnd;
                }
            }
        }
    }

    //==========================================================================
    // Stage 4: ENHANCE
    //==========================================================================

    std::vector<UpgradedProgram> enhance (const std::vector<AnalyzedSample>& samples,
                                          const juce::File& workDir)
    {
        std::vector<UpgradedProgram> programs;
        auto enhDir = workDir.getChildFile("enhanced");
        enhDir.createDirectory();

        juce::AudioFormatManager fmgr;
        fmgr.registerBasicFormats();

        for (const auto& s : samples)
        {
            std::unique_ptr<juce::AudioFormatReader> reader(fmgr.createReaderFor(s.sourceFile));
            if (!reader) continue;

            int n = (int) reader->lengthInSamples;
            int nch = (int) reader->numChannels;
            juce::AudioBuffer<float> original(nch, n);
            reader->read(&original, 0, n, 0, true, true);

            // DC offset removal
            if (settings_.removeDC)
            {
                for (int ch = 0; ch < nch; ++ch)
                {
                    auto* data = original.getWritePointer(ch);
                    float dc = 0.0f;
                    for (int i = 0; i < n; ++i) dc += data[i];
                    dc /= std::max(1, n);
                    if (std::abs(dc) > 1e-6f)
                        for (int i = 0; i < n; ++i) data[i] -= dc;
                }
            }

            // Fade guards
            if (settings_.applyFadeGuards)
                applyFadeGuards(original, reader->sampleRate);

            UpgradedProgram prog;
            prog.name = s.name.substring(0, 30);
            prog.category = s.category;
            prog.sourceInfo = s;
            prog.numVelocityLayers = settings_.velocityLayers;
            prog.numRoundRobin = settings_.roundRobin;

            // Generate velocity layers × round-robin
            for (int vel = 0; vel < settings_.velocityLayers; ++vel)
            {
                float t = (float) vel / std::max(1, settings_.velocityLayers - 1);

                // Shape: softer layers get amplitude reduction + low-pass
                juce::AudioBuffer<float> shaped(nch, n);
                for (int ch = 0; ch < nch; ++ch)
                {
                    auto* src = original.getReadPointer(ch);
                    auto* dst = shaped.getWritePointer(ch);

                    float amp = 0.2f + 0.8f * t;
                    for (int i = 0; i < n; ++i) dst[i] = src[i] * amp;

                    // Simple one-pole low-pass for soft layers
                    if (t < 0.7f)
                    {
                        float alpha = 0.3f + 0.7f * t;
                        float prev = 0.0f;
                        for (int i = 0; i < n; ++i)
                        {
                            dst[i] = prev + alpha * (dst[i] - prev);
                            prev = dst[i];
                        }
                    }
                }

                // Round-robin variations
                for (int rr = 0; rr < settings_.roundRobin; ++rr)
                {
                    juce::AudioBuffer<float> varied(nch, n);
                    std::mt19937 rng((unsigned) (n * 1000 + vel * 100 + rr));
                    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

                    for (int ch = 0; ch < nch; ++ch)
                    {
                        auto* src = shaped.getReadPointer(ch);
                        auto* dst = varied.getWritePointer(ch);

                        // Micro-pitch (±5 cents via linear interpolation)
                        float cents = dist(rng) * 5.0f;
                        float ratio = std::pow(2.0f, cents / 1200.0f);
                        if (rr == 0) ratio = 1.0f; // original is untouched

                        for (int i = 0; i < n; ++i)
                        {
                            float srcIdx = i * ratio;
                            int i0 = (int) srcIdx;
                            float frac = srcIdx - i0;
                            if (i0 + 1 < n)
                                dst[i] = src[i0] * (1.0f - frac) + src[i0 + 1] * frac;
                            else if (i0 < n)
                                dst[i] = src[i0];
                            else
                                dst[i] = 0.0f;
                        }

                        // Micro-gain (±0.5 dB) for rr > 0
                        if (rr > 0)
                        {
                            float gainDb = dist(rng) * 0.25f;
                            float gain = std::pow(10.0f, gainDb / 20.0f);
                            for (int i = 0; i < n; ++i) dst[i] *= gain;
                        }

                        // Subtle saturation for rr > 0
                        if (rr > 0)
                        {
                            float satAmt = std::abs(dist(rng)) * 0.03f;
                            if (satAmt > 0.01f)
                            {
                                float invTanh = 1.0f / std::tanh(1.0f + satAmt);
                                for (int i = 0; i < n; ++i)
                                    dst[i] = std::tanh(dst[i] * (1.0f + satAmt)) * invTanh;
                            }
                        }
                    }

                    // Write WAV
                    auto fname = s.name.substring(0, 20) + "__v" + juce::String(vel + 1)
                                 + "__c" + juce::String(rr + 1) + ".wav";
                    auto fpath = enhDir.getChildFile(fname);
                    writeWav(fpath, varied, reader->sampleRate, s.bitDepth);

                    EnhancedLayer layer;
                    layer.file = fpath;
                    layer.filename = fname;
                    layer.velLayer = vel;
                    layer.rrIndex = rr;
                    prog.layers.push_back(layer);
                }
            }
            programs.push_back(prog);
        }
        programs_ = programs;
        return programs;
    }

    //==========================================================================
    // Stage 5: NORMALIZE
    //==========================================================================

    void normalize (std::vector<UpgradedProgram>& programs)
    {
        juce::AudioFormatManager fmgr;
        fmgr.registerBasicFormats();

        for (auto& prog : programs)
        {
            // Find the loudest layer (highest vel, first rr)
            EnhancedLayer* loudest = nullptr;
            for (auto& l : prog.layers)
                if (l.rrIndex == 0 && (loudest == nullptr || l.velLayer > loudest->velLayer))
                    loudest = &l;

            if (!loudest) continue;

            // Measure LUFS of loudest
            std::unique_ptr<juce::AudioFormatReader> reader(fmgr.createReaderFor(loudest->file));
            if (!reader) continue;

            int n = (int) reader->lengthInSamples;
            juce::AudioBuffer<float> buf((int) reader->numChannels, n);
            reader->read(&buf, 0, n, 0, true, true);

            float currentLufs = measureLufs(buf, reader->sampleRate);
            if (currentLufs < -80.0f) continue;

            float gainDb = juce::jlimit(-24.0f, 24.0f, settings_.lufsTarget - currentLufs);
            float gain = std::pow(10.0f, gainDb / 20.0f);

            // Apply same gain to ALL layers (preserves velocity dynamics)
            for (auto& l : prog.layers)
            {
                std::unique_ptr<juce::AudioFormatReader> lr(fmgr.createReaderFor(l.file));
                if (!lr) continue;
                int ln = (int) lr->lengthInSamples;
                juce::AudioBuffer<float> lb((int) lr->numChannels, ln);
                lr->read(&lb, 0, ln, 0, true, true);

                for (int ch = 0; ch < lb.getNumChannels(); ++ch)
                {
                    auto* data = lb.getWritePointer(ch);
                    for (int i = 0; i < ln; ++i)
                        data[i] = juce::jlimit(-1.0f, 1.0f, data[i] * gain);
                }
                writeWav(l.file, lb, lr->sampleRate, (int) lr->bitsPerSample);
            }
        }
    }

    //==========================================================================
    // Stage 6: MAP — build XPM programs
    //==========================================================================

    void buildPrograms (std::vector<UpgradedProgram>& programs, const juce::File& workDir)
    {
        auto xpmDir = workDir.getChildFile("programs");
        xpmDir.createDirectory();

        auto splits = getVelocitySplits(settings_.velocityCurve);

        for (auto& prog : programs)
        {
            juce::String xml;
            if (isDrumCategory(prog.category))
                xml = buildDrumXPM(prog, splits);
            else
                xml = buildKeygroupXPM(prog, splits);

            auto xpmFile = xpmDir.getChildFile(prog.name + ".xpm");
            xpmFile.replaceWithText(xml);
        }
    }

    juce::String buildDrumXPM (const UpgradedProgram& prog,
                                const std::vector<VelocitySplit>& splits)
    {
        juce::String xml;
        xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        xml << "<MPCVObject type=\"com.akaipro.mpc.drum.program\">\n";
        xml << "  <Version>1.7</Version>\n";
        xml << "  <ProgramName>" << prog.name << "</ProgramName>\n";
        xml << "  <AfterTouch>\n";
        xml << "    <Destination>FilterCutoff</Destination>\n";
        xml << "    <Amount>30</Amount>\n";
        xml << "  </AfterTouch>\n";
        xml << "  <Instruments>\n";

        int note = gmNoteForCategory(prog.category);
        xml << "    <Instrument index=\"0\">\n";
        xml << "      <Note>" << note << "</Note>\n";
        xml << "      <TriggerMode>OneShot</TriggerMode>\n";
        xml << "      <PadColor>#E9C46A</PadColor>\n";

        if (prog.category == SampleCategory::HiHatClosed ||
            prog.category == SampleCategory::HiHatOpen)
            xml << "      <MuteGroup>1</MuteGroup>\n";

        xml << "      <Layers>\n";
        int layerIdx = 0;
        for (int v = 0; v < prog.numVelocityLayers && v < (int) splits.size(); ++v)
        {
            auto rrs = getLayersForVel(prog.layers, v);
            for (const auto& l : rrs)
            {
                xml << "        <Layer index=\"" << layerIdx++ << "\">\n";
                xml << "          <SampleName>" << l->filename << "</SampleName>\n";
                xml << "          <VelStart>" << splits[v].start << "</VelStart>\n";
                xml << "          <VelEnd>" << splits[v].end << "</VelEnd>\n";
                xml << "          <Volume>" << juce::String(splits[v].volume, 2) << "</Volume>\n";
                xml << "          <RootNote>0</RootNote>\n";
                xml << "          <KeyTrack>True</KeyTrack>\n";
                if (rrs.size() > 1)
                {
                    xml << "          <CycleType>RoundRobin</CycleType>\n";
                    xml << "          <CycleGroup>" << (v + 1) << "</CycleGroup>\n";
                }
                xml << "        </Layer>\n";
            }
        }
        xml << "      </Layers>\n";
        xml << "    </Instrument>\n";
        xml << "  </Instruments>\n";
        xml << "</MPCVObject>\n";
        return xml;
    }

    juce::String buildKeygroupXPM (const UpgradedProgram& prog,
                                    const std::vector<VelocitySplit>& splits)
    {
        juce::String xml;
        xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        xml << "<MPCVObject type=\"com.akaipro.mpc.keygroup.program\">\n";
        xml << "  <Version>1.7</Version>\n";
        xml << "  <ProgramName>" << prog.name << "</ProgramName>\n";
        xml << "  <AfterTouch>\n";
        xml << "    <Destination>FilterCutoff</Destination>\n";
        xml << "    <Amount>50</Amount>\n";
        xml << "  </AfterTouch>\n";
        xml << "  <ModWheel>\n";
        xml << "    <Destination>FilterCutoff</Destination>\n";
        xml << "    <Amount>70</Amount>\n";
        xml << "  </ModWheel>\n";
        xml << "  <PitchBendRange>12</PitchBendRange>\n";
        xml << "  <Keygroups>\n";
        xml << "    <Keygroup index=\"0\">\n";
        xml << "      <LowNote>0</LowNote>\n";
        xml << "      <HighNote>127</HighNote>\n";
        xml << "      <Layers>\n";

        int layerIdx = 0;
        for (int v = 0; v < prog.numVelocityLayers && v < (int) splits.size(); ++v)
        {
            auto rrs = getLayersForVel(prog.layers, v);
            for (const auto& l : rrs)
            {
                xml << "        <Layer index=\"" << layerIdx++ << "\">\n";
                xml << "          <SampleName>" << l->filename << "</SampleName>\n";
                xml << "          <VelStart>" << splits[v].start << "</VelStart>\n";
                xml << "          <VelEnd>" << splits[v].end << "</VelEnd>\n";
                xml << "          <Volume>" << juce::String(splits[v].volume, 2) << "</Volume>\n";
                xml << "          <RootNote>0</RootNote>\n";
                xml << "          <KeyTrack>True</KeyTrack>\n";
                xml << "          <TuneCoarse>0</TuneCoarse>\n";
                xml << "          <TuneFine>0</TuneFine>\n";
                if (prog.sourceInfo.isLoopable)
                {
                    xml << "          <LoopStart>" << prog.sourceInfo.loopStart << "</LoopStart>\n";
                    xml << "          <LoopEnd>" << prog.sourceInfo.loopEnd << "</LoopEnd>\n";
                    xml << "          <LoopCrossfade>100</LoopCrossfade>\n";
                }
                else
                {
                    xml << "          <LoopStart>-1</LoopStart>\n";
                    xml << "          <LoopEnd>-1</LoopEnd>\n";
                }
                if (rrs.size() > 1)
                {
                    xml << "          <CycleType>RoundRobin</CycleType>\n";
                    xml << "          <CycleGroup>" << (v + 1) << "</CycleGroup>\n";
                }
                xml << "        </Layer>\n";
            }
        }
        xml << "      </Layers>\n";
        xml << "    </Keygroup>\n";
        xml << "  </Keygroups>\n";
        xml << "</MPCVObject>\n";
        return xml;
    }

    //==========================================================================
    // Stage 7: PACKAGE
    //==========================================================================

    bool packageXPN (const std::vector<UpgradedProgram>& programs,
                     const juce::File& workDir, const juce::File& output)
    {
        juce::ZipFile::Builder builder;

        // Manifest
        juce::String manifest;
        manifest << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        manifest << "<Expansion>\n";
        manifest << "  <Name>" << settings_.packName << "</Name>\n";
        manifest << "  <Author>XOutshine by XO_OX Designs</Author>\n";
        manifest << "  <Version>1.0</Version>\n";
        manifest << "  <Description>Upgraded by XOutshine</Description>\n";
        manifest << "  <ProgramCount>" << (int) programs.size() << "</ProgramCount>\n";
        manifest << "</Expansion>\n";

        auto manifestFile = workDir.getChildFile("Manifest.xml");
        manifestFile.replaceWithText(manifest);
        builder.addFile(manifestFile, 9, "Expansions/Manifest.xml");

        auto xpmDir = workDir.getChildFile("programs");
        for (const auto& prog : programs)
        {
            auto xpmFile = xpmDir.getChildFile(prog.name + ".xpm");
            if (xpmFile.existsAsFile())
                builder.addFile(xpmFile, 9, "Programs/" + prog.name + ".xpm");

            for (const auto& l : prog.layers)
                if (l.file.existsAsFile())
                    builder.addFile(l.file, 9, "Samples/" + prog.name + "/" + l.filename);
        }

        juce::FileOutputStream fos(output);
        if (!fos.openedOk()) return false;
        return builder.writeToStream(fos, nullptr) != false;
    }

    bool packageFolder (const std::vector<UpgradedProgram>& programs,
                        const juce::File& workDir, const juce::File& output)
    {
        output.createDirectory();

        auto progDir = output.getChildFile("Programs");
        progDir.createDirectory();

        auto xpmDir = workDir.getChildFile("programs");
        for (const auto& prog : programs)
        {
            auto xpmFile = xpmDir.getChildFile(prog.name + ".xpm");
            if (xpmFile.existsAsFile())
                xpmFile.copyFileTo(progDir.getChildFile(prog.name + ".xpm"));

            auto sampDir = output.getChildFile("Samples").getChildFile(prog.name);
            sampDir.createDirectory();
            for (const auto& l : prog.layers)
                if (l.file.existsAsFile())
                    l.file.copyFileTo(sampDir.getChildFile(l.filename));
        }
        return true;
    }

    //==========================================================================
    // Stage 8: VALIDATE
    //==========================================================================

    int validate (const std::vector<UpgradedProgram>& programs)
    {
        int issues = 0;
        juce::AudioFormatManager fmgr;
        fmgr.registerBasicFormats();

        for (const auto& prog : programs)
        {
            for (const auto& l : prog.layers)
            {
                std::unique_ptr<juce::AudioFormatReader> reader(fmgr.createReaderFor(l.file));
                if (!reader) { issues++; continue; }

                int n = (int) reader->lengthInSamples;
                juce::AudioBuffer<float> buf((int) reader->numChannels, n);
                reader->read(&buf, 0, n, 0, true, true);

                float peak = buf.getMagnitude(0, n);
                if (peak >= 1.0f) issues++;

                // DC check
                auto* ch0 = buf.getReadPointer(0);
                float sum = 0.0f;
                for (int i = 0; i < n; ++i) sum += ch0[i];
                if (std::abs(sum / n) > 0.005f) issues++;

                // Fade check
                if (n > 10 && std::abs(ch0[0]) > 0.01f) issues++;
            }
        }
        return issues;
    }

    //==========================================================================
    // Utility
    //==========================================================================

    static void applyFadeGuards (juce::AudioBuffer<float>& buf, double sr)
    {
        int n = buf.getNumSamples();
        int fadeIn  = std::min((int)(sr * 0.002), n);  // 2ms
        int fadeOut = std::min((int)(sr * 0.010), n);   // 10ms

        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            auto* data = buf.getWritePointer(ch);
            for (int i = 0; i < fadeIn; ++i)
            {
                float g = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * i / fadeIn));
                data[i] *= g;
            }
            for (int i = 0; i < fadeOut; ++i)
            {
                float g = 0.5f * (1.0f + std::cos(juce::MathConstants<float>::pi * i / fadeOut));
                data[n - 1 - i] *= g;
            }
        }
    }

    static float measureLufs (const juce::AudioBuffer<float>& buf, double sr)
    {
        auto* ch0 = buf.getReadPointer(0);
        int n = buf.getNumSamples();
        int window = (int)(0.4 * sr);
        if (n < window) return -100.0f;

        std::vector<float> powers;
        for (int i = 0; i <= n - window; i += window / 4)
        {
            float ms = 0.0f;
            for (int j = 0; j < window; ++j) ms += ch0[i + j] * ch0[i + j];
            powers.push_back(ms / window);
        }
        if (powers.empty()) return -100.0f;

        float absGate = std::pow(10.0f, (-70.0f + 0.691f) / 10.0f);
        std::vector<float> gated;
        for (float p : powers) if (p > absGate) gated.push_back(p);
        if (gated.empty()) return -100.0f;

        float meanUngated = 0.0f;
        for (float p : gated) meanUngated += p;
        meanUngated /= gated.size();

        float relGate = meanUngated * std::pow(10.0f, -10.0f / 10.0f);
        std::vector<float> final;
        for (float p : gated) if (p > relGate) final.push_back(p);
        if (final.empty()) return -100.0f;

        float meanFinal = 0.0f;
        for (float p : final) meanFinal += p;
        meanFinal /= final.size();

        return 10.0f * std::log10(std::max(meanFinal, 1e-10f)) - 0.691f;
    }

    void writeWav (const juce::File& file, const juce::AudioBuffer<float>& buf,
                   double sr, int bitDepth)
    {
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> writer(
            wav.createWriterFor(new juce::FileOutputStream(file),
                                sr, (unsigned int) buf.getNumChannels(),
                                bitDepth, {}, 0));
        if (writer)
        {
            if (settings_.applyDither && bitDepth <= 24)
            {
                // TPDF dithered write
                juce::AudioBuffer<float> dithered(buf);
                std::mt19937 rng((unsigned int) buf.getNumSamples());
                std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
                float scale = 1.0f / std::pow(2.0f, (float)(bitDepth - 1));
                for (int ch = 0; ch < dithered.getNumChannels(); ++ch)
                {
                    auto* data = dithered.getWritePointer(ch);
                    for (int i = 0; i < dithered.getNumSamples(); ++i)
                    {
                        float d = (dist(rng) + dist(rng)) * 0.5f * scale;
                        data[i] = juce::jlimit(-1.0f, 1.0f, data[i] + d);
                    }
                }
                writer->writeFromAudioSampleBuffer(dithered, 0, dithered.getNumSamples());
            }
            else
            {
                writer->writeFromAudioSampleBuffer(buf, 0, buf.getNumSamples());
            }
        }
    }

    static std::vector<const EnhancedLayer*> getLayersForVel (
        const std::vector<EnhancedLayer>& layers, int velLayer)
    {
        std::vector<const EnhancedLayer*> result;
        for (const auto& l : layers)
            if (l.velLayer == velLayer) result.push_back(&l);
        return result;
    }
};

} // namespace xomnibus
