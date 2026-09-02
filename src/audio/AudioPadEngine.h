#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>
#include "audio/AudioPadTypes.h"

class AudioPadEngine
{
public:
    static constexpr int padCount = 16;

    AudioPadEngine();

    void prepare (double sampleRate, int maxBlockSize);
    void stopAll (bool immediate = false);

    juce::String loadFile (int index, const juce::File& file);
    void clearPad (int index);
    void triggerPad (int index);
    void stopPad (int index);

    void setPadState (int index, const AudioPadState& state, bool reloadFile = true);
    AudioPadState getPadState (int index) const;
    juce::Array<AudioPadState> captureStates() const;
    void restoreStates (const juce::Array<AudioPadState>& states);

    void setMasterVolume (float v) { masterVolume.store (juce::jlimit (0.0f, 1.5f, v)); }
    float getMasterVolume() const { return masterVolume.load(); }

    bool isPlaying (int index) const;
    float progress (int index) const;

    // Renders three independent buses for this block. Buffers are resized/preallocated by
    // AudioEngine and are only cleared/written here; no allocation happens on the audio thread.
    void render (juce::AudioBuffer<float>& preFx,
                 juce::AudioBuffer<float>& postFx,
                 juce::AudioBuffer<float>& output2Only,
                 int numSamples);

    static bool supportsFile (const juce::File& file);

private:
    struct Clip
    {
        juce::AudioBuffer<float> audio;
        double sampleRate = 48000.0;
    };

    struct PadRuntime
    {
        std::shared_ptr<const Clip> clip;
        std::atomic<float> volume { 0.85f };
        std::atomic<int> loop { 0 };
        std::atomic<int> route { (int) AudioPadRoute::postFx };
        std::atomic<int> retrigger { (int) AudioPadRetrigger::restart };
        std::atomic<float> fadeInMs { 0.0f };
        std::atomic<float> fadeOutMs { 10.0f };
        std::atomic<int> command { 0 }; // 1=trigger, 2=stop, 3=immediate stop
        std::atomic<int> playingForUi { 0 };
        std::atomic<float> progressForUi { 0.0f };

        // Audio-thread-owned playback state.
        double position = 0.0;
        bool playing = false;
        bool stopping = false;
        int stopFadeRemaining = 0;
        int stopFadeTotal = 0;
    };

    bool validIndex (int index) const { return juce::isPositiveAndBelow (index, padCount); }
    void applyRuntimeSettings (int index, const AudioPadState& state);
    void startPlayback (PadRuntime& p);
    void beginStop (PadRuntime& p, bool immediate);
    void renderPad (PadRuntime& p, juce::AudioBuffer<float>& target, int numSamples);

    mutable juce::CriticalSection stateLock;
    std::array<AudioPadState, padCount> states;
    std::array<PadRuntime, padCount> pads;
    juce::AudioFormatManager formats;
    std::atomic<float> masterVolume { 1.0f };
    std::atomic<double> engineSampleRate { 48000.0 };
};
