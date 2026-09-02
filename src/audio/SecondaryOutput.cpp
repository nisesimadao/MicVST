#include "audio/SecondaryOutput.h"

namespace
{
    constexpr const char* kPrimaryCableEndpoint = "CABLE Input";
}

SecondaryOutput::SecondaryOutput()
{
    deviceManager.getAvailableDeviceTypes();
    deviceManager.addChangeListener (this);
}

SecondaryOutput::~SecondaryOutput()
{
    onChanged = nullptr;
    deviceManager.removeChangeListener (this);
    close();
}

void SecondaryOutput::close()
{
    buffer.stop();
    deviceManager.removeAudioCallback (this);
    deviceManager.closeAudioDevice();
}

bool SecondaryOutput::isRunning() const
{
    auto* dev = deviceManager.getCurrentAudioDevice();
    return dev != nullptr && dev->isPlaying();
}

juce::StringArray SecondaryOutput::deviceNames()
{
    // Do not re-select the device type just to refresh a ComboBox: doing that while
    // Output2 is running could unnecessarily tear down/re-open the monitor endpoint.
    if (deviceManager.getCurrentAudioDeviceTypeObject() == nullptr)
        deviceManager.setCurrentAudioDeviceType (deviceManager.preferredTypeName(), true);

    juce::StringArray result;
    if (auto* type = deviceManager.getCurrentAudioDeviceTypeObject())
    {
        type->scanForDevices();
        result = type->getDeviceNames (false /* output */);
    }

    // Output is already permanently routed to CABLE Input. Opening the same endpoint
    // a second time is pointless and can fail on some drivers, so keep it out of Output2.
    for (int i = result.size(); --i >= 0;)
        if (result[i].containsIgnoreCase (kPrimaryCableEndpoint))
            result.remove (i);
    return result;
}

juce::String SecondaryOutput::setDevice (const juce::String& name)
{
    const auto requested = name.trim();
    if (requested == desiredName && (requested.isEmpty() || isRunning()))
        return {};

    desiredName = requested;
    close();

    if (desiredName.isEmpty())
    {
        juce::Logger::writeToLog ("Output2: off");
        if (onChanged) onChanged();
        return {};
    }

    deviceManager.setCurrentAudioDeviceType (deviceManager.preferredTypeName(), true);
    auto* type = deviceManager.getCurrentAudioDeviceTypeObject();
    if (type == nullptr)
    {
        const juce::String err = "Output2: no Windows audio device type available";
        juce::Logger::writeToLog (err);
        if (onChanged) onChanged();
        return err;
    }

    type->scanForDevices();
    if (! type->getDeviceNames (false).contains (desiredName))
    {
        const juce::String err = "Output2 device not found: " + desiredName;
        juce::Logger::writeToLog (err);
        if (onChanged) onChanged();
        return err;
    }

    auto setup = deviceManager.getAudioDeviceSetup();
    setup.inputDeviceName = {};
    setup.outputDeviceName = desiredName;
    setup.useDefaultInputChannels = false;
    setup.inputChannels.clear();
    setup.useDefaultOutputChannels = false;
    setup.outputChannels.clear();
    setup.outputChannels.setRange (0, 2, true);
    setup.sampleRate = 0.0; // device/mix default; MonitorBuffer resamples from the primary clock.
    setup.bufferSize = 0;   // device default; Output2 has its own safety queue.

    const auto err = deviceManager.setAudioDeviceSetup (setup, true);
    if (err.isNotEmpty())
    {
        juce::Logger::writeToLog ("Output2 open failed: " + err);
        if (onChanged) onChanged();
        return err;
    }

    deviceManager.addAudioCallback (this);
    if (auto* dev = deviceManager.getCurrentAudioDevice())
        juce::Logger::writeToLog ("Output2: '" + desiredName + "'"
            + " sr=" + juce::String (dev->getCurrentSampleRate(), 0)
            + " buf=" + juce::String (dev->getCurrentBufferSizeSamples()));

    if (onChanged) onChanged();
    return {};
}

void SecondaryOutput::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    buffer.prepare (device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());
}

void SecondaryOutput::audioDeviceStopped()
{
    buffer.stop();
}

void SecondaryOutput::audioDeviceIOCallbackWithContext (const float* const*, int,
                                                        float* const* outputChannelData,
                                                        int numOutputChannels, int numSamples,
                                                        const juce::AudioIODeviceCallbackContext&)
{
    buffer.render (outputChannelData, numOutputChannels, numSamples);
}

void SecondaryOutput::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (onChanged) onChanged();
}
