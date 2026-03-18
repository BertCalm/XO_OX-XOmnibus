/*
    XOmnibus XPN Export Tests
    ==========================
    Tests for XOriginate: XPM rule enforcement, WAV format, filename
    sanitization, note strategies, velocity layers, batch validation,
    bundle structure, cancellation, cover art regression, fade guard,
    group normalization, one-shot mode, and expression mapping.
*/

#include "XPNExportTests.h"

#include "Export/XOriginate.h"
#include "Export/XPNDrumExporter.h"
#include "Export/XPNCoverArt.h"
#include "Core/PresetManager.h"

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>
#include <string>
#include <cmath>
#include <set>

using namespace xomnibus;

//==============================================================================
// Test infrastructure
//==============================================================================

static int g_exportTestsPassed = 0;
static int g_exportTestsFailed = 0;

static void reportTest(const char* name, bool passed)
{
    if (passed)
    {
        std::cout << "  [PASS] " << name << "\n";
        g_exportTestsPassed++;
    }
    else
    {
        std::cout << "  [FAIL] " << name << "\n";
        g_exportTestsFailed++;
    }
}

//==============================================================================
// Helper: create a minimal valid PresetData
//==============================================================================

static PresetData makeTestPreset(const char* name, const char* engine = "OddfeliX")
{
    PresetData p;
    p.schemaVersion = 1;
    p.name = name;
    p.mood = "Foundation";
    p.engines.add(engine);
    p.dna.brightness = 0.5f;
    p.dna.warmth = 0.5f;
    p.dna.movement = 0.5f;
    p.dna.density = 0.5f;
    p.dna.space = 0.5f;
    p.dna.aggression = 0.5f;
    return p;
}

//==============================================================================
// Helper: get a clean temp directory for an export run
//==============================================================================

static juce::File getTestOutputDir(const char* testName)
{
    auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                   .getChildFile("xomnibus_export_tests")
                   .getChildFile(testName);
    dir.deleteRecursively();
    dir.createDirectory();
    return dir;
}

//==============================================================================
// Helper: parse XML from file
//==============================================================================

static std::unique_ptr<juce::XmlElement> parseXmlFile(const juce::File& f)
{
    return juce::XmlDocument::parse(f);
}

//==============================================================================
// Helper: get text content of a child element by tag name
//
// The actual XPM format uses child text elements like:
//   <RootNote>0</RootNote>
// NOT attributes like: <Keygroup RootNote="0">
//==============================================================================

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

static void testXPMRuleEnforcement()
{
    std::cout << "\n--- XPM Rule Enforcement ---\n";

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
    reportTest("XPM rules: export succeeds", result.success);

    // Find the XPM file
    auto bundleDir = config.outputDir.getChildFile("RuleTest");
    auto xpmFiles = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");
    reportTest("XPM rules: XPM file created", xpmFiles.size() == 1);

    if (xpmFiles.size() == 1)
    {
        auto xml = parseXmlFile(xpmFiles[0]);
        reportTest("XPM rules: valid XML", xml != nullptr);

        if (xml != nullptr)
        {
            // Navigate to Program element (MPCVObject > Program)
            auto* program = xml->getChildByName("Program");
            reportTest("XPM rules: Program element exists", program != nullptr);

            if (program != nullptr)
            {
                // Rule 1: KeyTrack = True (child text element)
                reportTest("XPM Rule 1: KeyTrack = True",
                           getChildText(program, "KeyTrack") == "True");

                // Navigate to Keygroups
                auto* keygroups = program->getChildByName("Keygroups");
                reportTest("XPM rules: Keygroups element exists", keygroups != nullptr);

                if (keygroups != nullptr)
                {
                    bool allRootNote0 = true;
                    bool allEmptyVelStart0 = true;
                    int keygroupCount = 0;

                    for (auto* kg = keygroups->getFirstChildElement(); kg; kg = kg->getNextElement())
                    {
                        if (kg->getTagName() != "Keygroup") continue;
                        keygroupCount++;

                        // Rule 2: RootNote = 0 (child text element)
                        if (getChildInt(kg, "RootNote", -1) != 0)
                            allRootNote0 = false;

                        // Rule 3: Empty layers have VelStart = 0
                        auto* layers = kg->getChildByName("Layers");
                        if (layers != nullptr)
                        {
                            int layerIdx = 0;
                            for (auto* layer = layers->getFirstChildElement();
                                 layer; layer = layer->getNextElement())
                            {
                                if (layer->getTagName() != "Layer") continue;
                                layerIdx++;
                                if (layerIdx > settings.velocityLayers)
                                {
                                    // This is an empty/padding layer — VelStart must be 0
                                    if (getChildInt(layer, "VelStart", -1) != 0)
                                        allEmptyVelStart0 = false;
                                }
                            }
                        }
                    }

                    reportTest("XPM Rule 2: all keygroups have RootNote = 0", allRootNote0);
                    reportTest("XPM Rule 3: empty layers have VelStart = 0", allEmptyVelStart0);
                    reportTest("XPM rules: keygroups created", keygroupCount > 0);
                }
            }
        }
    }
}

//==============================================================================
// 2. WAV Format Validation
//==============================================================================

