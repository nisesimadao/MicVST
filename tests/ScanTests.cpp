#include <juce_audio_processors/juce_audio_processors.h>
#include "state/PluginScanCache.h"

// Hilfsfunktion: eine plausible PluginDescription bauen (kein echtes Plugin nötig).
static juce::PluginDescription makeDesc (const juce::String& name, const juce::String& file)
{
    juce::PluginDescription d;
    d.name = name; d.pluginFormatName = "VST3"; d.fileOrIdentifier = file;
    d.uniqueId = 0x1234; d.category = "Fx"; d.manufacturerName = "Test";
    return d;
}

struct PluginScanCacheTest : juce::UnitTest
{
    PluginScanCacheTest() : juce::UnitTest ("PluginScanCache") {}
    void runTest() override
    {
        beginTest ("round-trip preserves types and skip list");
        {
            juce::KnownPluginList list;
            list.addType (makeDesc ("Pro-Q 3", "C:/VST3/Pro-Q 3.vst3"));
            juce::Array<SkippedPlugin> skips;
            skips.add ({ "C:/VST3/Broken.vst3", "unresponsive", 1234567 });

            auto xml = PluginScanCache::toXml (list, skips);
            expect (xml != nullptr);

            juce::KnownPluginList list2;
            juce::Array<SkippedPlugin> skips2;
            expect (PluginScanCache::fromXml (*xml, list2, skips2));
            expectEquals (list2.getNumTypes(), 1);
            expectEquals (list2.getTypes()[0].name, juce::String ("Pro-Q 3"));
            expectEquals (skips2.size(), 1);
            expectEquals (skips2[0].file, juce::String ("C:/VST3/Broken.vst3"));
            expectEquals (skips2[0].reason, juce::String ("unresponsive"));
            expect (skips2[0].fileTimeMs == 1234567);
        }

        beginTest ("foreign xml is rejected");
        {
            juce::XmlElement wrong ("SomethingElse");
            juce::KnownPluginList list; juce::Array<SkippedPlugin> skips;
            expect (! PluginScanCache::fromXml (wrong, list, skips));
            expectEquals (list.getNumTypes(), 0);
        }

        beginTest ("load deletes corrupt file and returns false");
        {
            auto f = juce::File::getSpecialLocation (juce::File::tempDirectory)
                        .getNonexistentChildFile ("micvst_cache_test", ".xml");
            f.replaceWithText ("this is not xml");
            juce::KnownPluginList list; juce::Array<SkippedPlugin> skips;
            expect (! PluginScanCache::load (f, list, skips));
            expect (! f.existsAsFile());
        }
    }
};
static PluginScanCacheTest pluginScanCacheTest;
