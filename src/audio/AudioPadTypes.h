#pragma once
#include <juce_core/juce_core.h>

enum class AudioPadRoute
{
    postFx = 0,
    preFx = 1,
    output2Only = 2
};

enum class AudioPadRetrigger
{
    restart = 0,
    stop = 1,
    ignore = 2
};

struct AudioPadState
{
    juce::String name;
    juce::String filePath;
    float volume = 0.85f;
    bool loop = false;
    AudioPadRoute route = AudioPadRoute::postFx;
    AudioPadRetrigger retrigger = AudioPadRetrigger::restart;
    juce::String hotkey;
    float fadeInMs = 0.0f;
    float fadeOutMs = 10.0f;
    juce::uint32 colourARGB = 0;
};