static void testWAVFormat()
{
    std::cout << "\n--- WAV Format Validation ---\n";

    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name = "WAVTest";
    config.bundleId = "com.xo-ox.test.wav";
    config.outputDir = getTestOutputDir("wav_format");

    XOriginate::RenderSettings settings;
    settings.sampleRate = 48000.0;
    settings.bitDepth = 24;
    settings.renderSeconds = 1.0f;
    settings.tailSeconds = 0.5f;
    settings.noteStrategy = XOriginate::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 1;

    std::vector<PresetData> presets = { makeTestPreset("WAVCheck") };
    auto result = exporter.exportBundle(config, settings, presets);
    reportTest("WAV format: export succeeds", result.success);
    reportTest("WAV format: samples rendered > 0", result.samplesRendered > 0);

    // Find WAV files
    auto bundleDir = config.outputDir.getChildFile("WAVTest");
    auto wavFiles = bundleDir.findChildFiles(juce::File::findFiles, true, "*.WAV");
    reportTest("WAV format: WAV files created", wavFiles.size() > 0);

    if (wavFiles.size() > 0)
    {
        juce::WavAudioFormat wav;
        auto stream = wavFiles[0].createInputStream();
        reportTest("WAV format: can open file stream", stream != nullptr);

        if (stream != nullptr)
        {
            auto reader = std::unique_ptr<juce::AudioFormatReader>(
                wav.createReaderFor(stream.release(), true));
            reportTest("WAV format: valid WAV header", reader != nullptr);

            if (reader != nullptr)
            {
                reportTest("WAV format: sample rate matches",
                           std::abs(reader->sampleRate - 48000.0) < 1.0);
                reportTest("WAV format: bit depth matches",
                           reader->bitsPerSample == 24);
                reportTest("WAV format: stereo",
                           reader->numChannels == 2);
                reportTest("WAV format: non-zero length",
                           reader->lengthInSamples > 0);

                // Expected length: (renderSeconds + tailSeconds) * sampleRate
                int64_t expectedSamples = (int64_t)(1.5 * 48000.0);
                reportTest("WAV format: correct duration",
                           reader->lengthInSamples == expectedSamples);
            }
        }
    }
}

//==============================================================================
// 3. Filename Sanitization Tests
//==============================================================================

static void testFilenameSanitization()
{
    std::cout << "\n--- Filename Sanitization ---\n";

    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name = "SanitizeTest";
    config.bundleId = "com.xo-ox.test.sanitize";
    config.outputDir = getTestOutputDir("sanitize");

    XOriginate::RenderSettings settings;
    settings.noteStrategy = XOriginate::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 1;
    settings.renderSeconds = 0.1f;
    settings.tailSeconds = 0.1f;

    // Preset with special characters in name
    auto preset = makeTestPreset("Test/Preset:With*Special<Chars>");
    std::vector<PresetData> presets = { preset };
    auto result = exporter.exportBundle(config, settings, presets);
    reportTest("Sanitize: export succeeds with special chars", result.success);

    // Verify no illegal chars in output paths
    auto bundleDir = config.outputDir.getChildFile("SanitizeTest");
    auto allFiles = bundleDir.findChildFiles(juce::File::findFiles, true);
    bool allClean = true;
    for (const auto& f : allFiles)
    {
        auto name = f.getFileName();
        if (name.containsAnyOf("/\\:*?\"<>|"))
            allClean = false;
    }
    reportTest("Sanitize: no illegal chars in filenames", allClean);

    // Test long name truncation
    juce::String longName;
    for (int i = 0; i < 100; ++i) longName += "A";
    auto longPreset = makeTestPreset("placeholder");
    longPreset.name = longName;
    std::vector<PresetData> longPresets = { longPreset };

    config.outputDir = getTestOutputDir("sanitize_long");
    auto longResult = exporter.exportBundle(config, settings, longPresets);
    reportTest("Sanitize: long name export succeeds", longResult.success);
}

//==============================================================================
// 4. Note Strategy Tests
//==============================================================================

static void testNoteStrategies()
{
    std::cout << "\n--- Note Strategy Coverage ---\n";

    XOriginate exporter;
    XOriginate::RenderSettings settings;
    settings.velocityLayers = 1;
    settings.renderSeconds = 0.1f;
    settings.tailSeconds = 0.1f;

    // Test each strategy produces expected note counts
    struct StrategyTest {
        XOriginate::RenderSettings::NoteStrategy strategy;
        const char* name;
        int expectedMin;   // minimum expected notes
        int expectedMax;   // maximum expected notes
    };

    StrategyTest tests[] = {
        { XOriginate::RenderSettings::NoteStrategy::OctavesOnly,   "OctavesOnly",   6, 7 },
        { XOriginate::RenderSettings::NoteStrategy::EveryFifth,    "EveryFifth",    10, 12 },
        { XOriginate::RenderSettings::NoteStrategy::EveryMinor3rd, "EveryMinor3rd", 24, 26 },
        { XOriginate::RenderSettings::NoteStrategy::Chromatic,     "Chromatic",     72, 74 },
    };

    for (auto& t : tests)
    {
        settings.noteStrategy = t.strategy;
        XOriginate::BundleConfig config;
        config.name = juce::String("Strategy_") + t.name;
        config.bundleId = juce::String("com.xo-ox.test.") + t.name;
        config.outputDir = getTestOutputDir(t.name);

        std::vector<PresetData> presets = { makeTestPreset("NoteTest") };
        auto result = exporter.exportBundle(config, settings, presets);

        juce::String testLabel = juce::String(t.name) + ": correct sample count";
        bool inRange = result.samplesRendered >= t.expectedMin
                    && result.samplesRendered <= t.expectedMax;
        reportTest(testLabel.toRawUTF8(), inRange);
    }
}

//==============================================================================
// 5. Velocity Layer Range Tests
//
// Fixed: parse actual MPCVObject > Program > Keygroups > Keygroup > Layers > Layer
// structure with child text elements (not Zone attributes).
//==============================================================================

