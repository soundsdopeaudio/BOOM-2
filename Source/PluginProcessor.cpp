#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FlipUtils.h"
#include <random>

using AP = juce::AudioProcessorValueTreeState;

static AP::ParameterLayout createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back(std::make_unique<juce::AudioParameterChoice>("engine", "Engine", boom::engineChoices(), (int)boom::Engine::Drums));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("timeSig", "Time Signature", boom::timeSigChoices(), 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("bars", "Bars", boom::barsChoices(), 0));

    p.push_back(std::make_unique<juce::AudioParameterFloat>("humanizeTiming", "Humanize Timing", juce::NormalisableRange<float>(0.f, 100.f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("humanizeVelocity", "Humanize Velocity", juce::NormalisableRange<float>(0.f, 100.f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("swing", "Swing", juce::NormalisableRange<float>(0.f, 100.f), 0.f));

    p.push_back(std::make_unique<juce::AudioParameterBool>("useTriplets", "Triplets", false));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("tripletDensity", "Triplet Density", juce::NormalisableRange<float>(0.f, 100.f), 0.f));
    p.push_back(std::make_unique<juce::AudioParameterBool>("useDotted", "Dotted Notes", false));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("dottedDensity", "Dotted Density", juce::NormalisableRange<float>(0.f, 100.f), 0.f));

    p.push_back(std::make_unique<juce::AudioParameterChoice>("key", "Key", boom::keyChoices(), 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("scale", "Scale", boom::scaleChoices(), 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("octave", "Octave", juce::StringArray("-2", "-1", "0", "+1", "+2"), 2));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("restDensity808", "Rest Density 808", juce::NormalisableRange<float>(0.f, 100.f), 10.f));

    p.push_back(std::make_unique<juce::AudioParameterChoice>("bassStyle", "Bass Style", boom::styleChoices(), 0));

    p.push_back(std::make_unique<juce::AudioParameterChoice>("drumStyle", "Drum Style", boom::styleChoices(), 0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("restDensityDrums", "Rest Density Drums", juce::NormalisableRange<float>(0.f, 100.f), 5.f));

    p.push_back(std::make_unique<juce::AudioParameterInt>("seed", "Seed", 0, 1000000, 0));

    return { p.begin(), p.end() };
}

BoomAudioProcessor::BoomAudioProcessor()
    : juce::AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "PARAMS", createLayout())
{
}

void BoomAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    juce::MemoryOutputStream mos(dest, true);
    apvts.copyState().writeToStream(mos);
}

void BoomAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto vt = juce::ValueTree::readFromData(data, (size_t)sizeInBytes); vt.isValid())
        apvts.replaceState(vt);
}

void BoomAudioProcessor::bumpDrumRowsUp()
{
    if (drumPattern.isEmpty()) return;
    for (auto& n : drumPattern)
        n.row = (n.row + 1) % drumRows.size();
}

void BoomAudioProcessor::flipMelodic(int seed, int density, int bars)
{
    boom::flip::MelodicPattern mp;
    mp.ensureStorageAllocated(melodicPattern.size());
    for (auto& n : melodicPattern) mp.add({ n.pitch, n.startTick, n.lengthTicks, n.velocity, 1 });
    boom::flip::microFlipMelodic(mp, seed, density, bars);
    melodicPattern.clearQuick();
    for (auto& m : mp) melodicPattern.add({ m.pitch, 0, m.startTick, m.lengthTicks, m.velocity });
}

void BoomAudioProcessor::flipDrums(int seed, int density, int bars)
{
    boom::flip::DrumPattern dp;
    dp.ensureStorageAllocated(drumPattern.size());
    for (auto& n : drumPattern) dp.add({ n.row, n.startTick, n.lengthTicks, n.velocity });
    boom::flip::microFlipDrums(dp, seed, density, bars);
    drumPattern.clearQuick();
    for (auto& e : dp) drumPattern.add({ 0, e.row, e.startTick, e.lengthTicks, e.velocity });
}

void BoomAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    // Store the real sample rate for capture/transcription and size the ring buffer.
    lastSampleRate = (sampleRate > 0.0 ? sampleRate : 44100.0);
    ensureCaptureCapacitySeconds(65.0); // ~60s cap + a little margin
    captureWritePos = 0;
    captureLengthSamples = 0;
    isCapturing.store(false);
}

