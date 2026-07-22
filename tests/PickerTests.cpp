#include <juce_audio_processors/juce_audio_processors.h>
#include "ui/PluginSearch.h"

static juce::PluginDescription desc (const juce::String& name, const juce::String& manu)
{
    juce::PluginDescription d;
    d.name = name; d.manufacturerName = manu; d.pluginFormatName = "VST3";
    d.fileOrIdentifier = "C:/v/" + name + ".vst3";
    return d;
}

struct FilterPluginsTest : juce::UnitTest
{
    FilterPluginsTest() : juce::UnitTest ("filterPlugins") {}
    void runTest() override
    {
        juce::Array<juce::PluginDescription> all;
        all.add (desc ("Pro-Q 3", "FabFilter"));
        all.add (desc ("LoudMax", "Thomas Mundt"));
        all.add (desc ("Pro-C 2", "FabFilter"));
        all.add (desc ("API Vision", "UAD"));

        beginTest ("empty query returns all, sorted by manufacturer then name");
        {
            auto out = filterPlugins (all, "");
            expectEquals (out.size(), 4);
            expectEquals (out[0].name, juce::String ("Pro-C 2"));    // FabFilter zuerst (A-Z)
            expectEquals (out[1].name, juce::String ("Pro-Q 3"));
            expectEquals (out[2].name, juce::String ("LoudMax"));    // Thomas Mundt
            expectEquals (out[3].name, juce::String ("API Vision")); // UAD
        }
        beginTest ("matches name and manufacturer, case-insensitive");
        {
            expectEquals (filterPlugins (all, "pro-q").size(), 1);
            expectEquals (filterPlugins (all, "FABFILTER").size(), 2);
            expectEquals (filterPlugins (all, "uad").size(), 1);
        }
        beginTest ("no hits and whitespace-only query");
        {
            expectEquals (filterPlugins (all, "zzz").size(), 0);
            expectEquals (filterPlugins (all, "   ").size(), 4);   // wie leer
        }
    }
};
static FilterPluginsTest filterPluginsTest;
