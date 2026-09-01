#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>

namespace BuiltinEffects
{
    static constexpr const char* autoTuneId   = "builtin:autotune";
    static constexpr const char* robotId      = "builtin:robot";
    static constexpr const char* radioId      = "builtin:radio";
    static constexpr const char* bitcrusherId = "builtin:bitcrusher";
    static constexpr const char* pitchId      = "builtin:pitch";
    static constexpr const char* deepVoiceId  = "builtin:deepvoice";

    bool isEffectId (const juce::String& id);
    juce::String displayName (const juce::String& id);
    juce::Array<juce::PluginDescription> descriptions();
    std::unique_ptr<juce::AudioProcessor> create (const juce::String& id);
}