// IMPORTANT: This is where we append input audio to the capture ring buffer when recording.
// We keep the audio buffer silent because BOOM is a MIDI generator/transformer.
void BoomAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // If we’re recording (Rhythmimick/Beatbox), append the *input* audio as mono into our ring buffer.
    if (isCapturing.load())
    {
        // Mix all channels in 'buffer' down to mono into our capture
        // (Hosts route mic/loopback into the plugin input; in standalone, this is your device input/mix.)
        // We do this BEFORE clearing, so we actually capture what came in.
        appendCaptureFrom(buffer);
    }

    // We are a MIDI plugin/generator here -> silence audio out.
    buffer.clear();

    // If capturing, append *input* audio to mono capture
    if (isCapturing.load())
    {
        // Prefer input if present; if synth-only, use buffer (standalone “loopback” case)
        const juce::AudioBuffer<float>* inBuf = &buffer;
#if !JucePlugin_IsSynth
        if (getTotalNumInputChannels() > 0)
            inBuf = getBusBuffer(buffer, true, 0).getNumChannels() > 0 ? &buffer : &buffer;
#endif
        appendCaptureFrom(*inBuf);
    }

    // We don't generate live MIDI in this function (editor actions build/export patterns),
    // so just pass the incoming MIDI through untouched, or clear if you prefer.
    juce::ignoreUnused(midi);
}

void BoomAudioProcessor::releaseResources()
{
    // Nothing heavy to free, but make sure capture is stopped and pointers reset.
    isCapturing.store(false);
    captureWritePos = 0;
    captureLengthSamples = 0;
}

void BoomAudioProcessor::ensureCaptureCapacitySeconds(double seconds)
{
    const int needed = (int)std::ceil(seconds * lastSampleRate);
    if (captureBuffer.getNumSamples() < needed)
        captureBuffer.setSize(1, needed, false, true, true);
}

void BoomAudioProcessor::appendCaptureFrom(const juce::AudioBuffer<float>& in)
{
    if (in.getNumSamples() <= 0) return;

    // mix all channels to mono temp
    juce::AudioBuffer<float> mono(1, in.getNumSamples());
    mono.clear();
    const int chans = in.getNumChannels();
    for (int ch = 0; ch < chans; ++ch)
        mono.addFrom(0, 0, in, ch, 0, in.getNumSamples(), 1.0f / juce::jmax(1, chans));

    // write into ring buffer (linear until full; stop if full ~ 60s)
    const int free = captureBuffer.getNumSamples() - captureWritePos;
    const int n = juce::jmin(free, mono.getNumSamples());
    if (n > 0)
        captureBuffer.copyFrom(0, captureWritePos, mono, 0, 0, n);

    captureWritePos += n;
    captureLengthSamples = juce::jmax(captureLengthSamples, captureWritePos);

    // once full, stop capturing (hard stop at ~ 60s)
    if (captureWritePos >= captureBuffer.getNumSamples())
        isCapturing.store(false);
}

BoomAudioProcessor::Pattern BoomAudioProcessor::transcribeAudioToDrums(const float* mono, int N, int bars, int bpm) const
{
    Pattern pat;
    if (mono == nullptr || N <= 0) return pat;

    const int fs = (int)lastSampleRate;
    const int hop = 512;
    const int win = 1024;
    const float preEmph = 0.97f;

    const int stepsPerBar = 16;
    const int totalSteps = juce::jmax(1, bars) * stepsPerBar;
    const double secPerBeat = 60.0 / juce::jlimit(40, 240, bpm);
    const double secPerStep = secPerBeat / 4.0; // 16th
    const int ticksPerStep = 24;

    auto bandEnergy = [&](int start, int end) -> std::vector<float>
    {
        std::vector<float> env;
        env.reserve(N / hop + 8);

        for (int i = 0; i + win <= N; i += hop)
        {
            float e = 0.f;
            for (int n = 0; n < win; ++n)
            {
                float x = mono[i + n] - preEmph * (n > 0 ? mono[i + n - 1] : 0.f);
                float w = 1.f;
                if (start >= 200 && end <= 2000) w = 0.7f;
                if (start >= 5000)               w = 0.5f;
                e += std::abs(x) * w;
            }
            e /= (float)win;
            env.push_back(e);
        }

        float mx = 1e-6f;
        for (auto v : env) mx = juce::jmax(mx, v);
        for (auto& v : env) v /= mx;

        return env;
    };

    auto low = bandEnergy(20, 200);
    auto mid = bandEnergy(200, 2000);
    auto high = bandEnergy(5000, 20000);

    auto detectPeaks = [&](const std::vector<float>& e, float thr, int minGapFrames)
    {
        std::vector<int> frames;
        int last = -minGapFrames;
        for (int i = 1; i + 1 < (int)e.size(); ++i)
        {
            if (e[i] > thr && e[i] > e[i - 1] && e[i] >= e[i + 1] && (i - last) >= minGapFrames)
            {
                frames.push_back(i);
                last = i;
            }
        }
        return frames;
    };

    auto kFrames = detectPeaks(low, 0.35f, (int)std::round(0.040 * fs / hop));
    auto sFrames = detectPeaks(mid, 0.30f, (int)std::round(0.050 * fs / hop));
    auto hFrames = detectPeaks(high, 0.28f, (int)std::round(0.030 * fs / hop));

    auto frameToTick = [&](int frame) -> int
    {
        double t = (double)(frame * hop) / fs;
        int step = (int)std::round(t / secPerStep);
        step = (step % totalSteps + totalSteps) % totalSteps;
        return step * ticksPerStep;
    };

    auto addHits = [&](const std::vector<int>& frames, int row, int vel)
    {
        for (auto f : frames)
            pat.add({ 0, row, frameToTick(f), 12, vel });
    };

    // rows: 0 kick, 1 snare, 2 hat
    addHits(kFrames, 0, 115);
    addHits(sFrames, 1, 108);
    addHits(hFrames, 2, 80);

    return pat;
}

