#pragma once
#include <juce_data_structures/juce_data_structures.h>
#include <juce_core/juce_core.h>
#include "audio/AudioPadTypes.h"

struct PluginEntryState
{
    juce::String fileOrId;
    bool bypassed = false;
    juce::MemoryBlock state;   // getStateInformation()-Blob
};

struct MicVSTState
{
    juce::String inputDevice, outputDevice;
    juce::String output2Device; // optional monitoring output; empty = Off
    double sampleRate = 48000.0;
    int    bufferSize = 0;   // Buffer-Wunsch in Samples; 0 = Auto (Geräte-Default)
    juce::Array<PluginEntryState> plugins;
    juce::StringArray pluginFolders;   // zusätzliche VST3-Suchordner

    juce::Array<AudioPadState> audioPads;
    float audioPadMasterVolume = 1.0f;

    juce::String windowState;          // DocumentWindow::getWindowStateAsString() (Größe/Position)

    // Opt-in Auto-Update-Check (siehe UpdateChecker). Default: aus, nie gefragt.
    bool updateCheckEnabled = false;   // Checkbox-Zustand
    bool updateCheckAsked   = false;   // Erststart-Popup schon gezeigt?
    juce::String lastNotifiedVersion;  // letzte per Tray-Bubble gemeldete Version (Dedup)
};

juce::ValueTree  toValueTree (const MicVSTState&);
MicVSTState    fromValueTree (const juce::ValueTree&);

// Datei unter %APPDATA%\MicVST\config.xml
juce::File    configFile();
bool          saveState (const MicVSTState&);
MicVSTState loadState();
