#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include "audio/MicVSTDeviceManager.h"
#include "audio/MonitorBuffer.h"

// Optional second playback device for monitoring MicVST's already-processed signal.
// It is deliberately independent from the primary AudioDeviceManager, whose output is
// reserved for VB-CABLE. Output2 may therefore be headphones/speakers/etc. without
// disturbing the virtual microphone path.
class SecondaryOutput : private juce::AudioIODeviceCallback,
                        private juce::ChangeListener
{
public:
    SecondaryOutput();
    ~SecondaryOutput() override;

    juce::StringArray deviceNames();
    juce::String setDevice (const juce::String& name); // empty = Off; returns JUCE error string.
    const juce::String& desiredDevice() const { return desiredName; }
    bool isRunning() const;

    void setSourceSampleRate (double rate) { buffer.setSourceSampleRate (rate); }
    void push (const float* const* channels, int numChannels, int numSamples)
    {
        buffer.push (channels, numChannels, numSamples);
    }

    std::function<void()> onChanged;

private:
    void audioDeviceIOCallbackWithContext (const float* const*, int,
                                           float* const* outputChannelData,
                                           int numOutputChannels, int numSamples,
                                           const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart (juce::AudioIODevice*) override;
    void audioDeviceStopped() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    void close();

    MicVSTDeviceManager deviceManager;
    MonitorBuffer buffer;
    juce::String desiredName;
};