void BoomAudioProcessor::aiAnalyzeCapturedToDrums(int bars, int bpm)
{
    if (captureLengthSamples <= 0) return;
    const int N = juce::jmin(captureLengthSamples, captureBuffer.getNumSamples());
    auto* mono = captureBuffer.getReadPointer(0);
    auto pat = transcribeAudioToDrums(mono, N, bars, bpm);
    setDrumPattern(pat);
}


void BoomAudioProcessor::aiStartCapture(CaptureSource src)
{
    currentCapture = src;
    captureWritePos = 0;
    captureLengthSamples = 0;
    isCapturing.store(true);
}

void BoomAudioProcessor::aiStopCapture()
{
    isCapturing.store(false);
}

juce::AudioProcessorEditor* BoomAudioProcessor::createEditor()
{
    return new class BoomAudioProcessorEditor(*this);
    ensureCaptureCapacitySeconds(65.0); // up to 60s + margin

}

void BoomAudioProcessor::aiSlapsmithExpand(int bars)
{
    Pattern src = getDrumPattern(); // mini grid should have already set this
    Pattern out;

    const int stepsPerBar = 16, tps = 24, total = juce::jmax(1, bars) * stepsPerBar;

    // copy seed
    for (auto n : src) out.add(n);

    // add hats on 8ths, add ghost hats on off-8ths
    for (int s = 0; s < total; ++s)
    {
        int tick = s * tps;
        bool hasHat = false;
        for (auto& n : src) if (n.row == 2 && n.startTick == tick) { hasHat = true; break; }
        if (!hasHat && (s % 2 == 0)) out.add({ 0, 2, tick, 12, 78 });
        if (s % 2 == 1) out.add({ 0, 2, tick, 8, 58 }); // ghost
    }

    // add snare rolls into 4 and 12
    for (int s : { 4, 12 })
    {
        int t = s * tps;
        out.add({ 0, 1, t - 6, 8, 72 });
        out.add({ 0, 1, t,   24,110 });
        out.add({ 0, 1, t + 6, 8, 72 });
    }

    // occasional kick pickups before downbeats
    for (int b = 0; b < bars; ++b)
    {
        int t = b * stepsPerBar * tps;
        out.add({ 0, 0, t - 3 * tps, 12, 95 }); // & of 1
        out.add({ 0, 0, t,           24, 118 });
        out.add({ 0, 0, t + 8 * tps, 24, 112 });
    }

    setDrumPattern(out);
}

void BoomAudioProcessor::aiStyleBlendDrums(juce::String styleA, juce::String styleB, int bars)
{
    // Map style names to seeds/weights
    auto styleToSeed = [](const juce::String& s) -> int
    {
        return juce::String(s).hashCode(); // works across JUCE versions
    };

    const int seedA = styleToSeed(styleA);
    const int seedB = styleToSeed(styleB);
    std::mt19937 rngA((unsigned)seedA), rngB((unsigned)seedB);

    Pattern out;
    const int stepsPerBar = 16, tps = 24, total = juce::jmax(1, bars) * stepsPerBar;

    // A side: straight grooves
    for (int s = 0; s < total; ++s)
    {
        int t = s * tps;
        if (s % stepsPerBar == 0 || s % stepsPerBar == 8) out.add({ 0,0,t,24,118 }); // kicks
        if (s % stepsPerBar == 4 || s % stepsPerBar == 12) out.add({ 0,1,t,24,112 }); // snares
        if (s % 2 == 0) out.add({ 0,2,t,12,78 }); // hats
    }

    // B side: syncopations and tom/percs
    for (int s = 0; s < total; ++s)
    {
        int t = s * tps;
        if (s % 4 == 3) out.add({ 0,0,t,12,95 });   // pickup kicks
        if (s % 8 == 6) out.add({ 0,3,t,12,80 });   // perc
        if (s % 2 == 1) out.add({ 0,2,t,8,60 });    // ghost hats
    }

    setDrumPattern(out);
}


