#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Filtert + sortiert die Plugin-Liste für den Picker. Leere/Whitespace-Query = alle;
// sonst case-insensitives Substring-Match auf Name ODER Hersteller.
// Sortierung: Hersteller A-Z, darin Name A-Z (stabil für die Gruppierung im Picker).
juce::Array<juce::PluginDescription> filterPlugins (const juce::Array<juce::PluginDescription>& all,
                                                    const juce::String& query);
