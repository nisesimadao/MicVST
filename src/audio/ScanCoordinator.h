#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>
#include "state/PluginScanCache.h"

// Ergebnis eines Kindprozess-Laufs für EINE .vst3-Datei.
struct ScanProcessResult
{
    enum class Status { ok, failed, timeout, aborted };
    Status status = Status::failed;
    juce::String resultXmlText;   // Inhalt der --out-Datei (nur bei ok)
};

// Injizierbar für Tests. Die echte Implementierung startet "MicVST.exe --scan".
struct ScanProcessRunner
{
    virtual ~ScanProcessRunner() = default;
    virtual ScanProcessResult run (const juce::String& pluginPath, int timeoutMs,
                                   std::function<bool()> shouldAbort) = 0;
};

struct ScanOutcome
{
    juce::Array<juce::PluginDescription> found;
    juce::Array<SkippedPlugin> skipped;
    bool completed = false;   // false = per shouldExit abgebrochen (nichts blacklisten!)
};

// Synchroner, testbarer Scan-Kern: Dateien der Reihe nach durch den Runner schicken.
// Timeout -> Skip "unresponsive"; Crash/kaputtes XML -> Skip "failed"; Abbruch -> nichts.
ScanOutcome runScan (const juce::StringArray& files, ScanProcessRunner& runner, int timeoutMs,
                     std::function<void (int, int, const juce::String&)> progress,
                     std::function<bool()> shouldExit);

// Parst die vom Kindmodus geschriebene Ergebnisdatei (<MicVSTScanResult> mit PLUGIN-Kindern).
bool parseScanResultXml (const juce::String& xmlText, juce::Array<juce::PluginDescription>& out);

// Welche Dateien müssen wirklich gescannt werden? Filtert Cache-Treffer und Skips heraus;
// Skip-Einträge, deren Datei sich geändert hat (Update), werden dabei aus skipped entfernt.
juce::StringArray filterFilesNeedingScan (const juce::StringArray& allFiles,
                                          const juce::KnownPluginList& list,
                                          juce::AudioPluginFormat& format,
                                          juce::Array<SkippedPlugin>& skipped);

// Hintergrund-Thread um runScan herum; marshallt Callbacks auf den Message-Thread.
// Besitzt den echten ChildProcess-Runner (oder einen injizierten für Tests).
class ScanCoordinator : private juce::Thread
{
public:
    static constexpr int defaultTimeoutMs = 30000;

    ScanCoordinator (juce::StringArray filesToScan,
                     std::function<void (int, int, juce::String)> onProgress,
                     std::function<void (ScanOutcome)> onFinished,
                     std::unique_ptr<ScanProcessRunner> runnerOverride = nullptr);
    ~ScanCoordinator() override;   // signalisiert Abbruch, killt laufenden Kindprozess

private:
    void run() override;

    juce::StringArray files;
    std::function<void (int, int, juce::String)> progressCb;
    std::function<void (ScanOutcome)> finishedCb;
    std::unique_ptr<ScanProcessRunner> runner;

    // Lebenszeit-Wächter für bereits via callAsync eingereihte Callbacks: Destruktor
    // und die Lambdas laufen beide auf dem Message-Thread, daher ist ein simples bool
    // hinter shared_ptr racefrei; der Worker-Thread kopiert nur den shared_ptr selbst
    // (dessen Refcount threadsicher ist), fasst den bool aber nie an.
    std::shared_ptr<bool> alive = std::make_shared<bool> (true);
};
