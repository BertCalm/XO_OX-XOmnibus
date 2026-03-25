#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>

#include "../DSP/FastMath.h"
#include "XPNExporter.h"
#include "XPNVelocityCurves.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <functional>
#include <random>
#include <vector>

namespace xolokun {

//==============================================================================
//  XOutshine — Sample Pack Upgrade Engine
//  "Outshine the original."
//
//  Integrated into the XOlokun desktop application. Ingests any WAV samples
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
    // Drum / percussive
    Kick, Snare, HiHatClosed, HiHatOpen, Clap, Tom, Percussion, FX,
    // Structural
    Loop,
    // Melodic
    Bass, Pad, Lead, Keys, Pluck, String, Woodwind, Brass, Vocal,
    // Fallback
    Unknown
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
        case SampleCategory::Loop:         return 60;
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
        case SampleCategory::Loop:         return "Loop";
        case SampleCategory::Woodwind:     return "Woodwind";
        case SampleCategory::Brass:        return "Brass";
        case SampleCategory::Vocal:        return "Vocal";
        default:                           return "Unknown";
    }
}

//------------------------------------------------------------------------------
// Velocity curve presets — defined in XPNVelocityCurves.h (shared with XOriginate)
//------------------------------------------------------------------------------
// VelocitySplit, XPNVelocityCurve, and getVelocitySplits() are available via the
// XPNVelocityCurves.h include above.

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

    int    detectedMidiNote = 60;  // YIN-detected root note (MIDI 0-127), default C4
    float  pitchConfidence = 0.0f; // YIN confidence (0-1, higher = more confident)
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
// Progress callback — structured to match XOriginate's pattern
//------------------------------------------------------------------------------

struct OutshineProgress {
    int          currentSample = 0;
    int          totalSamples  = 0;
    juce::String stage;
    float        overallProgress = 0.0f;  // 0-1
    bool         cancelled = false;
};

using ProgressCallback = std::function<void (OutshineProgress&)>;

//------------------------------------------------------------------------------
// Export format selection
//------------------------------------------------------------------------------

enum class ExportFormat { XPNPack, WAVFolder, XPMOnly };

//------------------------------------------------------------------------------
// MPE expression route configuration (for UI — not embedded in XPM)
//------------------------------------------------------------------------------

struct MPEExpressionRoutes
{
    float slideAmount    = 0.8f;   // CC74 → Filter Cutoff
    float pressureAmount = 0.4f;   // Pressure → Filter Resonance
    float pitchBendRange = 24;     // ±semitones
    bool  velocityToFilter = true; // Velocity → brightness
    float modWheelAmount = 0.5f;   // ModWheel → Filter Cutoff
};

//------------------------------------------------------------------------------
// XOutshine configuration
//------------------------------------------------------------------------------

struct OutshineSettings
{
    int            velocityLayers = 4;
    int            roundRobin     = 4;
    XPNVelocityCurve  velocityCurve  = XPNVelocityCurve::Musical;
    // Per-category LUFS targets (spec Section 6, Stage 5)
    float lufsTargetDrum       = -14.0f;  // Kick, Snare, HiHat*, Clap, Tom, Percussion
    float lufsTargetBass       = -16.0f;  // Bass
    float lufsTargetPad        = -18.0f;  // Pad, String
    float lufsTargetKeys       = -14.0f;  // Keys, Lead, Woodwind, Brass, Pluck, FX, Loop
    float lufsTargetVocal      = -16.0f;  // Vocal

    // Per-category true-peak ceilings (dBTP, negative values)
    float truePeakDrum         = -0.5f;   // Kick, Snare, HiHat*, Clap, Tom, Percussion, Keys
    float truePeakLeadPluck    = -0.4f;   // Lead, Pluck, Woodwind, Brass, FX, Loop
    float truePeakPadBassVocal = -0.3f;   // Pad, String, Vocal, Bass

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

    ~XOutshine()
    {
        if (workDir_.isDirectory())
            workDir_.deleteRecursively();
    }

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
        OutshineSettings s = settings;
        if (s.packName.isEmpty())
            s.packName = inputPath.getFileNameWithoutExtension();

        // Stage 1: INGEST — handle .xpn and directory inputs.
        // analyzeGrains() takes a flat file list; run() must expand the input first.
        juce::File tempWorkDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile("xoutshine_run_" + juce::String(juce::Random::getSystemRandom().nextInt()));
        tempWorkDir.createDirectory();

        auto ingested = ingest(inputPath, tempWorkDir);
        if (ingested.empty()) { tempWorkDir.deleteRecursively(); return false; }

        // Build a StringArray of the ingested WAV paths for analyzeGrains()
        juce::StringArray paths;
        for (const auto& sample : ingested)
            paths.add(sample.sourceFile.getFullPathName());

        // analyzeGrains() manages its own workDir_ — clean up the temporary ingest dir first
        tempWorkDir.deleteRecursively();

        if (!analyzeGrains(paths, s, progress))
            return false;