static void testVelocityLayerRanges()
{
    std::cout << "\n--- Velocity Layer Ranges ---\n";

    XOriginate exporter;

    for (int layers = 1; layers <= 4; ++layers)
    {
        XOriginate::BundleConfig config;
        config.name = juce::String("VelTest_") + juce::String(layers);
        config.bundleId = "com.xo-ox.test.vel";
        config.outputDir = getTestOutputDir(("vel_" + std::to_string(layers)).c_str());

        XOriginate::RenderSettings settings;
        settings.noteStrategy = XOriginate::RenderSettings::NoteStrategy::OctavesOnly;
        settings.velocityLayers = layers;
        settings.renderSeconds = 0.1f;
        settings.tailSeconds = 0.1f;
        settings.dnaAdaptiveVelocity = false; // deterministic splits for testing

        std::vector<PresetData> presets = { makeTestPreset("VelCheck") };
        auto result = exporter.exportBundle(config, settings, presets);

        // Parse XPM and check velocity ranges
        auto bundleDir = config.outputDir.getChildFile(config.name.replace(" ", "_"));
        auto xpmFiles = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");

        if (xpmFiles.size() == 1)
        {
            auto xml = parseXmlFile(xpmFiles[0]);
            if (xml != nullptr)
            {
                // Navigate: MPCVObject > Program > Keygroups > first Keygroup
                auto* program = xml->getChildByName("Program");
                auto* keygroups = program ? program->getChildByName("Keygroups") : nullptr;
                const juce::XmlElement* firstKg = nullptr;

                if (keygroups != nullptr)
                {
                    for (auto* kg = keygroups->getFirstChildElement(); kg; kg = kg->getNextElement())
                    {
                        if (kg->getTagName() == "Keygroup") { firstKg = kg; break; }
                    }
                }

                if (firstKg != nullptr)
                {
                    auto* layersElem = firstKg->getChildByName("Layers");
                    if (layersElem != nullptr)
                    {
                        bool noGaps = true;
                        int prevEnd = -1;
                        int minVelStart = 128;
                        int maxVelEnd = -1;

                        int activeLayerCount = 0;
                        for (auto* layer = layersElem->getFirstChildElement();
                             layer; layer = layer->getNextElement())
                        {
                            if (layer->getTagName() != "Layer") continue;

                            int velStart = getChildInt(layer, "VelStart");
                            int velEnd   = getChildInt(layer, "VelEnd");

                            // Skip empty/padding layers (VelEnd == 0)
                            if (velEnd == 0 && activeLayerCount >= layers)
                                continue;

                            activeLayerCount++;
                            if (velStart < minVelStart) minVelStart = velStart;
                            if (velEnd > maxVelEnd) maxVelEnd = velEnd;

                            // Check contiguity
                            if (prevEnd >= 0 && velStart != prevEnd + 1)
                                noGaps = false;
                            prevEnd = velEnd;
                        }

                        bool coversFullRange = (minVelStart == 0 && maxVelEnd == 127);

                        juce::String label = juce::String(layers) + " layers: ";
                        reportTest((label + "no velocity gaps").toRawUTF8(), noGaps);
                        reportTest((label + "covers 0-127").toRawUTF8(), coversFullRange);
                    }
                }
            }
        }
    }
}

//==============================================================================
// 6. Batch Validation Tests
//==============================================================================

static void testBatchValidation()
{
    std::cout << "\n--- Batch Validation ---\n";

    // Duplicate names detected
    {
        std::vector<PresetData> dupes = {
            makeTestPreset("SameName"),
            makeTestPreset("SameName"),
        };
        auto result = XOriginate::validateBatch(dupes);
        reportTest("Batch: duplicate names detected", !result.valid);
        bool hasDupeError = false;
        for (const auto& e : result.errors)
            if (e.contains("Duplicate")) hasDupeError = true;
        reportTest("Batch: error mentions 'Duplicate'", hasDupeError);
    }

    // Empty name is an error
    {
        auto p = makeTestPreset("");
        std::vector<PresetData> batch = { p };
        auto result = XOriginate::validateBatch(batch);
        reportTest("Batch: empty name is error", !result.valid);
    }

    // DNA out of range
    {
        auto p = makeTestPreset("BadDNA");
        p.dna.brightness = 1.5f;
        std::vector<PresetData> batch = { p };
        auto result = XOriginate::validateBatch(batch);
        reportTest("Batch: out-of-range DNA is error", !result.valid);
    }

    // Name > 30 chars is a warning (not error)
    {
        auto p = makeTestPreset("ThisNameIsDefinitelyLongerThan30Characters");
        std::vector<PresetData> batch = { p };
        auto result = XOriginate::validateBatch(batch);
        reportTest("Batch: long name is warning not error", result.valid);
        reportTest("Batch: has warning for long name", result.warnings.size() > 0);
    }

    // Valid batch passes
    {
        std::vector<PresetData> good = {
            makeTestPreset("Alpha"),
            makeTestPreset("Beta"),
            makeTestPreset("Gamma"),
        };
        auto result = XOriginate::validateBatch(good);
        reportTest("Batch: valid batch passes", result.valid);
        reportTest("Batch: no errors on valid batch", result.errors.isEmpty());
    }
}

//==============================================================================
// 7. Bundle Structure Tests
//==============================================================================

