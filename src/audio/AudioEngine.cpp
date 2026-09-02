#include "audio/AudioEngine.h"
using IOProc = juce::AudioProcessorGraph::AudioGraphIOProcessor;

namespace
{
    constexpr const char* kVBCableRenderEndpoint = "CABLE Input";
}

AudioEngine::AudioEngine()
{
    deviceManager.getAvailableDeviceTypes();
    deviceManager.addChangeListener (this);

    secondaryOutput.onChanged = [this]
    {
        if (onStatusChanged) onStatusChanged();
        if (onDeviceChanged) onDeviceChanged();
    };
}

AudioEngine::~AudioEngine()
{
    audioPads.stopAll (true);
    secondaryOutput.onChanged = nullptr;
    secondaryOutput.setDevice ({});
    deviceManager.removeChangeListener (this);
    deviceManager.removeAudioCallback (this);
    deviceManager.closeAudioDevice();
    player.setProcessor (nullptr);
}

bool AudioEngine::isRunning() const
{
    auto* dev = deviceManager.getCurrentAudioDevice();
    return dev != nullptr && dev->isPlaying();
}

void AudioEngine::changeListenerCallback (juce::ChangeBroadcaster*)
{
    juce::Logger::writeToLog (isRunning() ? "Audio: läuft"
                                          : "Audio: idle (Device getrennt?)");
    if (onStatusChanged) onStatusChanged();
    if (onDeviceChanged) onDeviceChanged();
}

juce::String AudioEngine::detectCableOutput()
{
    deviceManager.setCurrentAudioDeviceType (deviceManager.preferredTypeName(), true);
    if (auto* type = deviceManager.getCurrentDeviceTypeObject())
    {
        type->scanForDevices();
        const auto outs = type->getDeviceNames (false);
        for (auto& name : outs)
            if (name.containsIgnoreCase (kVBCableRenderEndpoint))
                return name;
    }
    return {};
}

juce::String AudioEngine::initialise (const juce::String& inputDeviceName,
                                      const juce::String& outputDeviceName)
{
    deviceManager.removeAudioCallback (this);
    deviceManager.closeAudioDevice();
    deviceManager.setCurrentAudioDeviceType (deviceManager.preferredTypeName(), true);

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup (setup);
    setup.inputDeviceName  = inputDeviceName;
    setup.outputDeviceName = outputDeviceName;
    if (preferredBufferSize > 0)
        setup.bufferSize = preferredBufferSize;
    setup.useDefaultInputChannels  = false;
    setup.useDefaultOutputChannels = false;
    setup.inputChannels.clear();
    setup.inputChannels.setRange (0, 2, true);
    setup.outputChannels.clear();
    setup.outputChannels.setRange (0, 2, true);
    setup.sampleRate = 48000.0;

    const juce::String err = deviceManager.setAudioDeviceSetup (setup, true);

    if (auto* d = deviceManager.getCurrentAudioDevice())
        juce::Logger::writeToLog ("Setup: in=" + setup.inputDeviceName
            + " out=" + setup.outputDeviceName
            + " sr=" + juce::String (d->getCurrentSampleRate(), 0)
            + " buf=" + juce::String (d->getCurrentBufferSizeSamples()));
    else
        juce::Logger::writeToLog ("Setup: kein Device offen (" + err + ")");

    graph.clear();
    auto inNode  = graph.addNode (std::make_unique<IOProc> (IOProc::audioInputNode));
    auto outNode = graph.addNode (std::make_unique<IOProc> (IOProc::audioOutputNode));

    pluginChain = std::make_unique<PluginChain> (graph, inNode->nodeID, outNode->nodeID);
    rebuildGraph();

    if (auto* d = deviceManager.getCurrentAudioDevice())
        playHead.sampleRate.store (d->getCurrentSampleRate());
    graph.setPlayHead (&playHead);

    player.setProcessor (&graph);
    deviceManager.addAudioCallback (this);
    return err;
}