        return exportPearl(outputPath, s, progress);
    }

    //--------------------------------------------------------------------------
    // Access results after run() / analyzeGrains() / exportPearl()
    //--------------------------------------------------------------------------
    int getNumPrograms() const { return (int) programs_.size(); }
    const juce::StringArray& getErrors() const { return errors_; }

    // Returns the samples analyzed by the last analyzeGrains() call.
    // Empty until analyzeGrains() has been called successfully.
    const std::vector<AnalyzedSample>& getAnalyzedSamples() const { return analyzedSamples_; }

    // Returns the upgraded programs produced by the last exportPearl() call.
    // Empty until exportPearl() has been called successfully.
    const std::vector<UpgradedProgram>& getPrograms() const { return programs_; }

    //--------------------------------------------------------------------------
    // Stage 1-3: Ingest + Classify + Analyze.
    // Populates analyzedSamples_ for preview panel access.
    // filePaths: flat list of WAV file paths (caller resolves folders/XPNs before calling).
    // Returns true on success (even if some samples produced warnings).
    //--------------------------------------------------------------------------
    bool analyzeGrains (const juce::StringArray& filePaths,
                        const OutshineSettings& settings,
                        ProgressCallback progress = nullptr)
    {
        settings_ = settings;
        progress_ = progress;
        cancelRequested_.store(false);
        errors_.clear();
        analyzedSamples_.clear();
        programs_.clear();

        // Clean up any previous workDir
        if (workDir_.isDirectory())
            workDir_.deleteRecursively();

        workDir_ = juce::File::getSpecialLocation(juce::File::tempDirectory)
                       .getChildFile("xoutshine_" + juce::String(juce::Random::getSystemRandom().nextInt()));
        workDir_.createDirectory();

        auto wavDir = workDir_.getChildFile("ingested");
        wavDir.createDirectory();

        // Build AnalyzedSample list from flat file list (files already on disk — no unzip needed)
        std::vector<AnalyzedSample> samples;
        for (const auto& path : filePaths)
        {
            if (cancelRequested_.load()) { workDir_.deleteRecursively(); return false; }
            juce::File f(path);
            if (!f.existsAsFile()) continue;

            // Copy to workDir for consistent handling
            auto dest = wavDir.getChildFile(f.getFileName());
            f.copyFileTo(dest);

            AnalyzedSample as;
            as.sourceFile = dest;
            as.name = dest.getFileNameWithoutExtension();
            samples.push_back(as);
        }

        if (samples.empty()) return false;

        progressState_ = OutshineProgress{};
        report(0.0f, "Opening grains...");
        progressState_.totalSamples = (int) samples.size();

        if (cancelRequested_.load()) { workDir_.deleteRecursively(); return false; }

        report(0.15f, "Classifying " + juce::String(samples.size()) + " samples...");
        classify(samples);

        if (cancelRequested_.load()) { workDir_.deleteRecursively(); return false; }

        report(0.25f, "Analyzing root notes...");
        analyze(samples);

        if (cancelRequested_.load()) { workDir_.deleteRecursively(); return false; }

        analyzedSamples_ = samples;
        report(1.0f, "Analysis complete — " + juce::String(samples.size()) + " samples ready.");
        return true;
    }

    //--------------------------------------------------------------------------
    // Stage 4-8: Enhance + Normalize + Map + Package + Validate.
    // Operates on the retained analyzedSamples_ from the last analyzeGrains() call.
    // outputPath: .xpn file or folder.
    // Returns true on success.
    //--------------------------------------------------------------------------
    bool exportPearl (const juce::File& outputPath,
                      const OutshineSettings& settings,
                      ProgressCallback progress = nullptr)
    {
        if (analyzedSamples_.empty())
        {
            errors_.add("exportPearl() called before analyzeGrains() — no samples to export.");
            return false;
        }

        settings_ = settings;
        progress_ = progress;
        cancelRequested_.store(false);

        if (settings_.packName.isEmpty())
            settings_.packName = analyzedSamples_[0].name;

        // Reuse workDir_ from analyzeGrains(). If it was cleaned up, fail gracefully.
        if (!workDir_.isDirectory())
        {
            errors_.add("exportPearl() called but workDir no longer exists. Call analyzeGrains() again.");
            return false;
        }

        bool ok = true;

        report(0.35f, "Enhancing — velocity layers + round-robin...");
        auto programs = enhance(analyzedSamples_, workDir_);
        if (cancelRequested_.load()) { workDir_.deleteRecursively(); return false; }

        report(0.60f, "LUFS normalizing...");
        normalize(programs);
        if (cancelRequested_.load()) { workDir_.deleteRecursively(); return false; }

        report(0.75f, "Building XPM programs...");
        buildPrograms(programs, workDir_);
        if (cancelRequested_.load()) { workDir_.deleteRecursively(); return false; }

        report(0.85f, "Packaging...");
        if (outputPath.getFileExtension().equalsIgnoreCase(".xpn"))
            ok = packageXPN(programs, workDir_, outputPath);
        else
            ok = packageFolder(programs, workDir_, outputPath);

        if (cancelRequested_.load())
        {
            workDir_.deleteRecursively();
            return false;
        }

        report(0.95f, "Validating...");
        int issues = validate(programs);

        report(1.0f, "Pearl complete — " + juce::String(programs.size())
               + " programs, " + juce::String(issues) + " issues.");

        programs_ = programs;

        // Clean up workDir after successful export
        workDir_.deleteRecursively();
        return ok && issues == 0 && !cancelRequested_.load();
    }

    //--------------------------------------------------------------------------
    // Signal the current analyzeGrains() or exportPearl() call to abort.
    // Safe to call from any thread.
    //--------------------------------------------------------------------------
    void cancel() { cancelRequested_.store(true); }

