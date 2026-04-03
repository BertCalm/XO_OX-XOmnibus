/*
    XOlokun XPN Export Tests
    ==========================
    Tests for XOriginate: XPM rule enforcement, WAV format, filename
    sanitization, note strategies, velocity layers, batch validation,
    bundle structure, cancellation, cover art regression, fade guard,
    group normalization, one-shot mode, and expression mapping.
    Migrated to Catch2 v3: issue #81
*/

#include "XPNExportTests.h"

#include <catch2/catch_test_macros.hpp>

#include "Export/XOriginate.h"
#include "Export/XPNDrumExporter.h"
#include "Export/XPNCoverArt.h"
#include "Core/PresetManager.h"

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <cmath>
#include <set>
#include <string>

using namespace xoceanus;

//==============================================================================
// Helpers
//==============================================================================

static PresetData makeTestPreset(const char* name, const char* engine = "OddfeliX")
{
    PresetData p;
    p.schemaVersion = 1;
    p.name = name;
    p.mood = "Foundation";
    p.engines.add(engine);
    p.dna.brightness = 0.5f;
    p.dna.warmth     = 0.5f;
    p.dna.movement   = 0.5f;
    p.dna.density    = 0.5f;
    p.dna.space      = 0.5f;
    p.dna.aggression = 0.5f;
    return p;
}

static juce::File getTestOutputDir(const char* testName)
{
    auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                   .getChildFile("xolokun_export_tests")
                   .getChildFile(testName);
    dir.deleteRecursively();
    dir.createDirectory();
    return dir;
}

static std::unique_ptr<juce::XmlElement> parseXmlFile(const juce::File& f)
{
    return juce::XmlDocument::parse(f);
}

static juce::String getChildText(const juce::XmlElement* parent, const juce::String& tagName)
{
    if (parent == nullptr) return {};
    auto* child = parent->getChildByName(tagName);
    if (child == nullptr) return {};
    return child->getAllSubText().trim();
}

static int getChildInt(const juce::XmlElement* parent, const juce::String& tagName, int defaultVal = -1)
{
    auto text = getChildText(parent, tagName);
    if (text.isEmpty()) return defaultVal;
    return text.getIntValue();
}

//==============================================================================
// 1. XPM Rule Enforcement Tests
//==============================================================================

TEST_CASE("XPN Export - XPM rule enforcement (KeyTrack/RootNote/VelStart)", "[export][xpm-rules]")
{
    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name = "RuleTest";
    config.bundleId = "com.xo-ox.test.rules";
    config.outputDir = getTestOutputDir("xpm_rules");

    XOriginate::RenderSettings settings;
    settings.noteStrategy = XOriginate::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 2;

    std::vector<PresetData> presets = { makeTestPreset("TestPad") };
    auto result = exporter.exportBundle(config, settings, presets);
    CHECK(result.success);

    auto bundleDir = config.outputDir.getChildFile("RuleTest");
    auto xpmFiles  = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");
    REQUIRE(xpmFiles.size() == 1);

    auto xml = parseXmlFile(xpmFiles[0]);
    REQUIRE(xml != nullptr);

    auto* program = xml->getChildByName("Program");
    REQUIRE(program != nullptr);

    // Rule 1: KeyTrack = True
    CHECK(getChildText(program, "KeyTrack") == "True");

    auto* keygroups = program->getChildByName("Keygroups");
    REQUIRE(keygroups != nullptr);

    bool allRootNote0   = true;
    bool allEmptyVelStart0 = true;
    int  keygroupCount  = 0;

    for (auto* kg = keygroups->getFirstChildElement(); kg; kg = kg->getNextElement())
    {
        if (kg->getTagName() != "Keygroup") continue;
        keygroupCount++;

        // Rule 2: RootNote = 0
        if (getChildInt(kg, "RootNote", -1) != 0)
            allRootNote0 = false;

        // Rule 3: empty layers have VelStart = 0
        auto* layers = kg->getChildByName("Layers");
        if (layers != nullptr)
        {
            int layerIdx = 0;
            for (auto* layer = layers->getFirstChildElement(); layer; layer = layer->getNextElement())
            {
                if (layer->getTagName() != "Layer") continue;
                layerIdx++;
                if (layerIdx > settings.velocityLayers)
                {
                    if (getChildInt(layer, "VelStart", -1) != 0)
                        allEmptyVelStart0 = false;
                }
            }
        }
    }

    CHECK(allRootNote0);
    CHECK(allEmptyVelStart0);
    CHECK(keygroupCount > 0);
}

//==============================================================================
// 2. WAV Format Validation
//==============================================================================

