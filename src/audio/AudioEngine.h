#pragma once
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "audio/Metering.h"
#include "audio/PluginChain.h"
#include "audio/MicVSTDeviceManager.h"
#include "audio/SecondaryOutput.h"
#include "audio/AudioPadEngine.h"
#include "audio/ScanCoordinator.h"
#include "state/Persistence.h"

class AudioEngine : private juce::AudioIODeviceCallback,
                    private juce::ChangeListener
{
public:
    AudioEngine();
    ~AudioEngine() override;

    juce::String initialise (const juce::String& inputDeviceName,
                             const juce::String& outputDeviceName);

    void setDeviceConfig (const juce::String& input, const juce::String& output,
                          double sampleRate, int bufferSize);

    void setPreferredBufferSize (int samples) { preferredBufferSize = samples; }
    int  getPreferredBufferSize() const       { return preferredBufferSize; }

    juce::String detectCableOutput();

    juce::StringArray getOutput2DeviceNames() { return secondaryOutput.deviceNames(); }
    juce::String setOutput2Device (const juce::String& name) { return secondaryOutput.setDevice (name); }
    juce::String getOutput2Device() const { return secondaryOutput.desiredDevice(); }
    bool isOutput2Running() const { return secondaryOutput.isRunning(); }

    AudioPadEngine& getAudioPads() { return audioPads; }
    const AudioPadEngine& getAudioPads() const { return audioPads; }

    MicVSTState captureState();
    void applyState (const MicVSTState&);

    bool isRunning() const;
    std::function<void()> onStatusChanged;
    std::function<void()> onDeviceChanged;

    std::function<void()> onStateChanged;
    void requestPersist() { if (onStateChanged) onStateChanged(); }

    std::function<void()> onFactoryResetRequested;
    void requestFactoryReset() { if (onFactoryResetRequested) onFactoryResetRequested(); }

    MicVSTDeviceManager&            getDeviceManager() { return deviceManager; }
    juce::AudioProcessorGraph&      getGraph()         { return graph; }
    PluginChain&                    getChain()         { return *pluginChain; }
    juce::AudioPluginFormatManager& getFormatManager() { return formatManager; }
    juce::KnownPluginList&          getKnownPlugins()  { return knownPlugins; }

    void loadPluginCache();
    void startBackgroundScan (int timeoutMs = ScanCoordinator::defaultTimeoutMs,
                              const juce::StringArray& forceRescan = {});
    void rescanAllPlugins();
    void retrySkippedPlugins();
    void skipCurrentScanFile();
    bool isScanning() const { return scanner != nullptr; }
    const juce::Array<SkippedPlugin>& getSkippedPlugins() const { return skippedPlugins; }
    std::function<void (int, int, juce::String)> onScanProgress;
    std::function<void()> onScanFinished;

    void addPluginFolder (const juce::String& folder);
    void removePluginFolder (const juce::String& folder);
    void setPluginFolders (const juce::StringArray& f) { pluginFolders = f; }
    const juce::StringArray& getPluginFolders() const  { return pluginFolders; }

    static juce::File pluginCacheFile();

    LevelReading inputLevel()  const { return inputMeter.read(); }
    LevelReading outputLevel() const { return outputMeter.read(); }

    void rebuildGraph();

private:
    struct PlayingHead : juce::AudioPlayHead
    {
        std::atomic<juce::int64> samples { 0 };
        std::atomic<double> sampleRate { 48000.0 };
        juce::Optional<PositionInfo> getPosition() const override
        {
            const auto s  = samples.load (std::memory_order_relaxed);
            const auto sr = sampleRate.load (std::memory_order_relaxed);
            PositionInfo info;
            info.setIsPlaying (true);
            info.setIsRecording (false);
            info.setIsLooping (false);
            info.setTimeInSamples (s);
            info.setTimeInSeconds ((double) s / sr);
            info.setBpm (120.0);
            info.setTimeSignature (juce::AudioPlayHead::TimeSignature{});
            info.setPpqPosition (((double) s / sr) * (120.0 / 60.0));
            return info;
        }
    };
    PlayingHead playHead;

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart (juce::AudioIODevice*) override;
    void audioDeviceStopped() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    juce::StringArray scanRoots() const;
    juce::StringArray listVst3Files() const;
    void handleScanFinished (const ScanOutcome&);
    void pruneOutsideFolders();
    void restoreChain (const juce::Array<PluginEntryState>&);

    MicVSTDeviceManager deviceManager;
    SecondaryOutput secondaryOutput;
    AudioPadEngine audioPads;
    juce::AudioProcessorGraph graph;
    juce::AudioProcessorPlayer player;
    std::unique_ptr<PluginChain> pluginChain;
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPlugins;
    juce::StringArray pluginFolders;
    juce::Array<SkippedPlugin> skippedPlugins;
    std::unique_ptr<ScanCoordinator> scanner;
    juce::Array<PluginEntryState> pendingPlugins;
    bool rescanQueued = false;
    int preferredBufferSize = 0;

    // Fixed-size scratch buffers prepared when the primary device starts. Audio Pads use
    // these buses without allocating on the realtime callback.
    juce::AudioBuffer<float> padPreFx, padPostFx, padOutput2Only;
    juce::AudioBuffer<float> mixedInput, output2Mix;
    int padScratchCapacity = 0;

    LevelMeter inputMeter, outputMeter;
};
