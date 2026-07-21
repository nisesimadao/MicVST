#include <juce_audio_processors/juce_audio_processors.h>
#include <map>
#include "state/PluginScanCache.h"
#include "audio/ScanCoordinator.h"

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

// Fake-Runner: liefert pro Datei ein vorkonfiguriertes Ergebnis.
struct FakeRunner : ScanProcessRunner
{
    std::map<juce::String, ScanProcessResult> results;
    juce::StringArray calls;
    ScanProcessResult run (const juce::String& path, int, std::function<bool()>) override
    {
        calls.add (path);
        auto it = results.find (path);
        return it != results.end() ? it->second : ScanProcessResult{};
    }
};

static juce::String scanResultXmlFor (const juce::PluginDescription& d)
{
    juce::XmlElement root ("MicVSTScanResult");
    root.addChildElement (d.createXml().release());
    return root.toString();
}

struct RunScanTest : juce::UnitTest
{
    RunScanTest() : juce::UnitTest ("runScan") {}
    void runTest() override
    {
        beginTest ("ok/timeout/failed are sorted into found and skipped");
        {
            FakeRunner r;
            r.results["C:/v/Good.vst3"] = { ScanProcessResult::Status::ok,
                                            scanResultXmlFor (makeDesc ("Good", "C:/v/Good.vst3")) };
            r.results["C:/v/Hang.vst3"] = { ScanProcessResult::Status::timeout, {} };
            r.results["C:/v/Bad.vst3"]  = { ScanProcessResult::Status::failed, {} };

            juce::StringArray progressNames; int lastTotal = 0;
            auto out = runScan ({ "C:/v/Good.vst3", "C:/v/Hang.vst3", "C:/v/Bad.vst3" }, r, 1000,
                                [&] (int, int total, const juce::String& n) { progressNames.add (n); lastTotal = total; },
                                [] { return false; });

            expect (out.completed);
            expectEquals (out.found.size(), 1);
            expectEquals (out.found[0].name, juce::String ("Good"));
            expectEquals (out.skipped.size(), 2);
            expectEquals (out.skipped[0].reason, juce::String ("unresponsive"));
            expectEquals (out.skipped[1].reason, juce::String ("failed"));
            expectEquals (progressNames.size(), 3);
            expectEquals (lastTotal, 3);
        }

        beginTest ("garbage xml on ok-status counts as failed");
        {
            FakeRunner r;
            r.results["C:/v/Garbage.vst3"] = { ScanProcessResult::Status::ok, "not xml" };
            auto out = runScan ({ "C:/v/Garbage.vst3" }, r, 1000, {}, [] { return false; });
            expectEquals (out.found.size(), 0);
            expectEquals (out.skipped.size(), 1);
            expectEquals (out.skipped[0].reason, juce::String ("failed"));
        }

        beginTest ("shouldExit aborts without blacklisting");
        {
            FakeRunner r;
            auto out = runScan ({ "C:/v/A.vst3", "C:/v/B.vst3" }, r, 1000, {},
                                [] { return true; });   // sofort abbrechen
            expect (! out.completed);
            expectEquals ((int) r.calls.size(), 0);
            expectEquals (out.skipped.size(), 0);
        }
    }
};
static RunScanTest runScanTest;

struct ParseScanResultTest : juce::UnitTest
{
    ParseScanResultTest() : juce::UnitTest ("parseScanResultXml") {}
    void runTest() override
    {
        beginTest ("valid result xml yields descriptions");
        {
            juce::Array<juce::PluginDescription> out;
            expect (parseScanResultXml (scanResultXmlFor (makeDesc ("X", "C:/v/X.vst3")), out));
            expectEquals (out.size(), 1);
            expectEquals (out[0].name, juce::String ("X"));
        }
        beginTest ("garbage and wrong root are rejected");
        {
            juce::Array<juce::PluginDescription> out;
            expect (! parseScanResultXml ("nope", out));
            expect (! parseScanResultXml ("<Wrong/>", out));
            expectEquals (out.size(), 0);
        }
    }
};
static ParseScanResultTest parseScanResultTest;