TEST_CASE("XPN Export - WAV format (48 kHz / 24-bit / stereo / correct length)", "[export][wav]")
{
    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name = "WAVTest";
    config.bundleId = "com.xo-ox.test.wav";
    config.outputDir = getTestOutputDir("wav_format");

    XOriginate::RenderSettings settings;
    settings.sampleRate    = 48000.0;
    settings.bitDepth      = 24;
    settings.renderSeconds = 1.0f;
    settings.tailSeconds   = 0.5f;
    settings.noteStrategy  = XOriginate::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 1;

    std::vector<PresetData> presets = { makeTestPreset("WAVCheck") };
    auto result = exporter.exportBundle(config, settings, presets);
    CHECK(result.success);
    CHECK(result.samplesRendered > 0);

    auto bundleDir = config.outputDir.getChildFile("WAVTest");
    auto wavFiles  = bundleDir.findChildFiles(juce::File::findFiles, true, "*.WAV");
    REQUIRE(wavFiles.size() > 0);

    juce::WavAudioFormat wav;
    auto stream = wavFiles[0].createInputStream();
    REQUIRE(stream != nullptr);

    auto reader = std::unique_ptr<juce::AudioFormatReader>(
        wav.createReaderFor(stream.release(), true));
    REQUIRE(reader != nullptr);

    CHECK(std::abs(reader->sampleRate - 48000.0) < 1.0);
    CHECK(reader->bitsPerSample == 24);
    CHECK(reader->numChannels == 2);
    CHECK(reader->lengthInSamples > 0);

    int64_t expectedSamples = (int64_t)(1.5 * 48000.0);
    CHECK(reader->lengthInSamples == expectedSamples);
}

//==============================================================================
// 3. Filename Sanitization
//==============================================================================

TEST_CASE("XPN Export - filename sanitization strips illegal characters", "[export][sanitize]")
{
    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name = "SanitizeTest";
    config.bundleId = "com.xo-ox.test.sanitize";
    config.outputDir = getTestOutputDir("sanitize");

    XOriginate::RenderSettings settings;
    settings.noteStrategy   = XOriginate::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 1;
    settings.renderSeconds  = 0.1f;
    settings.tailSeconds    = 0.1f;

    auto preset = makeTestPreset("Test/Preset:With*Special<Chars>");
    std::vector<PresetData> presets = { preset };
    auto result = exporter.exportBundle(config, settings, presets);
    CHECK(result.success);

    auto bundleDir = config.outputDir.getChildFile("SanitizeTest");
    auto allFiles  = bundleDir.findChildFiles(juce::File::findFiles, true);
    bool allClean  = true;
    for (const auto& f : allFiles)
    {
        if (f.getFileName().containsAnyOf("/\\:*?\"<>|"))
            allClean = false;
    }
    CHECK(allClean);
}

TEST_CASE("XPN Export - long preset name is accepted (truncated silently)", "[export][sanitize]")
{
    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name = "SanitizeTest";
    config.bundleId = "com.xo-ox.test.sanitize";
    config.outputDir = getTestOutputDir("sanitize_long");

    XOriginate::RenderSettings settings;
    settings.noteStrategy   = XOriginate::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 1;
    settings.renderSeconds  = 0.1f;
    settings.tailSeconds    = 0.1f;

    juce::String longName;
    for (int i = 0; i < 100; ++i) longName += "A";
    auto longPreset  = makeTestPreset("placeholder");
    longPreset.name  = longName;
    std::vector<PresetData> longPresets = { longPreset };

    auto longResult = exporter.exportBundle(config, settings, longPresets);
    CHECK(longResult.success);
}

//==============================================================================
// 4. Note Strategy Tests
//==============================================================================

TEST_CASE("XPN Export - note strategies produce expected sample counts", "[export][note-strategy]")
{
    XOriginate exporter;
    XOriginate::RenderSettings settings;
    settings.velocityLayers = 1;
    settings.renderSeconds  = 0.1f;
    settings.tailSeconds    = 0.1f;

    struct StrategyTest {
        XOriginate::RenderSettings::NoteStrategy strategy;
        const char* name;
        int expectedMin;
        int expectedMax;
    };

    StrategyTest tests[] = {
        { XOriginate::RenderSettings::NoteStrategy::OctavesOnly,   "OctavesOnly",   6, 7  },
        { XOriginate::RenderSettings::NoteStrategy::EveryFifth,    "EveryFifth",    10, 12 },
        { XOriginate::RenderSettings::NoteStrategy::EveryMinor3rd, "EveryMinor3rd", 24, 26 },
        { XOriginate::RenderSettings::NoteStrategy::Chromatic,     "Chromatic",     72, 74 },
    };

    for (auto& t : tests)
    {
        INFO("Strategy: " << t.name);
        settings.noteStrategy = t.strategy;

        XOriginate::BundleConfig config;
        config.name      = juce::String("Strategy_") + t.name;
        config.bundleId  = juce::String("com.xo-ox.test.") + t.name;
        config.outputDir = getTestOutputDir(t.name);

        std::vector<PresetData> presets = { makeTestPreset("NoteTest") };
        auto result = exporter.exportBundle(config, settings, presets);

        bool inRange = result.samplesRendered >= t.expectedMin
                    && result.samplesRendered <= t.expectedMax;
        CHECK(inRange);
    }
}

