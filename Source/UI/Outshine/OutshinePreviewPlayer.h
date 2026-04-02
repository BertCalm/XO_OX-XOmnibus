#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../GalleryColors.h"
#include "../../Export/XOutshine.h"

namespace xoceanus {

class OutshinePreviewPlayer : public juce::Component,
                              private juce::Timer
{
public:
    OutshinePreviewPlayer()
    {
        formatManager.registerBasicFormats();
        setWantsKeyboardFocus(true);
        A11y::setup(*this, "Preview Player", "Play C4 to preview the zone mapping");

        playC4Btn.setColour(juce::TextButton::buttonColourId,
                            GalleryColors::get(GalleryColors::slotBg()));
        playC4Btn.setColour(juce::TextButton::textColourOffId,
                            GalleryColors::get(GalleryColors::textDark()));
        playC4Btn.onClick = [this]() {
            if (playing) stopPlayback();
            else         playC4();
        };
        A11y::setup(playC4Btn, "Play C4", "Preview the instrument at middle C, velocity 100. Space to activate.");
        addAndMakeVisible(playC4Btn);

        // Advisory initial size — parent should call setBounds() to resize.
        // Use font metrics so the component is appropriately sized at any DPI/scale.
        const int btnH = juce::roundToInt(GalleryFonts::body(12.0f).getHeight() * 2.2f);
        setSize(juce::roundToInt(btnH * 3.75f), btnH);
    }

    ~OutshinePreviewPlayer() override
    {
        stopTimer();
        transportSource.stop();
        transportSource.setSource(nullptr);
        readerSource.reset();
        if (player)
        {
            deviceManager.removeAudioCallback(player.get());
            player.reset();
        }
    }

    void setSamples(const std::vector<AnalyzedSample>& samples)
    {
        currentSamples = samples;
    }

    void playC4()
    {
        const AnalyzedSample* sample = findSampleForMidi(60);
        if (!sample) return;

        transportSource.stop();
        transportSource.setSource(nullptr);
        readerSource.reset();

        std::unique_ptr<juce::AudioFormatReader> reader(
            formatManager.createReaderFor(sample->sourceFile));
        if (!reader) return;

        readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);
        transportSource.setSource(readerSource.get(), 0, nullptr,
                                  readerSource->getAudioFormatReader()->sampleRate);

        startAudioDevice();

        transportSource.setPosition(0.0);
        transportSource.start();
        playing = true;

        playC4Btn.setButtonText("Stop");
        startTimer((int)(kMaxPlaybackS * 1000.0f));
        repaint();
    }

    void stopPlayback()
    {
        stopTimer();
        transportSource.stop();
        playing = false;
        playC4Btn.setButtonText("Play C4");
        repaint();
    }

    bool isPlaying() const { return playing; }

    void focusGained(FocusChangeType) override { repaint(); }
    void focusLost(FocusChangeType) override { repaint(); }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));
        if (hasKeyboardFocus(true))
            A11y::drawFocusRing(g, getLocalBounds().toFloat(), 4.0f);
    }

    void resized() override
    {
        playC4Btn.setBounds(getLocalBounds());
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::spaceKey)
        {
            if (playing) stopPlayback();
            else         playC4();
            return true;
        }
        return false;
    }

private:
    void timerCallback() override
    {
        stopTimer();
        if (playing && (!transportSource.isPlaying()
                        || transportSource.getCurrentPosition() >= kMaxPlaybackS))
            stopPlayback();
    }

    const AnalyzedSample* findSampleForMidi(int midiNote) const
    {
        if (currentSamples.empty()) return nullptr;

        // Sort by detected MIDI note and find zone containing midiNote
        struct IndexedNote { int midi; int index; };
        std::vector<IndexedNote> sorted;
        for (int i = 0; i < (int)currentSamples.size(); ++i)
            sorted.push_back({ currentSamples[(size_t)i].detectedMidiNote, i });
        std::sort(sorted.begin(), sorted.end(), [](auto& a, auto& b) { return a.midi < b.midi; });

        for (int i = 0; i < (int)sorted.size(); ++i)
        {
            int low  = (i == 0) ? 0 : (sorted[(size_t)(i-1)].midi + sorted[(size_t)i].midi) / 2;
            int high = (i == (int)sorted.size() - 1) ? 127
                       : (sorted[(size_t)i].midi + sorted[(size_t)(i+1)].midi) / 2 - 1;
            if (midiNote >= low && midiNote <= high)
                return &currentSamples[(size_t)sorted[(size_t)i].index];
        }
        return &currentSamples[0];  // fallback to first sample
    }

    void startAudioDevice()
    {
        if (!player)
        {
            player = std::make_unique<juce::AudioSourcePlayer>();
            deviceManager.addAudioCallback(player.get());
        }

        if (deviceManager.getCurrentAudioDevice() == nullptr)
            deviceManager.initialiseWithDefaultDevices(0, 2);

        player->setSource(&transportSource);
    }

    juce::TextButton                               playC4Btn { "Play C4" };
    juce::AudioDeviceManager                       deviceManager;
    juce::AudioFormatManager                       formatManager;
    juce::AudioTransportSource                     transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    std::unique_ptr<juce::AudioSourcePlayer>       player;

    std::vector<AnalyzedSample>                    currentSamples;
    bool                                           playing { false };
    static constexpr float                         kMaxPlaybackS = 4.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshinePreviewPlayer)
};

} // namespace xoceanus