void AudioEngine::setDeviceConfig (const juce::String& input, const juce::String& output,
                                  double sampleRate, int bufferSize)
{
    (void) output;

    auto setup = deviceManager.getAudioDeviceSetup();
    setup.inputDeviceName  = input;
    setup.outputDeviceName = detectCableOutput();
    if (sampleRate > 0.0) setup.sampleRate = sampleRate;
    if (bufferSize > 0)   setup.bufferSize = bufferSize;
    setup.useDefaultInputChannels  = false;
    setup.useDefaultOutputChannels = false;
    setup.inputChannels.clear();  setup.inputChannels.setRange (0, 2, true);
    setup.outputChannels.clear(); setup.outputChannels.setRange (0, 2, true);

    deviceManager.setAudioDeviceSetup (setup, true);
    rebuildGraph();
}

void AudioEngine::rebuildGraph()
{
    if (pluginChain != nullptr)
        pluginChain->rebuildConnections();
}

juce::File AudioEngine::pluginCacheFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
              .getChildFile ("MicVST").getChildFile ("plugin_cache.xml");
}

juce::StringArray AudioEngine::scanRoots() const
{
    MicVST3Format vst3;
    juce::StringArray roots;
    const auto defaults = vst3.getDefaultLocationsToSearch();
    for (int i = 0; i < defaults.getNumPaths(); ++i)
        roots.add (defaults[i].getFullPathName());
    for (auto& f : pluginFolders)
        if (f.isNotEmpty()) roots.add (f);
    return roots;
}

juce::StringArray AudioEngine::listVst3Files() const
{
    MicVST3Format vst3;
    juce::FileSearchPath paths;
    for (auto& r : scanRoots())
        paths.add (juce::File (r));
    paths.removeRedundantPaths();
    return vst3.searchPathsForPlugins (paths, true, true);
}

void AudioEngine::loadPluginCache()
{
    if (! PluginScanCache::load (pluginCacheFile(), knownPlugins, skippedPlugins))
        juce::Logger::writeToLog ("Plugin-Cache fehlt/korrupt -> voller Scan");
}

void AudioEngine::startBackgroundScan (int timeoutMs, const juce::StringArray& forceRescan)
{
    if (isScanning()) { rescanQueued = true; return; }

    MicVST3Format vst3;
    auto files = filterFilesNeedingScan (listVst3Files(), knownPlugins, vst3, skippedPlugins, forceRescan);
    juce::Logger::writeToLog ("Scan: " + juce::String (files.size()) + " Datei(en) zu scannen");
    if (files.isEmpty())
    {
        PluginScanCache::save (pluginCacheFile(), knownPlugins, skippedPlugins);
        if (! pendingPlugins.isEmpty()) { restoreChain (pendingPlugins); pendingPlugins.clear(); }
        if (onScanFinished) onScanFinished();
        return;
    }

    scanner = std::make_unique<ScanCoordinator> (files,
        [this] (int cur, int total, juce::String name)
        {
            if (onScanProgress) onScanProgress (cur, total, name);
        },
        [this] (ScanOutcome outcome) { handleScanFinished (outcome); },
        timeoutMs);
}

void AudioEngine::handleScanFinished (const ScanOutcome& outcome)
{
    scanner = nullptr;

    mergeScanResults (knownPlugins, outcome);
    for (auto& s : outcome.skipped)
    {
        skippedPlugins.add (s);
        juce::Logger::writeToLog ("Scan übersprungen (" + s.reason + "): " + s.file);
    }

    pruneOutsideFolders();
    PluginScanCache::save (pluginCacheFile(), knownPlugins, skippedPlugins);

    if (! pendingPlugins.isEmpty())
    {
        restoreChain (pendingPlugins);
        pendingPlugins.clear();
    }
    if (onScanFinished) onScanFinished();

    if (rescanQueued) { rescanQueued = false; startBackgroundScan(); }
}