static void testBundleStructure()
{
    std::cout << "\n--- Bundle Structure ---\n";

    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name = "StructureTest";
    config.bundleId = "com.xo-ox.test.structure";
    config.manufacturer = "XO_OX Designs";
    config.version = "1.2.3";
    config.description = "Structure test bundle";
    config.coverEngine = "ONSET";
    config.outputDir = getTestOutputDir("structure");

    XOriginate::RenderSettings settings;
    settings.noteStrategy = XOriginate::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 1;
    settings.renderSeconds = 0.1f;
    settings.tailSeconds = 0.1f;

    std::vector<PresetData> presets = {
        makeTestPreset("Pad Alpha"),
        makeTestPreset("Lead Beta"),
    };

    auto result = exporter.exportBundle(config, settings, presets);
    reportTest("Structure: export succeeds", result.success);
    reportTest("Structure: 2 presets exported", result.presetsExported == 2);

    auto bundleDir = config.outputDir.getChildFile("StructureTest");

    // Required files
    reportTest("Structure: Manifest.xml exists",
               bundleDir.getChildFile("Manifest.xml").existsAsFile());
    reportTest("Structure: Preview.png exists",
               bundleDir.getChildFile("Preview.png").existsAsFile());
    reportTest("Structure: Keygroups/ exists",
               bundleDir.getChildFile("Keygroups").isDirectory());

    // Manifest content
    auto manifest = parseXmlFile(bundleDir.getChildFile("Manifest.xml"));
    reportTest("Structure: Manifest is valid XML", manifest != nullptr);
    if (manifest != nullptr)
    {
        reportTest("Structure: Manifest Name correct",
                   manifest->getStringAttribute("Name") == "StructureTest");
        reportTest("Structure: Manifest Manufacturer correct",
                   manifest->getStringAttribute("Manufacturer") == "XO_OX Designs");
        reportTest("Structure: Manifest Version correct",
                   manifest->getStringAttribute("Version") == "1.2.3");
        reportTest("Structure: Manifest PresetCount correct",
                   manifest->getIntAttribute("PresetCount") == 2);
    }

    // XPM files for each preset
    auto xpmFiles = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");
    reportTest("Structure: 2 XPM files created", xpmFiles.size() == 2);

    // Cover art files
    reportTest("Structure: artwork.png exists",
               bundleDir.getChildFile("artwork.png").existsAsFile());
    reportTest("Structure: artwork_2000.png exists",
               bundleDir.getChildFile("artwork_2000.png").existsAsFile());

    // Total size tracking
    reportTest("Structure: totalSizeBytes > 0", result.totalSizeBytes > 0);
}

//==============================================================================
// 8. Cancel Mid-Export Tests
//==============================================================================

static void testCancelMidExport()
{
    std::cout << "\n--- Cancel Mid-Export ---\n";

    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name = "CancelTest";
    config.bundleId = "com.xo-ox.test.cancel";
    config.outputDir = getTestOutputDir("cancel");

    XOriginate::RenderSettings settings;
    settings.noteStrategy = XOriginate::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 1;
    settings.renderSeconds = 0.1f;
    settings.tailSeconds = 0.1f;

    // Create 10 presets, cancel after 2nd
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

    reportTest("Cancel: export reports not successful", !result.success);
    reportTest("Cancel: fewer presets exported than total",
               result.presetsExported < (int)presets.size());
}

//==============================================================================
// 9. Empty Preset List Test
//==============================================================================

static void testEmptyPresetList()
{
    std::cout << "\n--- Empty Preset List ---\n";

    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name = "EmptyTest";
    config.bundleId = "com.xo-ox.test.empty";
    config.outputDir = getTestOutputDir("empty");

    XOriginate::RenderSettings settings;
    std::vector<PresetData> empty;

    auto result = exporter.exportBundle(config, settings, empty);
    reportTest("Empty: export returns failure", !result.success);
    reportTest("Empty: error message set", result.errorMessage.isNotEmpty());
    reportTest("Empty: zero presets exported", result.presetsExported == 0);
}

//==============================================================================
// 10. Cover Art Regression Tests
//==============================================================================

static void testCoverArtRegression()
{
    std::cout << "\n--- Cover Art Regression ---\n";

    // All 20 engine IDs resolve to correct label
    struct EngineCheck {
        const char* id;
        const char* expectedLabel;
    };

    EngineCheck engines[] = {
        { "ONSET",     "ONSET" },
        { "OVERWORLD", "OVERWORLD" },
        { "ODDFELIX",  "OddfeliX" },
        { "ODDOSCAR",  "OddOscar" },
        { "OVERDUB",   "OVERDUB" },
        { "ODYSSEY",   "ODYSSEY" },
        { "OBLONG",    "OBLONG" },
        { "OBESE",     "OBESE" },
        { "OPAL",      "OPAL" },
        { "ORGANON",   "ORGANON" },
        { "OUROBOROS",  "OUROBOROS" },
        { "OBSIDIAN",  "OBSIDIAN" },
        { "OVERBITE",  "OVERBITE" },
        { "ORIGAMI",   "ORIGAMI" },
        { "ORACLE",    "ORACLE" },
        { "OBSCURA",   "OBSCURA" },
        { "OCEANIC",   "OCEANIC" },
        { "OPTIC",     "OPTIC" },
        { "OBLIQUE",   "OBLIQUE" },
        { "ORBITAL",   "ORBITAL" },
    };

    for (auto& e : engines)
    {
        auto def = XPNCoverArt::getEngineDef(e.id);
        juce::String testName = juce::String(e.id) + " resolves to " + e.expectedLabel;
        reportTest(testName.toRawUTF8(),
                   juce::String(def.label) == juce::String(e.expectedLabel));
    }

    // Legacy aliases resolve correctly
    struct AliasCheck {
        const char* legacy;
        const char* expectedLabel;
    };

    AliasCheck aliases[] = {
        { "SNAP",  "OddfeliX" },
        { "MORPH", "OddOscar" },
        { "DUB",   "OVERDUB" },
        { "DRIFT", "ODYSSEY" },
        { "BOB",   "OBLONG" },
        { "FAT",   "OBESE" },
    };

    for (auto& a : aliases)
    {
        auto def = XPNCoverArt::getEngineDef(a.legacy);
        juce::String testName = juce::String("Legacy ") + a.legacy + " -> " + a.expectedLabel;
        reportTest(testName.toRawUTF8(),
                   juce::String(def.label) == juce::String(a.expectedLabel));
    }

    // Generate cover art and verify output files
    {
        auto outputDir = getTestOutputDir("cover_art");
        auto result = XPNCoverArt::generate("ONSET", "Test Pack", outputDir, 10, "1.0", 42);
        reportTest("Cover art: generation succeeds", result.success);
        reportTest("Cover art: artwork.png exists", result.cover1000.existsAsFile());
        reportTest("Cover art: artwork_2000.png exists", result.cover2000.existsAsFile());

        // Check file sizes are reasonable (>1KB for a PNG)
        reportTest("Cover art: 1000px file has content",
                   result.cover1000.getSize() > 1024);
        reportTest("Cover art: 2000px file has content",
                   result.cover2000.getSize() > 1024);
        reportTest("Cover art: 2000px larger than 1000px",
                   result.cover2000.getSize() > result.cover1000.getSize());
    }

    // Default/unknown engine falls back gracefully
    {
        auto def = XPNCoverArt::getEngineDef("NONEXISTENT_ENGINE");
        reportTest("Cover art: unknown engine gets default label",
                   juce::String(def.label) == "XO_OX");
    }
}