private:
    OutshineSettings settings_;
    ProgressCallback progress_;
    OutshineProgress progressState_;
    std::vector<UpgradedProgram> programs_;
    juce::StringArray errors_;

    // Staged API state
    std::vector<AnalyzedSample> analyzedSamples_;   // populated by analyzeGrains()
    juce::File workDir_;                            // retained between analyzeGrains() and exportPearl()
    std::atomic<bool> cancelRequested_{ false };    // set true to abort current operation

    void report (float p, const juce::String& msg)
    {
        if (progress_)
        {
            progressState_.overallProgress = p;
            progressState_.stage = msg;
            progress_(progressState_);
        }
    }

    // Returns the LUFS target for a given category (per spec Section 6, Stage 5 table).
    float getLufsTarget (SampleCategory c) const
    {
        switch (c) {
            case SampleCategory::Kick:
            case SampleCategory::Snare:
            case SampleCategory::HiHatClosed:
            case SampleCategory::HiHatOpen:
            case SampleCategory::Clap:
            case SampleCategory::Tom:
            case SampleCategory::Percussion:
                return settings_.lufsTargetDrum;

            case SampleCategory::Bass:
                return settings_.lufsTargetBass;

            case SampleCategory::Pad:
            case SampleCategory::String:
                return settings_.lufsTargetPad;

            case SampleCategory::Keys:
            case SampleCategory::Lead:
            case SampleCategory::Woodwind:
            case SampleCategory::Brass:
            case SampleCategory::Pluck:
            case SampleCategory::FX:
            case SampleCategory::Loop:
                return settings_.lufsTargetKeys;

            case SampleCategory::Vocal:
                return settings_.lufsTargetVocal;

            default:
                return settings_.lufsTargetKeys;  // Unknown falls back to −14
        }
    }

    // Returns the true-peak ceiling (dBTP) for a given category.
    float getTruePeakCeiling (SampleCategory c) const
    {
        switch (c) {
            case SampleCategory::Kick:
            case SampleCategory::Snare:
            case SampleCategory::HiHatClosed:
            case SampleCategory::HiHatOpen:
            case SampleCategory::Clap:
            case SampleCategory::Tom:
            case SampleCategory::Percussion:
            case SampleCategory::Keys:
                return settings_.truePeakDrum;

            case SampleCategory::Lead:
            case SampleCategory::Pluck:
            case SampleCategory::Woodwind:
            case SampleCategory::Brass:
            case SampleCategory::FX:
            case SampleCategory::Loop:
                return settings_.truePeakLeadPluck;

            case SampleCategory::Pad:
            case SampleCategory::String:
            case SampleCategory::Vocal:
            case SampleCategory::Bass:
                return settings_.truePeakPadBassVocal;

            default:
                return settings_.truePeakPadBassVocal;
        }
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
        if (nl.contains("loop") || nl.contains(" lp") || nl.contains("_lp") || nl.contains("phrase"))
            return SampleCategory::Loop;
        if (nl.contains("flute") || nl.contains("clarinet") || nl.contains("saxophone")
            || nl.contains("_sax") || nl.contains(" sax") || nl.contains("oboe")
            || nl.contains("bassoon") || nl.contains("recorder") || nl.contains("piccolo")
            || nl.contains("fife"))
            return SampleCategory::Woodwind;
        if (nl.contains("trumpet") || nl.contains("trombone") || nl.contains("french horn")
            || nl.contains("frenchhorn") || nl.contains("tuba") || nl.contains("cornet")
            || nl.contains("flugelhorn") || nl.contains("_horn") || nl.contains(" horn"))
            return SampleCategory::Brass;
        if (nl.contains("vocal") || nl.contains("voice") || nl.contains("choir")
            || nl.contains("singing") || nl.contains("_vox") || nl.contains(" vox")
            || nl.contains("acapella") || nl.contains("spoken"))
            return SampleCategory::Vocal;
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

    //--------------------------------------------------------------------------
    // YIN pitch detection (de Cheveigne & Kawahara 2002)
    // Returns MIDI note (0-127) on success, -1 on failure.
    // confidence is set to 1 - CMNDF minimum (higher = more confident).
    //--------------------------------------------------------------------------
    static int yinDetectPitch (const float* data, int numSamples, double sampleRate, float& confidence)
    {
        confidence = 0.0f;
        int minBuffer = (int)(2.0 * sampleRate / 40.0);
        if (numSamples < minBuffer || numSamples < 512)
            return -1;

        int W = std::min(numSamples / 2, 4096);

        // Step 1: Difference function
        std::vector<double> d(W, 0.0);
        for (int tau = 1; tau < W; ++tau)
        {
            double sum = 0.0;
            for (int j = 0; j < W; ++j)
            {
                double diff = (double)data[j] - (double)data[j + tau];
                sum += diff * diff;
            }
            d[tau] = sum;
        }

        // Step 2: Cumulative mean normalized difference function
        std::vector<double> cmndf(W, 0.0);
        cmndf[0] = 1.0;
        double runningSum = 0.0;
        for (int tau = 1; tau < W; ++tau)
        {
            runningSum += d[tau];
            cmndf[tau] = (runningSum > 0.0) ? (d[tau] * tau / runningSum) : 1.0;
        }

        // Step 3: Absolute threshold (0.15)
        constexpr double kThreshold = 0.15;
        constexpr int kMinTau = 2;
        int tauMin = (int)(sampleRate / 2000.0);
        int tauMax = (int)(sampleRate / 40.0);
        tauMin = std::max(tauMin, kMinTau);
        tauMax = std::min(tauMax, W - 1);

        int bestTau = -1;
        double bestVal = 1.0;
        for (int tau = tauMin; tau <= tauMax; ++tau)
        {
            if (cmndf[tau] < kThreshold)
            {
                bestTau = tau;
                bestVal = cmndf[tau];
                break;
            }
            if (cmndf[tau] < bestVal)
            {
                bestVal = cmndf[tau];
                bestTau = tau;
            }
        }

        if (bestTau <= 0)
            return -1;

        // Step 4: Parabolic interpolation
        double tau0 = (double)bestTau;
        if (bestTau > tauMin && bestTau < tauMax)
        {
            double alpha = cmndf[bestTau - 1];
            double beta  = cmndf[bestTau];
            double gamma = cmndf[bestTau + 1];
            double denom = alpha - 2.0 * beta + gamma;
            if (std::abs(denom) > 1e-10)
                tau0 = bestTau - (gamma - alpha) / (2.0 * denom);
        }

        if (tau0 <= 0.0)
            return -1;

        double frequency = sampleRate / tau0;
        if (frequency < 20.0 || frequency > 20000.0)
            return -1;

        double midiNote = 69.0 + 12.0 * std::log2(frequency / 440.0);
        int midiNoteInt = (int)std::round(midiNote);
        if (midiNoteInt < 0 || midiNoteInt > 127)
            return -1;

        confidence = (float)juce::jlimit(0.0, 1.0, 1.0 - bestVal);
        return midiNoteInt;
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

            // Sample rate guard — spec requires: pass ≤48 kHz, downsample 96 kHz, reject >96 kHz
            if (s.sampleRate > 96000.0)
            {
                errors_.add("\"" + s.name + "\": sample rate "
                    + juce::String((int)(s.sampleRate / 1000)) + " kHz is not supported. "
                    + "Maximum is 96 kHz. File will be skipped.");
                // Mark as invalid so downstream stages can skip it
                s.bitDepth = -1;  // sentinel: invalid sample
                continue;
            }

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
            s.rmsDb  = gainToDb(std::max(rms, 1e-10f));
            s.peakDb = gainToDb(std::max(peak, 1e-10f));
            s.dcOffset = sum / std::max(1, n);

            // Tail length
            float threshold = dbToGain(-60.0f);
            int lastActive = n - 1;
            for (int i = n - 1; i >= 0; --i)
            {
                if (std::abs(ch0[i]) > threshold) { lastActive = i; break; }
            }
            s.tailLengthS = (float) (n - lastActive) / (float) s.sampleRate;

            // YIN pitch detection — skip drums and FX
            if (!isDrumCategory(s.category) && s.category != SampleCategory::FX)
            {
                int detectLen = std::min(n, (int)(4.0 * s.sampleRate));
                std::vector<float> mono(detectLen, 0.0f);
                int nchMix = buf.getNumChannels();
                for (int ch = 0; ch < nchMix; ++ch)
                {
                    auto* src = buf.getReadPointer(ch);
                    for (int i = 0; i < detectLen; ++i)
                        mono[i] += src[i] / (float)nchMix;
                }

                float confidence = 0.0f;
                int detected = yinDetectPitch(mono.data(), detectLen, s.sampleRate, confidence);
                if (detected >= 0)
                {
                    s.detectedMidiNote = detected;
                    s.pitchConfidence  = confidence;
                }
                else
                {
                    s.detectedMidiNote = 60;
                    s.pitchConfidence  = 0.0f;
                    errors_.add("YIN detection failed for \"" + s.name + "\" — using C4 as provisional root.");
                }
            }

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

        // Channel validation: all melodic samples must have matching channel counts.
        // Mixed mono/stereo causes MPC to reject the program.
        {
            int melodicChannels = -1;  // -1 = not yet set
            bool mismatchFound = false;

            for (auto& s : samples)
            {
                if (s.bitDepth < 0) continue;  // already flagged invalid (Task 4 sentinel)
                if (isDrumCategory(s.category)) continue;  // drums map to separate pads; no constraint

                if (melodicChannels < 0)
                {
                    melodicChannels = s.numChannels;
                }
                else if (s.numChannels != melodicChannels)
                {
                    mismatchFound = true;
                    errors_.add("Channel mismatch: \"" + s.name + "\" is "
                        + juce::String(s.numChannels) + "-channel but other melodic samples are "
                        + juce::String(melodicChannels) + "-channel. "
                        + "MPC requires all layers in a keygroup to match. "
                        + "Mixed-channel samples will be excluded.");
                    s.bitDepth = -1;  // reuse sentinel to exclude from enhance()
                }
            }

            if (mismatchFound)
            {
                errors_.add("Channel validation failed: some samples excluded. "
                            "Convert all melodic samples to the same channel count before running Outshine.");
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
            // Skip samples that were flagged invalid in analyze()
            if (s.bitDepth < 0) continue;

            std::unique_ptr<juce::AudioFormatReader> reader(fmgr.createReaderFor(s.sourceFile));
            if (!reader) continue;

            int n = (int) reader->lengthInSamples;
            int nch = (int) reader->numChannels;
            juce::AudioBuffer<float> original(nch, n);
            reader->read(&original, 0, n, 0, true, true);

            // DC offset removal — inline implementation
            if (settings_.removeDC)
            {
                for (int ch = 0; ch < nch; ++ch)
                {
                    auto* data = original.getWritePointer(ch);
                    float dc = 0.0f;
                    for (int i = 0; i < n; ++i) dc += data[i];
                    dc /= (float)n;
                    for (int i = 0; i < n; ++i) data[i] -= dc;
                }
            }

            // Fade guards — 64-sample linear fade in/out to prevent clicks
            if (settings_.applyFadeGuards)
            {
                const int fadeLen = juce::jmin(64, n / 2);
                for (int ch = 0; ch < nch; ++ch)
                {
                    auto* data = original.getWritePointer(ch);
                    for (int i = 0; i < fadeLen; ++i)
                    {
                        float g = (float)i / (float)fadeLen;
                        data[i] *= g;
                        data[n - 1 - i] *= g;
                    }
                }
            }

            UpgradedProgram prog;
            prog.name = sanitizeForFAT32(s.name.substring(0, 20));
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

                    // Smooth velocity-to-filter taper (raised cosine, avoids brightness jump at layer boundary)
                    float filterAmount = 1.0f; // 1.0 = no filtering (passthrough)
                    if (t < 0.6f)
                        filterAmount = 0.3f + 0.7f * (t / 0.6f); // linear ramp 0.3→1.0 over 0-0.6
                    else if (t < 0.8f)
                        filterAmount = 0.5f * (1.0f + std::cos(juce::MathConstants<float>::pi * (t - 0.6f) / 0.2f)); // cosine taper
                    // else filterAmount stays 1.0 (no filtering for t >= 0.8)
                    if (filterAmount < 1.0f)
                    {
                        float alpha = filterAmount;
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

                        // Micro-pitch (±3 cents via linear interpolation)
                        float cents = dist(rng) * 3.0f;
                        float ratio = fastPow2(cents / 1200.0f);
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
                            float gainDb = dist(rng) * 0.5f;
                            float gain = dbToGain(gainDb);
                            for (int i = 0; i < n; ++i) dst[i] *= gain;
                        }

                        // Subtle saturation for rr > 0
                        if (rr > 0)
                        {
                            float satAmt = std::abs(dist(rng)) * 0.03f;
                            if (satAmt > 0.01f)
                            {
                                float invTanh = 1.0f / fastTanh(1.0f + satAmt);
                                for (int i = 0; i < n; ++i)
                                    dst[i] = fastTanh(dst[i] * (1.0f + satAmt)) * invTanh;
                            }
                        }
                    }

                    // Write WAV
                    auto fname = sanitizeForFAT32(s.name.substring(0, 20)) + "__v" + juce::String(vel + 1)
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

            float lufsTarget = getLufsTarget(prog.category);
            float gainDb = juce::jlimit(-24.0f, 24.0f, lufsTarget - currentLufs);
            float gain = dbToGain(gainDb);

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

                // Apply true-peak ceiling (simple peak clamp — not an oversampled true-peak limiter,
                // but sufficient for Phase 1A. Phase 1B should use a 4x oversampled limiter.)
                float ceiling = dbToGain(getTruePeakCeiling(prog.category));
                float layerPeak = lb.getMagnitude(0, lb.getNumSamples());
                if (layerPeak > ceiling && layerPeak > 0.0f)
                {
                    float limitGain = ceiling / layerPeak;
                    lb.applyGain(limitGain);
                }

                // Bit depth: always write 24-bit integer.
                // 32-bit float → 24-bit: scale and truncate (dither applied in writeWav).
                // 16-bit → 24-bit: zero-pad lower 8 bits (no dither needed on promotion).
                int targetBitDepth = 24;

                // Sample rate: downsample 96 kHz → 48 kHz via 2:1 decimation with LP filter.
                // 44.1 kHz and 48 kHz pass through unchanged.
                double targetSampleRate = lr->sampleRate;
                if (lr->sampleRate > 48001.0)  // 96 kHz path
                {
                    juce::AudioBuffer<float> downsampled = downsampleBy2(lb);
                    targetSampleRate = lr->sampleRate / 2.0;
                    writeWav(l.file, downsampled, targetSampleRate, targetBitDepth);
                }
                else
                {
                    writeWav(l.file, lb, targetSampleRate, targetBitDepth);
                }
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

        // Separate drum programs (one XPM per pad) from melodic programs (one XPM for all zones)
        std::vector<UpgradedProgram*> drumProgs;
        std::vector<UpgradedProgram*> melodicProgs;
        for (auto& prog : programs)
        {
            if (isDrumCategory(prog.category))
                drumProgs.push_back(&prog);
            else
                melodicProgs.push_back(&prog);
        }

        // Write one drum XPM per drum program (unchanged behavior)
        for (auto* prog : drumProgs)
        {
            auto xpmFile = xpmDir.getChildFile(prog->name + ".xpm");
            xpmFile.replaceWithText(buildDrumXPM(*prog, splits));
        }

        // Write one keygroup XPM for all melodic programs combined
        if (!melodicProgs.empty())
        {
            // Use the pack name as the program name; fall back to first sample name
            juce::String melodicName = settings_.packName.isEmpty()
                ? melodicProgs[0]->name : settings_.packName;
            auto xpmFile = xpmDir.getChildFile(sanitizeForFAT32(melodicName) + ".xpm");
            xpmFile.replaceWithText(buildKeygroupXPM(melodicProgs, splits, melodicName));
        }
    }

    juce::String buildDrumXPM (const UpgradedProgram& prog,
                                const std::vector<VelocitySplit>& splits)
    {
        juce::String xml;
        xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        xml << "<MPCVObject type=\"com.akaipro.mpc.drum.program\">\n";
        xml << "  <Version>1.7</Version>\n";
        xml << "  <ProgramName>" << xmlEscape(prog.name) << "</ProgramName>\n";
        xml << "  <AfterTouch>\n";
        xml << "    <Destination>ChokeSpeed</Destination>\n";
        xml << "    <Amount>40</Amount>\n";
        xml << "  </AfterTouch>\n";
        xml << "  <Instruments>\n";

        int note = gmNoteForCategory(prog.category);
        xml << "    <Instrument index=\"0\">\n";
        xml << "      <Note>" << note << "</Note>\n";
        xml << "      <TriggerMode>OneShot</TriggerMode>\n";
        xml << "      <PadColor>#E9C46A</PadColor>\n";

        if (prog.category == SampleCategory::HiHatClosed)
            xml << "      <ChokeSend>1</ChokeSend>\n";
        else if (prog.category == SampleCategory::HiHatOpen)
            xml << "      <ChokeReceive>1</ChokeReceive>\n";

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
                xml << "          <KeyTrack>False</KeyTrack>\n";
                if (rrs.size() > 1)
                {
                    xml << "          <CycleType>RoundRobin</CycleType>\n";
                    xml << "          <CycleGroup>" << (v + 1) << "</CycleGroup>\n";
                }
                xml << "        </Layer>\n";
            }
        }
        // Empty layers get VelStart = 0 (critical XPM rule #3)
        for (int v = prog.numVelocityLayers; v < 4; ++v)
        {
            xml << "        <Layer index=\"" << layerIdx++ << "\">\n";
            xml << "          <VelStart>0</VelStart>\n";
            xml << "          <VelEnd>0</VelEnd>\n";
            xml << "        </Layer>\n";
        }
        xml << "      </Layers>\n";
        xml << "    </Instrument>\n";
        xml << "  </Instruments>\n";
        xml << "</MPCVObject>\n";
        return xml;
    }

    // Multi-zone keygroup builder — one <Keygroup> per melodic sample.
    juce::String buildKeygroupXPM (const std::vector<UpgradedProgram*>& progs,
                                    const std::vector<VelocitySplit>& splits,
                                    const juce::String& programName)
    {
        // Sort by detected MIDI note (ascending)
        std::vector<UpgradedProgram*> sorted = progs;
        std::sort(sorted.begin(), sorted.end(),
                  [](const UpgradedProgram* a, const UpgradedProgram* b) {
                      return a->sourceInfo.detectedMidiNote < b->sourceInfo.detectedMidiNote;
                  });

        // Compute zone boundaries: midpoint rule.
        // lowNote[i]  = (rootNote[i-1] + rootNote[i]) / 2 + 1  (exclusive upper of previous zone)
        // highNote[i] = (rootNote[i] + rootNote[i+1]) / 2       (inclusive upper of this zone)
        // Edge cases: first zone low = 0, last zone high = 127.
        int n = (int)sorted.size();
        std::vector<int> lowNotes(n), highNotes(n);

        if (n == 1)
        {
            // Single sample: full range
            lowNotes[0]  = 0;
            highNotes[0] = 127;
        }
        else
        {
            for (int i = 0; i < n; ++i)
            {
                int root = sorted[i]->sourceInfo.detectedMidiNote;

                // Low boundary
                if (i == 0)
                    lowNotes[i] = 0;
                else
                {
                    int prevRoot = sorted[i - 1]->sourceInfo.detectedMidiNote;
                    lowNotes[i] = (prevRoot + root) / 2 + 1;  // round down, then +1 for exclusive upper
                }

                // High boundary
                if (i == n - 1)
                    highNotes[i] = 127;
                else
                {
                    int nextRoot = sorted[i + 1]->sourceInfo.detectedMidiNote;
                    highNotes[i] = (root + nextRoot) / 2;      // inclusive upper of this zone
                }

                // Clamp
                lowNotes[i]  = juce::jlimit(0, 127, lowNotes[i]);
                highNotes[i] = juce::jlimit(0, 127, highNotes[i]);
            }
        }

        // Build XPM XML
        juce::String xml;
        xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        xml << "<MPCVObject type=\"com.akaipro.mpc.keygroup.program\">\n";
        xml << "  <Version>1.7</Version>\n";
        xml << "  <ProgramName>" << xmlEscape(programName) << "</ProgramName>\n";
        xml << "  <AfterTouch>\n";
        xml << "    <Destination>FilterResonance</Destination>\n";
        xml << "    <Amount>40</Amount>\n";
        xml << "  </AfterTouch>\n";
        xml << "  <ModWheel>\n";
        xml << "    <Destination>FilterCutoff</Destination>\n";
        xml << "    <Amount>50</Amount>\n";
        xml << "  </ModWheel>\n";
        xml << "  <PitchBendRange>24</PitchBendRange>\n";
        xml << "  <Keygroups>\n";

        for (int i = 0; i < n; ++i)
        {
            const auto& prog = *sorted[i];
            int root = prog.sourceInfo.detectedMidiNote;

            xml << "    <Keygroup index=\"" << i << "\">\n";
            xml << "      <LowNote>" << lowNotes[i] << "</LowNote>\n";
            xml << "      <HighNote>" << highNotes[i] << "</HighNote>\n";

            xml << "      <Layers>\n";
            int layerIdx = 0;

            for (int v = 0; v < prog.numVelocityLayers && v < (int)splits.size(); ++v)
            {
                auto rrs = getLayersForVel(prog.layers, v);
                for (const auto* l : rrs)
                {
                    xml << "        <Layer index=\"" << layerIdx++ << "\">\n";
                    xml << "          <SampleName>" << l->filename << "</SampleName>\n";
                    xml << "          <VelStart>" << splits[v].start << "</VelStart>\n";
                    xml << "          <VelEnd>" << splits[v].end << "</VelEnd>\n";
                    xml << "          <Volume>" << juce::String(splits[v].volume, 2) << "</Volume>\n";
                    xml << "          <RootNote>" << root << "</RootNote>\n";
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

            // Empty layers must have VelStart = 0 (XPM hardware requirement)
            for (int v = prog.numVelocityLayers; v < 4; ++v)
            {
                xml << "        <Layer index=\"" << layerIdx++ << "\">\n";
                xml << "          <VelStart>0</VelStart>\n";
                xml << "          <VelEnd>0</VelEnd>\n";
                xml << "        </Layer>\n";
            }

            xml << "      </Layers>\n";
            xml << "    </Keygroup>\n";
        }

        xml << "  </Keygroups>\n";
        xml << "</MPCVObject>\n";
        return xml;
    }

    // Single-sample overload — delegates to the multi-zone builder.
    // Preserves backward compatibility with any callers that pass a single UpgradedProgram.
    juce::String buildKeygroupXPM (const UpgradedProgram& prog,
                                    const std::vector<VelocitySplit>& splits)
    {
        std::vector<UpgradedProgram*> single = { const_cast<UpgradedProgram*>(&prog) };
        return buildKeygroupXPM(single, splits, prog.name);
    }

    //==========================================================================
    // Stage 7: PACKAGE
    //==========================================================================

    bool packageXPN (const std::vector<UpgradedProgram>& programs,
                     const juce::File& workDir, const juce::File& output)
    {
        // Build manifest first (small, always in memory)
        juce::String manifest;
        manifest << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        manifest << "<Expansion>\n";
        manifest << "  <Name>" << xmlEscape(settings_.packName) << "</Name>\n";
        manifest << "  <Author>XOutshine by XO_OX Designs</Author>\n";
        manifest << "  <Version>1.0</Version>\n";
        manifest << "  <Description>Upgraded by XOutshine</Description>\n";
        manifest << "  <ProgramCount>" << (int)programs.size() << "</ProgramCount>\n";
        manifest << "</Expansion>\n";

        auto manifestFile = workDir.getChildFile("Manifest.xml");
        manifestFile.replaceWithText(manifest);

        // Enumerate all files to add. Order: Manifest, XPMs, WAVs.
        // juce::ZipFile::Builder reads each file from disk during writeToStream()
        // using a FileInputStream — one file at a time. Peak memory = one compressed chunk.
        juce::ZipFile::Builder builder;
        builder.addFile(manifestFile, 9, "Expansions/Manifest.xml");

        auto xpmDir = workDir.getChildFile("programs");
        int totalFiles = 1;  // manifest

        for (const auto& prog : programs)
        {
            auto xpmFile = xpmDir.getChildFile(prog.name + ".xpm");
            if (xpmFile.existsAsFile())
            {
                builder.addFile(xpmFile, 9, "Programs/" + prog.name + ".xpm");
                ++totalFiles;
            }
            for (const auto& l : prog.layers)
            {
                if (l.file.existsAsFile())
                {
                    // WAV files: use compression level 0 (store) to avoid re-encoding.
                    // WAV is already compressed at 24-bit integer; zlib compression of PCM
                    // yields <1% reduction and wastes CPU. MPC unzips faster at level 0.
                    builder.addFile(l.file, 0, "Samples/" + prog.name + "/" + l.filename);
                    ++totalFiles;
                }
            }
        }

        juce::FileOutputStream fos(output);
        if (!fos.openedOk())
        {
            errors_.add("Package failed: cannot open output for writing: " + output.getFullPathName());
            return false;
        }

        // writeToStream() streams files from disk one at a time.
        // The double* progress parameter (0.0-1.0) is optional; pass nullptr.
        bool ok = (builder.writeToStream(fos, nullptr) > 0);
        if (!ok)
            errors_.add("Package failed: ZIP write error for " + output.getFullPathName());

        return ok;
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

    // XML-escape a string for safe embedding in XML element content
    static juce::String xmlEscape (const juce::String& s)
    {
        return s.replace("&", "&amp;")
                .replace("<", "&lt;")
                .replace(">", "&gt;")
                .replace("\"", "&quot;");
    }

    // Strip FAT32-illegal characters from a filename base (no extension)
    static juce::String sanitizeForFAT32 (const juce::String& name)
    {
        return name.removeCharacters("?<>:\"/\\|*");
    }

    static float measureLufs (const juce::AudioBuffer<float>& buf, double sr)
    {
        int n = buf.getNumSamples();
        int nch = buf.getNumChannels();
        int window = (int)(0.4 * sr);
        if (n < window || nch == 0) return -100.0f;

        // Sum mean-square energy across all channels (ITU-R BS.1770 multi-channel)
        std::vector<float> powers;
        for (int i = 0; i <= n - window; i += window / 4)
        {
            float ms = 0.0f;
            for (int ch = 0; ch < nch; ++ch)
            {
                auto* data = buf.getReadPointer(ch);
                for (int j = 0; j < window; ++j) ms += data[i + j] * data[i + j];
            }
            powers.push_back(ms / (window * nch));
        }
        if (powers.empty()) return -100.0f;

        // Absolute gate: 10^((-70+0.691)/10) — constant, precomputed
        constexpr float absGate = 1.9498446e-07f;  // std::pow(10, -69.309/10)
        std::vector<float> gated;
        for (float p : powers) if (p > absGate) gated.push_back(p);
        if (gated.empty()) return -100.0f;

        float meanUngated = 0.0f;
        for (float p : gated) meanUngated += p;
        meanUngated /= gated.size();

        float relGate = meanUngated * 0.1f;  // 10^(-10/10) = 0.1
        std::vector<float> finalGated;
        for (float p : gated) if (p > relGate) finalGated.push_back(p);
        if (finalGated.empty()) return -100.0f;

        float meanFinal = 0.0f;
        for (float p : finalGated) meanFinal += p;
        meanFinal /= finalGated.size();

        // 10*log10(x) = 10*log2(x)/log2(10) = 10*log2(x)*0.30103 = 3.01030*log2(x)
        return 3.01030f * fastLog2(std::max(meanFinal, 1e-10f)) - 0.691f;
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
                float scale = 1.0f / fastPow2((float)(bitDepth - 1));
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

    // 2:1 decimation with a simple 5-tap LP FIR anti-alias filter.
    // Cutoff is approximately 0.45 * Nyquist of the output rate.
    // Suitable for 96 kHz → 48 kHz. Not suitable for >2:1 ratios.
    static juce::AudioBuffer<float> downsampleBy2 (const juce::AudioBuffer<float>& src)
    {
        int nch  = src.getNumChannels();
        int nIn  = src.getNumSamples();
        int nOut = nIn / 2;
        juce::AudioBuffer<float> out(nch, nOut);

        // 5-tap symmetric LP FIR: h = [0.0625, 0.25, 0.375, 0.25, 0.0625]
        // Designed for cutoff ~0.45 * Nyquist_out. Provides ~50 dB stopband attenuation.
        constexpr float h[5] = { 0.0625f, 0.25f, 0.375f, 0.25f, 0.0625f };

        for (int ch = 0; ch < nch; ++ch)
        {
            auto* src_ch = src.getReadPointer(ch);
            auto* dst_ch = out.getWritePointer(ch);

            for (int i = 0; i < nOut; ++i)
            {
                // Input sample index for output sample i is 2*i.
                // Filter window centered at 2*i, taps at 2*i-2 to 2*i+2.
                float acc = 0.0f;
                for (int k = 0; k < 5; ++k)
                {
                    int srcIdx = 2 * i + (k - 2);
                    if (srcIdx >= 0 && srcIdx < nIn)
                        acc += h[k] * src_ch[srcIdx];
                }
                dst_ch[i] = acc;
            }
        }
        return out;
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

} // namespace xolokun