void AudioEngine::rescanAllPlugins()
{
    if (isScanning()) return;
    knownPlugins.clear();
    skippedPlugins.clear();
    pluginCacheFile().deleteFile();
    startBackgroundScan();
}

void AudioEngine::retrySkippedPlugins()
{
    if (isScanning() || skippedPlugins.isEmpty()) return;

    juce::StringArray forceRescan;
    for (auto& s : skippedPlugins)
        if (juce::File (s.file).exists())
            forceRescan.add (s.file);

    skippedPlugins.clear();
    startBackgroundScan (ScanCoordinator::retryTimeoutMs, forceRescan);
}

void AudioEngine::skipCurrentScanFile()
{
    if (scanner != nullptr) scanner->skipCurrentFile();
}

void AudioEngine::pruneOutsideFolders()
{
    const auto roots = scanRoots();
    for (auto& t : knownPlugins.getTypes())
        if (! pathIsInsideAnyFolder (t.fileOrIdentifier, roots)) knownPlugins.removeType (t);
    for (int i = skippedPlugins.size(); --i >= 0;)
        if (! pathIsInsideAnyFolder (skippedPlugins[i].file, roots)) skippedPlugins.remove (i);
}

void AudioEngine::addPluginFolder (const juce::String& folder)
{
    if (folder.isNotEmpty() && ! pluginFolders.contains (folder))
        pluginFolders.add (folder);
    startBackgroundScan();
}

void AudioEngine::removePluginFolder (const juce::String& folder)
{
    pluginFolders.removeString (folder);
    pruneOutsideFolders();
    PluginScanCache::save (pluginCacheFile(), knownPlugins, skippedPlugins);
    startBackgroundScan();
}

MicVSTState AudioEngine::captureState()
{
    MicVSTState s;
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup (setup);
    s.inputDevice  = setup.inputDeviceName;
    s.outputDevice = setup.outputDeviceName;
    s.output2Device = secondaryOutput.desiredDevice();
    s.sampleRate   = setup.sampleRate;
    s.bufferSize   = preferredBufferSize;
    s.pluginFolders = pluginFolders;
    s.audioPads = audioPads.captureStates();
    s.audioPadMasterVolume = audioPads.getMasterVolume();

    if (! pendingPlugins.isEmpty()) { s.plugins = pendingPlugins; return s; }
    if (pluginChain == nullptr) return s;

    for (auto& e : pluginChain->entries())
    {
        PluginEntryState p;
        p.fileOrId = e.fileOrId;
        p.bypassed = e.bypassed;
        if (auto* node = graph.getNodeForId (e.node))
        {
            auto* proc = node->getProcessor();
            const juce::ScopedLock sl (proc->getCallbackLock());
            proc->getStateInformation (p.state);
        }
        s.plugins.add (p);
    }
    return s;
}

void AudioEngine::applyState (const MicVSTState& s)
{
    setPreferredBufferSize (s.bufferSize);

    const auto cableOutput = detectCableOutput();
    juce::Logger::writeToLog (cableOutput.isNotEmpty()
        ? "VB-CABLE host output = " + cableOutput
        : "VB-CABLE not found -> output 'none'");
    initialise (s.inputDevice, cableOutput);

    audioPads.setMasterVolume (s.audioPadMasterVolume);
    audioPads.restoreStates (s.audioPads);

    if (const auto output2Error = setOutput2Device (s.output2Device); output2Error.isNotEmpty())
        juce::Logger::writeToLog (output2Error);

    bool allResolvable = true;
    for (auto& p : s.plugins)
        if (! p.fileOrId.startsWith ("builtin:") && knownPlugins.getTypeForFile (p.fileOrId) == nullptr)
            { allResolvable = false; break; }

    if (allResolvable) restoreChain (s.plugins);
    else               { pendingPlugins = s.plugins;
                         juce::Logger::writeToLog ("Ketten-Restore wartet auf Plugin-Scan"); }
    rebuildGraph();
}