//==============================================================================
// 5. Velocity Layer Range Tests
//==============================================================================

TEST_CASE("XPN Export - velocity layers cover 0-127 with no gaps", "[export][velocity-layers]")
{
    XOriginate exporter;

    for (int layers = 1; layers <= 4; ++layers)
    {
        INFO("Velocity layers: " << layers);

        XOriginate::BundleConfig config;
        config.name      = juce::String("VelTest_") + juce::String(layers);
        config.bundleId  = "com.xo-ox.test.vel";
        config.outputDir = getTestOutputDir(("vel_" + std::to_string(layers)).c_str());

        XOriginate::RenderSettings settings;
        settings.noteStrategy   = XOriginate::RenderSettings::NoteStrategy::OctavesOnly;
        settings.velocityLayers = layers;
        settings.renderSeconds  = 0.1f;
        settings.tailSeconds    = 0.1f;

        std::vector<PresetData> presets = { makeTestPreset("VelCheck") };
        exporter.exportBundle(config, settings, presets);

        auto bundleDir = config.outputDir.getChildFile(config.name.replace(" ", "_"));
        auto xpmFiles  = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");
        REQUIRE(xpmFiles.size() == 1);

        auto xml = parseXmlFile(xpmFiles[0]);
        REQUIRE(xml != nullptr);

        auto* program   = xml->getChildByName("Program");
        auto* keygroups = program ? program->getChildByName("Keygroups") : nullptr;
        REQUIRE(keygroups != nullptr);

        const juce::XmlElement* firstKg = nullptr;
        for (auto* kg = keygroups->getFirstChildElement(); kg; kg = kg->getNextElement())
        {
            if (kg->getTagName() == "Keygroup") { firstKg = kg; break; }
        }
        REQUIRE(firstKg != nullptr);

        auto* layersElem = firstKg->getChildByName("Layers");
        REQUIRE(layersElem != nullptr);

        bool noGaps       = true;
        int  prevEnd      = -1;
        int  minVelStart  = 128;
        int  maxVelEnd    = -1;
        int  activeLayerCount = 0;

        for (auto* layer = layersElem->getFirstChildElement(); layer; layer = layer->getNextElement())
        {
            if (layer->getTagName() != "Layer") continue;
            int velStart = getChildInt(layer, "VelStart");
            int velEnd   = getChildInt(layer, "VelEnd");

            if (velEnd == 0 && activeLayerCount >= layers) continue;
            activeLayerCount++;
            if (velStart < minVelStart) minVelStart = velStart;
            if (velEnd   > maxVelEnd)   maxVelEnd   = velEnd;
            if (prevEnd >= 0 && velStart != prevEnd + 1) noGaps = false;
            prevEnd = velEnd;
        }

        CHECK(noGaps);
        CHECK(minVelStart == 0);
        CHECK(maxVelEnd   == 127);
    }
}

//==============================================================================
// 6. Batch Validation Tests
//==============================================================================

TEST_CASE("XPN Export - batch validation rejects duplicate names", "[export][batch]")
{
    std::vector<PresetData> dupes = { makeTestPreset("SameName"), makeTestPreset("SameName") };
    auto result = XOriginate::validateBatch(dupes);
    CHECK(!result.valid);
    bool hasDupeError = false;
    for (const auto& e : result.errors)
        if (e.contains("Duplicate")) hasDupeError = true;
    CHECK(hasDupeError);
}

TEST_CASE("XPN Export - batch validation rejects empty name", "[export][batch]")
{
    std::vector<PresetData> batch = { makeTestPreset("") };
    CHECK(!XOriginate::validateBatch(batch).valid);
}

TEST_CASE("XPN Export - batch validation rejects out-of-range DNA", "[export][batch]")
{
    auto p = makeTestPreset("BadDNA");
    p.dna.brightness = 1.5f;
    std::vector<PresetData> batch = { p };
    CHECK(!XOriginate::validateBatch(batch).valid);
}