//==============================================================================
// 11. XPM Keygroup Coverage Tests
//
// Fixed: parse actual Keygroup structure with child text elements,
// not Zone attributes.
//==============================================================================

static void testXPMKeygroupCoverage()
{
    std::cout << "\n--- XPM Keygroup Coverage ---\n";

    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name = "KeygroupCoverage";
    config.bundleId = "com.xo-ox.test.keygroups";
    config.outputDir = getTestOutputDir("keygroups");

    XOriginate::RenderSettings settings;
    settings.noteStrategy = XOriginate::RenderSettings::NoteStrategy::EveryMinor3rd;
    settings.velocityLayers = 3;
    settings.renderSeconds = 0.1f;
    settings.tailSeconds = 0.1f;

    std::vector<PresetData> presets = { makeTestPreset("KeygroupCheck") };
    exporter.exportBundle(config, settings, presets);

    auto bundleDir = config.outputDir.getChildFile("KeygroupCoverage");
    auto xpmFiles = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");

    if (xpmFiles.size() == 1)
    {
        auto xml = parseXmlFile(xpmFiles[0]);
        if (xml != nullptr)
        {
            auto* program = xml->getChildByName("Program");
            auto* keygroups = program ? program->getChildByName("Keygroups") : nullptr;

            if (keygroups != nullptr)
            {
                // Collect all key ranges from Keygroup child text elements
                std::set<int> coveredKeys;
                bool noOverlap = true;
                int prevHighKey = -1;

                for (auto* kg = keygroups->getFirstChildElement(); kg; kg = kg->getNextElement())
                {
                    if (kg->getTagName() != "Keygroup") continue;

                    int lowKey  = getChildInt(kg, "LowNote");
                    int highKey = getChildInt(kg, "HighNote");

                    // Check no overlap with previous keygroup
                    if (prevHighKey >= 0 && lowKey <= prevHighKey)
                        noOverlap = false;

                    for (int k = lowKey; k <= highKey; ++k)
                        coveredKeys.insert(k);

                    prevHighKey = highKey;

                    // Check sample file references in layers
                    auto* layersElem = kg->getChildByName("Layers");
                    if (layersElem != nullptr)
                    {
                        for (auto* layer = layersElem->getFirstChildElement();
                             layer; layer = layer->getNextElement())
                        {
                            if (layer->getTagName() != "Layer") continue;
                            auto sampleName = getChildText(layer, "SampleName");
                            int velEnd = getChildInt(layer, "VelEnd");
                            if (sampleName.isNotEmpty() && velEnd > 0)
                            {
                                auto fullPath = bundleDir.getChildFile("Keygroups")
                                                    .getChildFile(sampleName);
                                // Format is valid (file existence checked separately)
                            }
                        }
                    }
                }

                // Key 0 and 127 should be covered
                reportTest("Keygroup coverage: key 0 covered", coveredKeys.count(0) > 0);
                reportTest("Keygroup coverage: key 127 covered", coveredKeys.count(127) > 0);
                reportTest("Keygroup coverage: no keygroup overlaps", noOverlap);

                // Check contiguity (no gaps in key range)
                bool contiguous = true;
                for (int k = 0; k <= 127; ++k)
                {
                    if (coveredKeys.count(k) == 0)
                    {
                        contiguous = false;
                        break;
                    }
                }
                reportTest("Keygroup coverage: all 128 keys covered", contiguous);
            }
        }
    }
}

//==============================================================================
// 12. Fade Guard Verification
//
// Checks that the first and last N samples of rendered WAVs are near zero
// to prevent clicks at loop boundaries.
//==============================================================================