void AudioEngine::restoreChain (const juce::Array<PluginEntryState>& plugins)
{
    double sr = deviceManager.getCurrentAudioDevice() != nullptr
              ? deviceManager.getCurrentAudioDevice()->getCurrentSampleRate() : 48000.0;

    for (auto& p : plugins)
    {
        if (p.fileOrId.startsWith ("builtin:"))
        {
            bool added = false;
            if (p.fileOrId == PluginChain::monoToStereoId)
            {
                pluginChain->addMonoToStereo();
                added = true;
            }
            else if (p.fileOrId == PluginChain::stereoToMonoId)
            {
                pluginChain->addStereoToMono();
                added = true;
            }
            else
            {
                added = pluginChain->addBuiltIn (p.fileOrId);
            }

            if (added)
            {
                const int idx = (int) pluginChain->entries().size() - 1;
                if (auto* node = graph.getNodeForId (pluginChain->entries()[(size_t) idx].node))
                    node->getProcessor()->setStateInformation (p.state.getData(), (int) p.state.getSize());
                pluginChain->setBypass (idx, p.bypassed);
            }
            continue;
        }

        auto type = knownPlugins.getTypeForFile (p.fileOrId);
        if (type == nullptr) { juce::Logger::writeToLog ("Plugin fehlt: " + p.fileOrId); continue; }
        juce::String err;
        if (pluginChain->addPlugin (formatManager, *type, sr, 128, err))
        {
            const int idx = (int) pluginChain->entries().size() - 1;
            if (auto* node = graph.getNodeForId (pluginChain->entries()[(size_t) idx].node))
                node->getProcessor()->setStateInformation (p.state.getData(), (int) p.state.getSize());
            pluginChain->setBypass (idx, p.bypassed);
        }
        else juce::Logger::writeToLog ("Plugin-Load: " + err);
    }
    rebuildGraph();
}

void AudioEngine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    juce::Logger::writeToLog ("Device start: '" + device->getName() + "'"
        + " | inCh aktiv=" + juce::String (device->getActiveInputChannels().countNumberOfSetBits())
        + " von [" + device->getInputChannelNames().joinIntoString (", ") + "]"
        + " | outCh aktiv=" + juce::String (device->getActiveOutputChannels().countNumberOfSetBits())
        + " | sr=" + juce::String (device->getCurrentSampleRate(), 0)
        + " | buf=" + juce::String (device->getCurrentBufferSizeSamples()));

    const int block = juce::jmax (1, device->getCurrentBufferSizeSamples());
    padScratchCapacity = juce::jmax (8192, block * 4);
    for (auto* b : { &padPreFx, &padPostFx, &padOutput2Only, &mixedInput, &output2Mix })
        b->setSize (2, padScratchCapacity, false, true, false);

    audioPads.prepare (device->getCurrentSampleRate(), padScratchCapacity);
    secondaryOutput.setSourceSampleRate (device->getCurrentSampleRate());
    player.audioDeviceAboutToStart (device);
}

void AudioEngine::audioDeviceStopped()
{
    audioPads.stopAll (true);
    player.audioDeviceStopped();
}