TEST_CASE("XPN Export - batch validation warns on long name but stays valid", "[export][batch]")
{
    auto p = makeTestPreset("ThisNameIsDefinitelyLongerThan30Characters");
    std::vector<PresetData> batch = { p };
    auto result = XOriginate::validateBatch(batch);
    CHECK(result.valid);
    CHECK(result.warnings.size() > 0);
}

TEST_CASE("XPN Export - valid batch passes with no errors", "[export][batch]")
{
    std::vector<PresetData> good = {
        makeTestPreset("Alpha"),
        makeTestPreset("Beta"),
        makeTestPreset("Gamma"),
    };
    auto result = XOriginate::validateBatch(good);
    CHECK(result.valid);
    CHECK(result.errors.isEmpty());
}

//==============================================================================
// 7. Bundle Structure Tests
//==============================================================================

TEST_CASE("XPN Export - bundle structure contains required files and manifest", "[export][structure]")
{
    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name         = "StructureTest";
    config.bundleId     = "com.xo-ox.test.structure";
    config.manufacturer = "XO_OX Designs";
    config.version      = "1.2.3";
    config.description  = "Structure test bundle";
    config.coverEngine  = "ONSET";
    config.outputDir    = getTestOutputDir("structure");

    XOriginate::RenderSettings settings;
    settings.noteStrategy   = XOriginate::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 1;
    settings.renderSeconds  = 0.1f;
    settings.tailSeconds    = 0.1f;

    std::vector<PresetData> presets = { makeTestPreset("Pad Alpha"), makeTestPreset("Lead Beta") };
    auto result = exporter.exportBundle(config, settings, presets);
    CHECK(result.success);
    CHECK(result.presetsExported == 2);

    auto bundleDir = config.outputDir.getChildFile("StructureTest");
    CHECK(bundleDir.getChildFile("Manifest.xml").existsAsFile());
    CHECK(bundleDir.getChildFile("Preview.png").existsAsFile());
    CHECK(bundleDir.getChildFile("Keygroups").isDirectory());
    CHECK(bundleDir.getChildFile("artwork.png").existsAsFile());
    CHECK(bundleDir.getChildFile("artwork_2000.png").existsAsFile());
    CHECK(result.totalSizeBytes > 0);

    auto manifest = parseXmlFile(bundleDir.getChildFile("Manifest.xml"));
    REQUIRE(manifest != nullptr);
    CHECK(manifest->getStringAttribute("Name")          == "StructureTest");
    CHECK(manifest->getStringAttribute("Manufacturer")  == "XO_OX Designs");
    CHECK(manifest->getStringAttribute("Version")       == "1.2.3");
    CHECK(manifest->getIntAttribute("PresetCount")      == 2);

    auto xpmFiles = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");
    CHECK(xpmFiles.size() == 2);
}

//==============================================================================
// 8. Cancel Mid-Export
//==============================================================================

TEST_CASE("XPN Export - cancellation stops export before all presets are processed",
          "[export][cancel]")
{
    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name      = "CancelTest";
    config.bundleId  = "com.xo-ox.test.cancel";
    config.outputDir = getTestOutputDir("cancel");

    XOriginate::RenderSettings settings;
    settings.noteStrategy   = XOriginate::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 1;
    settings.renderSeconds  = 0.1f;
    settings.tailSeconds    = 0.1f;

    std::vector<PresetData> presets;
    for (int i = 0; i < 10; ++i)
        presets.push_back(makeTestPreset(("Preset_" + std::to_string(i)).c_str()));

    int cancelAfter = 2;
    auto result = exporter.exportBundle(config, settings, presets,
        [&](XOriginate::Progress& p)
        {
            if (p.currentPreset >= cancelAfter)
                p.cancelled = true;
        });

    CHECK(!result.success);
    CHECK(result.presetsExported < (int)presets.size());
}

//==============================================================================
// 9. Empty Preset List
//==============================================================================

TEST_CASE("XPN Export - empty preset list returns failure with error message", "[export][empty]")
{
    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name      = "EmptyTest";
    config.bundleId  = "com.xo-ox.test.empty";
    config.outputDir = getTestOutputDir("empty");

    std::vector<PresetData> empty;
    auto result = exporter.exportBundle(config, XOriginate::RenderSettings{}, empty);
    CHECK(!result.success);
    CHECK(result.errorMessage.isNotEmpty());
    CHECK(result.presetsExported == 0);
}

//==============================================================================
// 10. Cover Art Regression
//==============================================================================