static void testFadeGuard()
{
    std::cout << "\n--- Fade Guard Verification ---\n";

    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name = "FadeGuard";
    config.bundleId = "com.xo-ox.test.fadeguard";
    config.outputDir = getTestOutputDir("fadeguard");

    XOriginate::RenderSettings settings;
    settings.noteStrategy = XOriginate::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 1;
    settings.renderSeconds = 0.5f;
    settings.tailSeconds = 1.0f; // generous tail so release decays to near-zero

    std::vector<PresetData> presets = { makeTestPreset("FadeCheck") };
    auto result = exporter.exportBundle(config, settings, presets);
    reportTest("Fade guard: export succeeds", result.success);

    auto bundleDir = config.outputDir.getChildFile("FadeGuard");
    auto wavFiles = bundleDir.findChildFiles(juce::File::findFiles, true, "*.WAV");
    reportTest("Fade guard: WAV files exist", wavFiles.size() > 0);

    if (wavFiles.size() > 0)
    {
        juce::WavAudioFormat wav;
        auto stream = wavFiles[0].createInputStream();

        if (stream != nullptr)
        {
            auto reader = std::unique_ptr<juce::AudioFormatReader>(
                wav.createReaderFor(stream.release(), true));

            if (reader != nullptr && reader->lengthInSamples > 0)
            {
                // Check first 64 samples are near zero (attack hasn't peaked yet)
                constexpr int guardSamples = 64;
                constexpr float threshold = 0.1f; // generous threshold for attack ramp

                juce::AudioBuffer<float> startBuf(
                    (int)reader->numChannels, guardSamples);
                reader->read(&startBuf, 0, guardSamples, 0, true, true);

                float startPeak = 0.0f;
                for (int ch = 0; ch < startBuf.getNumChannels(); ++ch)
                {
                    auto range = startBuf.findMinMax(ch, 0, guardSamples);
                    float chPeak = juce::jmax(
                        std::abs(range.getStart()), std::abs(range.getEnd()));
                    if (chPeak > startPeak) startPeak = chPeak;
                }
                reportTest("Fade guard: first 64 samples near zero",
                           startPeak < threshold);

                // Check last 64 samples are near zero (tail has decayed)
                int64_t tailStart = reader->lengthInSamples - guardSamples;
                if (tailStart > 0)
                {
                    juce::AudioBuffer<float> endBuf(
                        (int)reader->numChannels, guardSamples);
                    reader->read(&endBuf, 0, guardSamples, tailStart, true, true);

                    float endPeak = 0.0f;
                    for (int ch = 0; ch < endBuf.getNumChannels(); ++ch)
                    {
                        auto range = endBuf.findMinMax(ch, 0, guardSamples);
                        float chPeak = juce::jmax(
                            std::abs(range.getStart()), std::abs(range.getEnd()));
                        if (chPeak > endPeak) endPeak = chPeak;
                    }
                    reportTest("Fade guard: last 64 samples near zero",
                               endPeak < threshold);
                }
            }
        }
    }
}

//==============================================================================
// 13. Group Normalization
//
// Verifies that when multiple velocity layers are rendered, the loudest layer
// (hard) is at the ceiling and softer layers are proportionally quieter.
//==============================================================================

static void testGroupNormalization()
{
    std::cout << "\n--- Group Normalization ---\n";

    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name = "NormTest";
    config.bundleId = "com.xo-ox.test.norm";
    config.outputDir = getTestOutputDir("norm");

    XOriginate::RenderSettings settings;
    settings.noteStrategy = XOriginate::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 4;
    settings.renderSeconds = 0.5f;
    settings.tailSeconds = 0.5f;

    std::vector<PresetData> presets = { makeTestPreset("NormCheck") };
    auto result = exporter.exportBundle(config, settings, presets);
    reportTest("Group norm: export succeeds", result.success);

    // Find WAV files for one note across all 4 velocity layers
    auto bundleDir = config.outputDir.getChildFile("NormTest");
    auto allWavs = bundleDir.findChildFiles(juce::File::findFiles, true, "*.WAV");

    // Group WAVs by note (they share the note name, differ by __v1..v4)
    // Just check that v4 (hard) has higher peak than v1 (ghost)
    juce::File v1File, v4File;
    for (const auto& f : allWavs)
    {
        auto name = f.getFileName();
        if (name.contains("__v1.WAV") && v1File == juce::File{})
            v1File = f;
        if (name.contains("__v4.WAV") && v4File == juce::File{})
            v4File = f;
    }

    reportTest("Group norm: found v1 (ghost) file", v1File.existsAsFile());
    reportTest("Group norm: found v4 (hard) file", v4File.existsAsFile());

    if (v1File.existsAsFile() && v4File.existsAsFile())
    {
        juce::WavAudioFormat wav;

        auto readPeak = [&](const juce::File& f) -> float
        {
            auto s = f.createInputStream();
            if (!s) return 0.0f;
            auto r = std::unique_ptr<juce::AudioFormatReader>(
                wav.createReaderFor(s.release(), true));
            if (!r || r->lengthInSamples == 0) return 0.0f;

            juce::AudioBuffer<float> buf((int)r->numChannels,
                                          (int)r->lengthInSamples);
            r->read(&buf, 0, (int)r->lengthInSamples, 0, true, true);

            float peak = 0.0f;
            for (int ch = 0; ch < buf.getNumChannels(); ++ch)
            {
                auto range = buf.findMinMax(ch, 0, buf.getNumSamples());
                float chPeak = juce::jmax(
                    std::abs(range.getStart()), std::abs(range.getEnd()));
                if (chPeak > peak) peak = chPeak;
            }
            return peak;
        };

        float peakV1 = readPeak(v1File);
        float peakV4 = readPeak(v4File);

        reportTest("Group norm: hard layer louder than ghost",
                   peakV4 > peakV1);
        reportTest("Group norm: hard layer near ceiling (> 0.8)",
                   peakV4 > 0.8f);
        reportTest("Group norm: ghost layer quieter (< hard)",
                   peakV1 < peakV4);
    }
}

//==============================================================================
// 14. One-Shot Mode in Drum XPM
//
// Verifies that every drum instrument has <TriggerMode>OneShot</TriggerMode>.
//==============================================================================

