#pragma once
#include <JuceHeader.h>
#include "EngineDefs.h"

class BoomAudioProcessor : public juce::AudioProcessor
{
public:
    BoomAudioProcessor();
    ~BoomAudioProcessor() override = default;

    // AudioProcessor
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override; // keep if already present
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        return layouts.getMainOutputChannelSet() != juce::AudioChannelSet::disabled();
    }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer& midi) override { midi.clear(); } // no playback
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "BOOM"; }
    juce::AudioProcessorEditor* createEditor() override;

    // MIDI effect flags
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Programs
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    // State
    void getStateInformation(juce::MemoryBlock& dest) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Pattern model used by the Editor
    struct Note
    {
        int pitch{60};   // used for 808/Bass
        int row{0};      // used for Drums
        int startTick{0};
        int lengthTicks{24};
        int velocity{100};
    };
    using Pattern = juce::Array<Note>;

    const Pattern& getDrumPattern() const noexcept   { return drumPattern; }
    const Pattern& getMelodicPattern() const noexcept{ return melodicPattern; }
    void setDrumPattern(const Pattern& p)            { drumPattern = p; }
    void setMelodicPattern(const Pattern& p)         { melodicPattern = p; }

    const juce::StringArray& getDrumRows() const { return drumRows; }

    // Simple transforms
    void bumpDrumRowsUp();
    void flipMelodic(int seed, int density, int bars);
    void flipDrums(int seed, int density, int bars);

    // APVTS
    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS apvts;

    // === AI: Audio Capture Sources ===
    enum class CaptureSource { Loopback, Microphone };

    // Start/stop capture; in plugin context both read from processBlock input.
    void aiStartCapture(CaptureSource src);
    void aiStopCapture();
    bool aiIsCapturing() const { return isCapturing.load(); }

    // Transcribe captured audio into a drum pattern (kick/snare/hat) for given bars/bpm
    void aiAnalyzeCapturedToDrums(int bars, int bpm);

    // Slapsmith: expand current drum pattern (or provided seed pattern already set by editor) into a fuller groove
    void aiSlapsmithExpand(int bars);

    // StyleBlender: blend two style names from your style combo boxes into a new drum groove
    void aiStyleBlendDrums(juce::String styleA, juce::String styleB, int bars);


private:
    Pattern drumPattern, melodicPattern;
    juce::StringArray drumRows { boom::defaultDrumRows() };

    // === AI: capture ring buffer ===
    std::atomic<bool> isCapturing { false };
    CaptureSource currentCapture{ CaptureSource::Loopback };

    juce::AudioBuffer<float> captureBuffer;   // mono scratch buffer
    int captureWritePos = 0;
    int captureLengthSamples = 0;             // how many valid samples recorded
    double lastSampleRate = 44100.0;

    void appendCaptureFrom(const juce::AudioBuffer<float>& in);

    // Analysis helpers
    Pattern transcribeAudioToDrums(const float* mono, int numSamples, int bars, int bpm) const;
    void ensureCaptureCapacitySeconds(double seconds);


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoomAudioProcessor)
};