TEST_CASE("XPN Export - cover art engine labels resolve correctly", "[export][cover-art]")
{
    struct EngineCheck { const char* id; const char* expectedLabel; };
    EngineCheck engines[] = {
        { "ONSET",     "ONSET"     },
        { "OVERWORLD", "OVERWORLD" },
        { "ODDFELIX",  "OddfeliX"  },
        { "ODDOSCAR",  "OddOscar"  },
        { "OVERDUB",   "OVERDUB"   },
        { "ODYSSEY",   "ODYSSEY"   },
        { "OBLONG",    "OBLONG"    },
        { "OBESE",     "OBESE"     },
        { "OPAL",      "OPAL"      },
        { "ORGANON",   "ORGANON"   },
        { "OUROBOROS", "OUROBOROS" },
        { "OBSIDIAN",  "OBSIDIAN"  },
        { "OVERBITE",  "OVERBITE"  },
        { "ORIGAMI",   "ORIGAMI"   },
        { "ORACLE",    "ORACLE"    },
        { "OBSCURA",   "OBSCURA"   },
        { "OCEANIC",   "OCEANIC"   },
        { "OPTIC",     "OPTIC"     },
        { "OBLIQUE",   "OBLIQUE"   },
        { "ORBITAL",   "ORBITAL"   },
    };
    for (auto& e : engines)
    {
        INFO("Engine ID: " << e.id);
        auto def = XPNCoverArt::getEngineDef(e.id);
        CHECK(juce::String(def.label) == juce::String(e.expectedLabel));
    }
}

TEST_CASE("XPN Export - cover art legacy aliases resolve correctly", "[export][cover-art]")
{
    struct AliasCheck { const char* legacy; const char* expectedLabel; };
    AliasCheck aliases[] = {
        { "SNAP",  "OddfeliX" },
        { "MORPH", "OddOscar" },
        { "DUB",   "OVERDUB"  },
        { "DRIFT", "ODYSSEY"  },
        { "BOB",   "OBLONG"   },
        { "FAT",   "OBESE"    },
    };
    for (auto& a : aliases)
    {
        INFO("Legacy: " << a.legacy);
        auto def = XPNCoverArt::getEngineDef(a.legacy);
        CHECK(juce::String(def.label) == juce::String(a.expectedLabel));
    }
}

TEST_CASE("XPN Export - cover art generation produces valid PNG files", "[export][cover-art]")
{
    auto outputDir = getTestOutputDir("cover_art");
    auto result = XPNCoverArt::generate("ONSET", "Test Pack", outputDir, 10, "1.0", 42);
    CHECK(result.success);
    CHECK(result.cover1000.existsAsFile());
    CHECK(result.cover2000.existsAsFile());
    CHECK(result.cover1000.getSize() > 1024);
    CHECK(result.cover2000.getSize() > 1024);
    CHECK(result.cover2000.getSize() > result.cover1000.getSize());
}

TEST_CASE("XPN Export - unknown engine falls back to default cover art label", "[export][cover-art]")
{
    auto def = XPNCoverArt::getEngineDef("NONEXISTENT_ENGINE");
    CHECK(juce::String(def.label) == "XO_OX");
}

//==============================================================================
// 11. XPM Keygroup Coverage
//==============================================================================

TEST_CASE("XPN Export - keygroups cover all 128 MIDI notes without overlap", "[export][keygroup-coverage]")
{
    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name      = "KeygroupCoverage";
    config.bundleId  = "com.xo-ox.test.keygroups";
    config.outputDir = getTestOutputDir("keygroups");

    XOriginate::RenderSettings settings;
    settings.noteStrategy   = XOriginate::RenderSettings::NoteStrategy::EveryMinor3rd;
    settings.velocityLayers = 3;
    settings.renderSeconds  = 0.1f;
    settings.tailSeconds    = 0.1f;

    std::vector<PresetData> presets = { makeTestPreset("KeygroupCheck") };
    exporter.exportBundle(config, settings, presets);

    auto bundleDir = config.outputDir.getChildFile("KeygroupCoverage");
    auto xpmFiles  = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");
    REQUIRE(xpmFiles.size() == 1);

    auto xml = parseXmlFile(xpmFiles[0]);
    REQUIRE(xml != nullptr);

    auto* program   = xml->getChildByName("Program");
    auto* keygroups = program ? program->getChildByName("Keygroups") : nullptr;
    REQUIRE(keygroups != nullptr);

    std::set<int> coveredKeys;
    bool noOverlap  = true;
    int  prevHighKey = -1;

    for (auto* kg = keygroups->getFirstChildElement(); kg; kg = kg->getNextElement())
    {
        if (kg->getTagName() != "Keygroup") continue;
        int lowKey  = getChildInt(kg, "LowNote");
        int highKey = getChildInt(kg, "HighNote");
        if (prevHighKey >= 0 && lowKey <= prevHighKey) noOverlap = false;
        for (int k = lowKey; k <= highKey; ++k) coveredKeys.insert(k);
        prevHighKey = highKey;
    }

    CHECK(coveredKeys.count(0)   > 0);
    CHECK(coveredKeys.count(127) > 0);
    CHECK(noOverlap);

    bool contiguous = true;
    for (int k = 0; k <= 127; ++k)
        if (coveredKeys.count(k) == 0) { contiguous = false; break; }
    CHECK(contiguous);
}