static void testOneShotModeDrum()
{
    std::cout << "\n--- One-Shot Mode (Drum) ---\n";

    XPNDrumExporter drumExporter;
    XPNDrumExporter::DrumExportConfig config;
    config.name = "OneShotTest";
    config.bundleId = "com.xo-ox.test.oneshot";
    config.outputDir = getTestOutputDir("oneshot");

    auto preset = makeTestPreset("DrumOneShot", "Onset");
    std::vector<PresetData> presets = { preset };

    auto result = drumExporter.exportDrumBundle(config, presets);
    reportTest("One-shot: drum export succeeds", result.success);

    auto bundleDir = config.outputDir.getChildFile("OneShotTest");
    auto xpmFiles = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");
    reportTest("One-shot: XPM file created", xpmFiles.size() == 1);

    if (xpmFiles.size() == 1)
    {
        auto xml = parseXmlFile(xpmFiles[0]);
        reportTest("One-shot: valid XML", xml != nullptr);

        if (xml != nullptr)
        {
            auto* program = xml->getChildByName("Program");
            auto* instruments = program ? program->getChildByName("Instruments") : nullptr;

            if (instruments != nullptr)
            {
                bool allOneShot = true;
                int instrumentCount = 0;

                for (auto* inst = instruments->getFirstChildElement();
                     inst; inst = inst->getNextElement())
                {
                    if (inst->getTagName() != "Instrument") continue;
                    instrumentCount++;

                    auto triggerMode = getChildText(inst, "TriggerMode");
                    if (triggerMode != "OneShot")
                        allOneShot = false;
                }

                reportTest("One-shot: instruments found",
                           instrumentCount == XPNDrumExporter::kNumPads);
                reportTest("One-shot: all pads have TriggerMode=OneShot",
                           allOneShot);
            }
        }
    }
}

//==============================================================================
// 15. Expression Mapping in Keygroup XPM
//
// Verifies AfterTouch, ModWheel, and PitchBendRange in keygroup programs.
//==============================================================================

static void testExpressionMappingKeygroup()
{
    std::cout << "\n--- Expression Mapping (Keygroup) ---\n";

    XOriginate exporter;
    XOriginate::BundleConfig config;
    config.name = "ExprTest";
    config.bundleId = "com.xo-ox.test.expression";
    config.outputDir = getTestOutputDir("expression");

    XOriginate::RenderSettings settings;
    settings.noteStrategy = XOriginate::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 1;
    settings.renderSeconds = 0.1f;
    settings.tailSeconds = 0.1f;

    std::vector<PresetData> presets = { makeTestPreset("ExprCheck") };
    auto result = exporter.exportBundle(config, settings, presets);
    reportTest("Expression (keygroup): export succeeds", result.success);

    auto bundleDir = config.outputDir.getChildFile("ExprTest");
    auto xpmFiles = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");

    if (xpmFiles.size() == 1)
    {
        auto xml = parseXmlFile(xpmFiles[0]);
        if (xml != nullptr)
        {
            auto* program = xml->getChildByName("Program");
            reportTest("Expression (keygroup): Program exists", program != nullptr);

            if (program != nullptr)
            {
                // AfterTouch
                auto* afterTouch = program->getChildByName("AfterTouch");
                reportTest("Expression: AfterTouch element exists",
                           afterTouch != nullptr);
                if (afterTouch != nullptr)
                {
                    reportTest("Expression: AfterTouch destination = FilterCutoff",
                               getChildText(afterTouch, "Destination") == "FilterCutoff");
                    reportTest("Expression: AfterTouch amount = 50",
                               getChildText(afterTouch, "Amount") == "50");
                }

                // ModWheel
                auto* modWheel = program->getChildByName("ModWheel");
                reportTest("Expression: ModWheel element exists",
                           modWheel != nullptr);
                if (modWheel != nullptr)
                {
                    reportTest("Expression: ModWheel destination = FilterCutoff",
                               getChildText(modWheel, "Destination") == "FilterCutoff");
                    reportTest("Expression: ModWheel amount = 70",
                               getChildText(modWheel, "Amount") == "70");
                }

                // PitchBendRange
                reportTest("Expression: PitchBendRange = 12",
                           getChildText(program, "PitchBendRange") == "12");
            }
        }
    }
}

//==============================================================================
// 16. Expression Mapping in Drum XPM
//
// Verifies AfterTouch in drum programs (simpler than keygroup).
//==============================================================================

static void testExpressionMappingDrum()
{
    std::cout << "\n--- Expression Mapping (Drum) ---\n";

    XPNDrumExporter drumExporter;
    XPNDrumExporter::DrumExportConfig config;
    config.name = "DrumExprTest";
    config.bundleId = "com.xo-ox.test.drumexpr";
    config.outputDir = getTestOutputDir("drumexpr");

    auto preset = makeTestPreset("DrumExprCheck", "Onset");
    std::vector<PresetData> presets = { preset };

    auto result = drumExporter.exportDrumBundle(config, presets);
    reportTest("Expression (drum): export succeeds", result.success);

    auto bundleDir = config.outputDir.getChildFile("DrumExprTest");
    auto xpmFiles = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");

    if (xpmFiles.size() == 1)
    {
        auto xml = parseXmlFile(xpmFiles[0]);
        if (xml != nullptr)
        {
            auto* program = xml->getChildByName("Program");
            if (program != nullptr)
            {
                auto* afterTouch = program->getChildByName("AfterTouch");
                reportTest("Expression (drum): AfterTouch exists",
                           afterTouch != nullptr);
                if (afterTouch != nullptr)
                {
                    reportTest("Expression (drum): AfterTouch dest = FilterCutoff",
                               getChildText(afterTouch, "Destination") == "FilterCutoff");
                    reportTest("Expression (drum): AfterTouch amount = 30",
                               getChildText(afterTouch, "Amount") == "30");
                }
            }
        }
    }
}