void AudioEngine::audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                                    int numInputChannels,
                                                    float* const* outputChannelData,
                                                    int numOutputChannels,
                                                    int numSamples,
                                                    const juce::AudioIODeviceCallbackContext& context)
{
    playHead.samples.fetch_add (numSamples, std::memory_order_relaxed);

    if (numInputChannels > 0)
    {
        juce::AudioBuffer<float> inView (const_cast<float* const*> (inputChannelData),
                                         numInputChannels, numSamples);
        inputMeter.process (inView);
    }

    if (numSamples > padScratchCapacity || padScratchCapacity <= 0)
    {
        // Should never happen with WASAPI's fixed callback size; keep the primary path alive
        // rather than allocating on the realtime thread.
        player.audioDeviceIOCallbackWithContext (inputChannelData, numInputChannels,
                                                 outputChannelData, numOutputChannels,
                                                 numSamples, context);
        if (numOutputChannels > 0)
        {
            secondaryOutput.push (const_cast<const float* const*> (outputChannelData),
                                  numOutputChannels, numSamples);
            juce::AudioBuffer<float> outView (outputChannelData, numOutputChannels, numSamples);
            outputMeter.process (outView);
        }
        return;
    }

    audioPads.render (padPreFx, padPostFx, padOutput2Only, numSamples);

    // Pre-FX pads are mixed into the graph input, so they pass through the same VST/DSP chain.
    const int playerInputs = numInputChannels > 0 ? juce::jmin (2, numInputChannels) : 2;
    mixedInput.clear (0, numSamples);
    for (int ch = 0; ch < playerInputs; ++ch)
    {
        if (numInputChannels > 0 && inputChannelData[ch] != nullptr)
            mixedInput.copyFrom (ch, 0, inputChannelData[ch], numSamples);

        if (playerInputs == 1)
        {
            mixedInput.addFrom (0, 0, padPreFx, 0, 0, numSamples, 0.5f);
            mixedInput.addFrom (0, 0, padPreFx, 1, 0, numSamples, 0.5f);
        }
        else
            mixedInput.addFrom (ch, 0, padPreFx, juce::jmin (ch, 1), 0, numSamples);
    }

    const float* mixedPtrs[2] = { mixedInput.getReadPointer (0), mixedInput.getReadPointer (1) };
    player.audioDeviceIOCallbackWithContext (mixedPtrs, playerInputs,
                                             outputChannelData, numOutputChannels,
                                             numSamples, context);

    // Post-FX pads bypass the DSP chain and are mixed directly into the virtual-mic bus.
    if (numOutputChannels == 1 && outputChannelData[0] != nullptr)
    {
        juce::FloatVectorOperations::addWithMultiply (outputChannelData[0], padPostFx.getReadPointer (0), 0.5f, numSamples);
        juce::FloatVectorOperations::addWithMultiply (outputChannelData[0], padPostFx.getReadPointer (1), 0.5f, numSamples);
    }
    else
    {
        for (int ch = 0; ch < juce::jmin (2, numOutputChannels); ++ch)
            if (outputChannelData[ch] != nullptr)
                juce::FloatVectorOperations::add (outputChannelData[ch], padPostFx.getReadPointer (ch), numSamples);
    }

    // Output2 receives the primary processed mix plus its private pad bus. Output2-only pads
    // never reach CABLE Input, so they can be used for local cues/metronomes/preview sounds.
    if (secondaryOutput.isRunning())
    {
        output2Mix.clear (0, numSamples);
        if (numOutputChannels == 1 && outputChannelData[0] != nullptr)
        {
            output2Mix.copyFrom (0, 0, outputChannelData[0], numSamples);
            output2Mix.copyFrom (1, 0, outputChannelData[0], numSamples);
        }
        else if (numOutputChannels > 1)
        {
            if (outputChannelData[0] != nullptr) output2Mix.copyFrom (0, 0, outputChannelData[0], numSamples);
            if (outputChannelData[1] != nullptr) output2Mix.copyFrom (1, 0, outputChannelData[1], numSamples);
        }
        output2Mix.addFrom (0, 0, padOutput2Only, 0, 0, numSamples);
        output2Mix.addFrom (1, 0, padOutput2Only, 1, 0, numSamples);
        const float* monitorPtrs[2] = { output2Mix.getReadPointer (0), output2Mix.getReadPointer (1) };
        secondaryOutput.push (monitorPtrs, 2, numSamples);
    }

    if (numOutputChannels > 0)
    {
        juce::AudioBuffer<float> outView (outputChannelData, numOutputChannels, numSamples);
        outputMeter.process (outView);
    }
}