//==============================================================================
// 12. Fade Guard Verification
//==============================================================================

TEST_CASE("XPN Export - rendered WAV starts and ends near zero (fade guard)", "[export][fade-guard]")
{
    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name      = "FadeGuard";
    config.bundleId  = "com.xo-ox.test.fadeguard";
    config.outputDir = getTestOutputDir("fadeguard");

    XOriginate::RenderSettings settings;
    settings.noteStrategy   = XOriginate::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 1;
    settings.renderSeconds  = 0.5f;
    settings.tailSeconds    = 1.0f;

    std::vector<PresetData> presets = { makeTestPreset("FadeCheck") };
    auto result = exporter.exportBundle(config, settings, presets);
    REQUIRE(result.success);

    auto bundleDir = config.outputDir.getChildFile("FadeGuard");
    auto wavFiles  = bundleDir.findChildFiles(juce::File::findFiles, true, "*.WAV");
    REQUIRE(wavFiles.size() > 0);

    juce::WavAudioFormat wav;
    auto stream = wavFiles[0].createInputStream();
    REQUIRE(stream != nullptr);
    auto reader = std::unique_ptr<juce::AudioFormatReader>(
        wav.createReaderFor(stream.release(), true));
    REQUIRE(reader != nullptr);
    REQUIRE(reader->lengthInSamples > 0);

    constexpr int   guardSamples = 64;
    constexpr float threshold    = 0.1f;

    // Start check
    juce::AudioBuffer<float> startBuf((int)reader->numChannels, guardSamples);
    reader->read(&startBuf, 0, guardSamples, 0, true, true);
    float startPeak = 0.0f;
    for (int ch = 0; ch < startBuf.getNumChannels(); ++ch)
    {
        auto range = startBuf.findMinMax(ch, 0, guardSamples);
        float chPeak = juce::jmax(std::abs(range.getStart()), std::abs(range.getEnd()));
        if (chPeak > startPeak) startPeak = chPeak;
    }
    CHECK(startPeak < threshold);

    // End check
    int64_t tailStart = reader->lengthInSamples - guardSamples;
    if (tailStart > 0)
    {
        juce::AudioBuffer<float> endBuf((int)reader->numChannels, guardSamples);
        reader->read(&endBuf, 0, guardSamples, tailStart, true, true);
        float endPeak = 0.0f;
        for (int ch = 0; ch < endBuf.getNumChannels(); ++ch)
        {
            auto range = endBuf.findMinMax(ch, 0, guardSamples);
            float chPeak = juce::jmax(std::abs(range.getStart()), std::abs(range.getEnd()));
            if (chPeak > endPeak) endPeak = chPeak;
        }
        CHECK(endPeak < threshold);
    }
}

//==============================================================================
// 13. Group Normalization
//==============================================================================

TEST_CASE("XPN Export - group normalization: hard layer louder than ghost", "[export][normalization]")
{
    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name      = "NormTest";
    config.bundleId  = "com.xo-ox.test.norm";
    config.outputDir = getTestOutputDir("norm");

    XOriginate::RenderSettings settings;
    settings.noteStrategy   = XOriginate::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 4;
    settings.renderSeconds  = 0.5f;
    settings.tailSeconds    = 0.5f;

    std::vector<PresetData> presets = { makeTestPreset("NormCheck") };
    auto result = exporter.exportBundle(config, settings, presets);
    REQUIRE(result.success);

    auto bundleDir = config.outputDir.getChildFile("NormTest");
    auto allWavs   = bundleDir.findChildFiles(juce::File::findFiles, true, "*.WAV");

    juce::File v1File, v4File;
    for (const auto& f : allWavs)
    {
        auto name = f.getFileName();
        if (name.contains("__v1.WAV") && v1File == juce::File{}) v1File = f;
        if (name.contains("__v4.WAV") && v4File == juce::File{}) v4File = f;
    }

    REQUIRE(v1File.existsAsFile());
    REQUIRE(v4File.existsAsFile());

    juce::WavAudioFormat wav;
    auto readPeak = [&](const juce::File& f) -> float
    {
        auto s = f.createInputStream();
        if (!s) return 0.0f;
        auto r = std::unique_ptr<juce::AudioFormatReader>(
            wav.createReaderFor(s.release(), true));
        if (!r || r->lengthInSamples == 0) return 0.0f;
        juce::AudioBuffer<float> buf((int)r->numChannels, (int)r->lengthInSamples);
        r->read(&buf, 0, (int)r->lengthInSamples, 0, true, true);
        float peak = 0.0f;
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            auto range = buf.findMinMax(ch, 0, buf.getNumSamples());
            float chPeak = juce::jmax(std::abs(range.getStart()), std::abs(range.getEnd()));
            if (chPeak > peak) peak = chPeak;
        }
        return peak;
    };

    float peakV1 = readPeak(v1File);
    float peakV4 = readPeak(v4File);

    CHECK(peakV4 > peakV1);
    CHECK(peakV4 > 0.8f);
    CHECK(peakV1 < peakV4);
}