//==============================================================================
// 17. Pad Color in Drum XPM
//
// Verifies that PadColor elements are present and contain valid hex strings.
//==============================================================================

static void testPadColorDrum()
{
    std::cout << "\n--- Pad Color (Drum) ---\n";

    XPNDrumExporter drumExporter;
    XPNDrumExporter::DrumExportConfig config;
    config.name = "PadColorTest";
    config.bundleId = "com.xo-ox.test.padcolor";
    config.outputDir = getTestOutputDir("padcolor");

    auto preset = makeTestPreset("PadColorCheck", "Onset");
    std::vector<PresetData> presets = { preset };

    auto result = drumExporter.exportDrumBundle(config, presets);
    reportTest("Pad color: drum export succeeds", result.success);

    auto bundleDir = config.outputDir.getChildFile("PadColorTest");
    auto xpmFiles = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");

    if (xpmFiles.size() == 1)
    {
        auto xml = parseXmlFile(xpmFiles[0]);
        if (xml != nullptr)
        {
            auto* program = xml->getChildByName("Program");
            auto* instruments = program ? program->getChildByName("Instruments") : nullptr;

            if (instruments != nullptr)
            {
                bool allHaveColor = true;
                bool allValidHex = true;
                int instrumentCount = 0;

                for (auto* inst = instruments->getFirstChildElement();
                     inst; inst = inst->getNextElement())
                {
                    if (inst->getTagName() != "Instrument") continue;
                    instrumentCount++;

                    auto color = getChildText(inst, "PadColor");
                    if (color.isEmpty())
                        allHaveColor = false;
                    else if (color.length() != 6 ||
                             !color.containsOnly("0123456789ABCDEFabcdef"))
                        allValidHex = false;
                }

                reportTest("Pad color: all instruments have PadColor",
                           allHaveColor && instrumentCount > 0);
                reportTest("Pad color: all colors are valid 6-digit hex",
                           allValidHex && instrumentCount > 0);
            }
        }
    }
}

//==============================================================================
// 18. Mute Group Configuration
//
// Verifies that configurable mute groups are present beyond just hi-hat.
//==============================================================================

static void testMuteGroups()
{
    std::cout << "\n--- Mute Group Configuration ---\n";

    XPNDrumExporter drumExporter;
    XPNDrumExporter::DrumExportConfig config;
    config.name = "MuteGroupTest";
    config.bundleId = "com.xo-ox.test.mutegroups";
    config.outputDir = getTestOutputDir("mutegroups");

    auto preset = makeTestPreset("MuteCheck", "Onset");
    std::vector<PresetData> presets = { preset };

    auto result = drumExporter.exportDrumBundle(config, presets);
    reportTest("Mute groups: drum export succeeds", result.success);

    auto bundleDir = config.outputDir.getChildFile("MuteGroupTest");
    auto xpmFiles = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");

    if (xpmFiles.size() == 1)
    {
        auto xml = parseXmlFile(xpmFiles[0]);
        if (xml != nullptr)
        {
            auto* program = xml->getChildByName("Program");
            auto* instruments = program ? program->getChildByName("Instruments") : nullptr;

            if (instruments != nullptr)
            {
                std::set<int> muteGroupsSeen;
                bool hiHatGroupFound = false;
                bool kickGroupFound = false;
                bool snareGroupFound = false;

                for (auto* inst = instruments->getFirstChildElement();
                     inst; inst = inst->getNextElement())
                {
                    if (inst->getTagName() != "Instrument") continue;

                    int mg = getChildInt(inst, "MuteGroup", 0);
                    muteGroupsSeen.insert(mg);

                    auto name = getChildText(inst, "InstrumentName");

                    if (mg == XPNDrumExporter::kChokeHiHat &&
                        (name.containsIgnoreCase("HAT")))
                        hiHatGroupFound = true;

                    if (mg == XPNDrumExporter::kChokeKick &&
                        name.containsIgnoreCase("KICK"))
                        kickGroupFound = true;

                    if (mg == XPNDrumExporter::kChokeSnare &&
                        (name.containsIgnoreCase("SNARE") ||
                         name.containsIgnoreCase("CLAP")))
                        snareGroupFound = true;
                }

                reportTest("Mute groups: hi-hat choke group present",
                           hiHatGroupFound);
                reportTest("Mute groups: kick choke group present",
                           kickGroupFound);
                reportTest("Mute groups: snare/clap choke group present",
                           snareGroupFound);
                reportTest("Mute groups: multiple distinct groups used",
                           muteGroupsSeen.size() >= 3);
            }
        }
    }
}

//==============================================================================
// Public entry point
//==============================================================================

namespace export_tests {

int runAll()
{
    g_exportTestsPassed = 0;
    g_exportTestsFailed = 0;

    std::cout << "\n========================================\n";
    std::cout << "  XPN Export Tests\n";
    std::cout << "========================================\n";

    testXPMRuleEnforcement();
    testWAVFormat();
    testFilenameSanitization();
    testNoteStrategies();
    testVelocityLayerRanges();
    testBatchValidation();
    testBundleStructure();
    testCancelMidExport();
    testEmptyPresetList();
    testCoverArtRegression();
    testXPMKeygroupCoverage();
    testFadeGuard();
    testGroupNormalization();
    testOneShotModeDrum();
    testExpressionMappingKeygroup();
    testExpressionMappingDrum();
    testPadColorDrum();
    testMuteGroups();

    std::cout << "\n  Export Tests: " << g_exportTestsPassed << " passed, "
              << g_exportTestsFailed << " failed\n";

    return g_exportTestsFailed;
}

} // namespace export_tests