//==============================================================================
// 14. One-Shot Mode in Drum XPM
//==============================================================================

TEST_CASE("XPN Export - drum XPM: all pads have TriggerMode=OneShot", "[export][drum][one-shot]")
{
    XPNDrumExporter drumExporter;
    XPNDrumExporter::DrumExportConfig config;
    config.name      = "OneShotTest";
    config.bundleId  = "com.xo-ox.test.oneshot";
    config.outputDir = getTestOutputDir("oneshot");

    std::vector<PresetData> presets = { makeTestPreset("DrumOneShot", "Onset") };
    auto result = drumExporter.exportDrumBundle(config, presets);
    REQUIRE(result.success);

    auto bundleDir = config.outputDir.getChildFile("OneShotTest");
    auto xpmFiles  = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");
    REQUIRE(xpmFiles.size() == 1);

    auto xml = parseXmlFile(xpmFiles[0]);
    REQUIRE(xml != nullptr);

    auto* program     = xml->getChildByName("Program");
    auto* instruments = program ? program->getChildByName("Instruments") : nullptr;
    REQUIRE(instruments != nullptr);

    bool allOneShot      = true;
    int  instrumentCount = 0;

    for (auto* inst = instruments->getFirstChildElement(); inst; inst = inst->getNextElement())
    {
        if (inst->getTagName() != "Instrument") continue;
        instrumentCount++;
        if (getChildText(inst, "TriggerMode") != "OneShot")
            allOneShot = false;
    }

    CHECK(instrumentCount == XPNDrumExporter::kNumPads);
    CHECK(allOneShot);
}

//==============================================================================
// 15. Expression Mapping in Keygroup XPM
//==============================================================================

TEST_CASE("XPN Export - keygroup XPM has AfterTouch / ModWheel / PitchBendRange",
          "[export][expression][keygroup]")
{
    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name      = "ExprTest";
    config.bundleId  = "com.xo-ox.test.expression";
    config.outputDir = getTestOutputDir("expression");

    XOriginate::RenderSettings settings;
    settings.noteStrategy   = XOriginate::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 1;
    settings.renderSeconds  = 0.1f;
    settings.tailSeconds    = 0.1f;

    std::vector<PresetData> presets = { makeTestPreset("ExprCheck") };
    auto result = exporter.exportBundle(config, settings, presets);
    REQUIRE(result.success);

    auto bundleDir = config.outputDir.getChildFile("ExprTest");
    auto xpmFiles  = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");
    REQUIRE(xpmFiles.size() == 1);

    auto xml = parseXmlFile(xpmFiles[0]);
    REQUIRE(xml != nullptr);
    auto* program = xml->getChildByName("Program");
    REQUIRE(program != nullptr);

    auto* afterTouch = program->getChildByName("AfterTouch");
    REQUIRE(afterTouch != nullptr);
    CHECK(getChildText(afterTouch, "Destination") == "FilterCutoff");
    CHECK(getChildText(afterTouch, "Amount")      == "50");

    auto* modWheel = program->getChildByName("ModWheel");
    REQUIRE(modWheel != nullptr);
    CHECK(getChildText(modWheel, "Destination") == "FilterCutoff");
    CHECK(getChildText(modWheel, "Amount")      == "70");

    CHECK(getChildText(program, "PitchBendRange") == "12");
}

//==============================================================================
// 16. Expression Mapping in Drum XPM
//==============================================================================

TEST_CASE("XPN Export - drum XPM has AfterTouch element", "[export][expression][drum]")
{
    XPNDrumExporter drumExporter;
    XPNDrumExporter::DrumExportConfig config;
    config.name      = "DrumExprTest";
    config.bundleId  = "com.xo-ox.test.drumexpr";
    config.outputDir = getTestOutputDir("drumexpr");

    std::vector<PresetData> presets = { makeTestPreset("DrumExprCheck", "Onset") };
    auto result = drumExporter.exportDrumBundle(config, presets);
    REQUIRE(result.success);

    auto bundleDir = config.outputDir.getChildFile("DrumExprTest");
    auto xpmFiles  = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");
    REQUIRE(xpmFiles.size() == 1);

    auto xml = parseXmlFile(xpmFiles[0]);
    REQUIRE(xml != nullptr);
    auto* program = xml->getChildByName("Program");
    REQUIRE(program != nullptr);

    auto* afterTouch = program->getChildByName("AfterTouch");
    REQUIRE(afterTouch != nullptr);
    CHECK(getChildText(afterTouch, "Destination") == "FilterCutoff");
    CHECK(getChildText(afterTouch, "Amount")      == "30");
}

//==============================================================================
// 17. Pad Color in Drum XPM
//==============================================================================

TEST_CASE("XPN Export - drum XPM: all pads have valid 6-digit hex PadColor",
          "[export][drum][pad-color]")
{
    XPNDrumExporter drumExporter;
    XPNDrumExporter::DrumExportConfig config;
    config.name      = "PadColorTest";
    config.bundleId  = "com.xo-ox.test.padcolor";
    config.outputDir = getTestOutputDir("padcolor");

    std::vector<PresetData> presets = { makeTestPreset("PadColorCheck", "Onset") };
    auto result = drumExporter.exportDrumBundle(config, presets);
    REQUIRE(result.success);

    auto bundleDir = config.outputDir.getChildFile("PadColorTest");
    auto xpmFiles  = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");
    REQUIRE(xpmFiles.size() == 1);

    auto xml = parseXmlFile(xpmFiles[0]);
    REQUIRE(xml != nullptr);
    auto* program     = xml->getChildByName("Program");
    auto* instruments = program ? program->getChildByName("Instruments") : nullptr;
    REQUIRE(instruments != nullptr);

    bool allHaveColor = true;
    bool allValidHex  = true;
    int  count        = 0;

    for (auto* inst = instruments->getFirstChildElement(); inst; inst = inst->getNextElement())
    {
        if (inst->getTagName() != "Instrument") continue;
        count++;
        auto color = getChildText(inst, "PadColor");
        if (color.isEmpty())
            allHaveColor = false;
        else if (color.length() != 6 || !color.containsOnly("0123456789ABCDEFabcdef"))
            allValidHex = false;
    }

    CHECK(count > 0);
    CHECK(allHaveColor);
    CHECK(allValidHex);
}

//==============================================================================
// 18. Mute Group Configuration
//==============================================================================

TEST_CASE("XPN Export - drum XPM has hi-hat, kick, and snare mute groups",
          "[export][drum][mute-groups]")
{
    XPNDrumExporter drumExporter;
    XPNDrumExporter::DrumExportConfig config;
    config.name      = "MuteGroupTest";
    config.bundleId  = "com.xo-ox.test.mutegroups";
    config.outputDir = getTestOutputDir("mutegroups");

    std::vector<PresetData> presets = { makeTestPreset("MuteCheck", "Onset") };
    auto result = drumExporter.exportDrumBundle(config, presets);
    REQUIRE(result.success);

    auto bundleDir = config.outputDir.getChildFile("MuteGroupTest");
    auto xpmFiles  = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");
    REQUIRE(xpmFiles.size() == 1);

    auto xml = parseXmlFile(xpmFiles[0]);
    REQUIRE(xml != nullptr);
    auto* program     = xml->getChildByName("Program");
    auto* instruments = program ? program->getChildByName("Instruments") : nullptr;
    REQUIRE(instruments != nullptr);

    std::set<int> muteGroupsSeen;
    bool hiHatGroupFound = false;
    bool kickGroupFound  = false;
    bool snareGroupFound = false;

    for (auto* inst = instruments->getFirstChildElement(); inst; inst = inst->getNextElement())
    {
        if (inst->getTagName() != "Instrument") continue;
        int    mg   = getChildInt(inst, "MuteGroup", 0);
        auto   name = getChildText(inst, "InstrumentName");
        muteGroupsSeen.insert(mg);

        if (mg == XPNDrumExporter::kChokeHiHat && name.containsIgnoreCase("HAT"))
            hiHatGroupFound = true;
        if (mg == XPNDrumExporter::kChokeKick  && name.containsIgnoreCase("KICK"))
            kickGroupFound  = true;
        if (mg == XPNDrumExporter::kChokeSnare &&
            (name.containsIgnoreCase("SNARE") || name.containsIgnoreCase("CLAP")))
            snareGroupFound = true;
    }

    CHECK(hiHatGroupFound);
    CHECK(kickGroupFound);
    CHECK(snareGroupFound);
    CHECK(muteGroupsSeen.size() >= 3);
}

// Backward-compat shim
namespace export_tests {
int runAll() { return 0; }
} // namespace export_tests
