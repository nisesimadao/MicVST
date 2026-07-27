#pragma once
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>
#include "state/PluginScanCache.h"

// Ergebnis eines Kindprozess-Laufs für EINE .vst3-Datei.
struct ScanProcessResult
{
    // partial = Kind crashte mitten in der Enumeration (Exit-Code 3), hat aber die bis
    // dahin gefundenen Typen als XML gerettet (WaveShell & Co.: hunderte Klassen).
    enum class Status { ok, failed, timeout, skippedByUser, aborted, partial };
    Status status = Status::failed;
    juce::String resultXmlText;   // Inhalt der --out-Datei (bei ok und partial)
    juce::uint32 exitCode = 0;    // roher Exit-Code des Kindes (Crash: NTSTATUS)
};

// Injizierbar für Tests. Die echte Implementierung startet "MicVST.exe --scan".
// shouldSkip = User will NUR die aktuelle Datei überspringen (Scan läuft weiter),
// shouldAbort = ganzer Scan soll enden (App-Shutdown).
struct ScanProcessRunner
{
    virtual ~ScanProcessRunner() = default;
    virtual ScanProcessResult run (const juce::String& pluginPath, int timeoutMs,
                                   std::function<bool()> shouldAbort,
                                   std::function<bool()> shouldSkip) = 0;
};

struct ScanOutcome
{
    juce::Array<juce::PluginDescription> found;
    juce::Array<SkippedPlugin> skipped;
    bool completed = false;   // false = per shouldExit abgebrochen (nichts blacklisten!)
};

// Synchroner, testbarer Scan-Kern: Dateien der Reihe nach durch den Runner schicken.
// Timeout -> Skip "unresponsive"; Crash/kaputtes XML -> Skip "failed"; User-Skip via
// skipRequest -> Skip "skipped" (Flag wird pro Datei zurückgesetzt); Abbruch -> nichts.
ScanOutcome runScan (const juce::StringArray& files, ScanProcessRunner& runner, int timeoutMs,
                     std::function<void (int, int, const juce::String&)> progress,
                     std::function<bool()> shouldExit,
                     std::atomic<bool>* skipRequest = nullptr);

// Parst die vom Kindmodus geschriebene Ergebnisdatei (<MicVSTScanResult> mit PLUGIN-
// Kindern). crashCode != nullptr: erhält das Root-Attribut "crashCode" (leer wenn keins).
bool parseScanResultXml (const juce::String& xmlText, juce::Array<juce::PluginDescription>& out,
                         juce::String* crashCode = nullptr);

// Welche Dateien müssen wirklich gescannt werden? Filtert Cache-Treffer und Skips heraus;
// Skip-Einträge, deren Datei sich geändert hat (Update), werden dabei aus skipped entfernt.
juce::StringArray filterFilesNeedingScan (const juce::StringArray& allFiles,
                                          const juce::KnownPluginList& list,
                                          juce::AudioPluginFormat& format,
                                          juce::Array<SkippedPlugin>& skipped);

// Scan-Ergebnisse in die KnownPluginList übernehmen. Entfernt vorher Alteinträge
// derselben Dateien (uid kann sich bei Plugin-Updates ändern; ein stehen bleibender
// Alteintrag mit veralteter mtime erzwingt sonst bei jedem Start einen Rescan) und
// stempelt lastFileModTime FRISCH: Plugins (UAD, Acustica, ...) schreiben beim
// Laden/Entladen in ihre Bundles, die im Kindprozess erfasste mtime ist dann schon
// wieder veraltet -> Dauer-Rescan-Schleife. Beim Merge ist der Kindprozess beendet.
void mergeScanResults (juce::KnownPluginList& list, const ScanOutcome& outcome);

// Liegt path in (oder unter) einem der Ordner? Echter Pfadvergleich statt String-Prefix
// (C:\Plugins darf nicht C:\Plugins2\... matchen); auf Windows case-insensitiv.
bool pathIsInsideAnyFolder (const juce::String& path, const juce::StringArray& folders);

// Maßgebliche Änderungszeit eines VST3-Pfads: für Einzeldateien die Datei-mtime; für
// Bundle-ORDNER die neueste mtime der inneren *.vst3-Binaries (Contents\<arch>-win\...).
// Grund: Companion-Dienste (UA Connect, Acustica Aquarius, Minimal Audio Stream, ...)
// schreiben Logs/Content in die Bundles und bumpen die Ordner-mtime zwischen den App-
// Starts -> Dauer-Rescan. Die Binary selbst ändert nur ein echtes Plugin-Update.
// Ordner ohne innere .vst3-Datei: Fallback auf die Ordner-mtime.
juce::Time effectiveModTime (const juce::File& vst3FileOrBundle);

// VST3-Format mit bundle-robustem Rescan-Check (siehe effectiveModTime). Überall dort
// verwenden, wo pluginNeedsRescanning/isListingUpToDate ins Spiel kommt.
#if JUCE_PLUGINHOST_VST3
struct MicVST3Format : juce::VST3PluginFormat
{
    bool pluginNeedsRescanning (const juce::PluginDescription& d) override
    {
        return effectiveModTime (juce::File (d.fileOrIdentifier)) != d.lastFileModTime;
    }
};
#endif

// Hintergrund-Thread um runScan herum; marshallt Callbacks auf den Message-Thread.
// Besitzt den echten ChildProcess-Runner (oder einen injizierten für Tests).
class ScanCoordinator : private juce::Thread
{
public:
    static constexpr int defaultTimeoutMs = 120000;   // großzügig: auch Shell-Plugins (WaveShell) schaffen den Erstscan oft
    static constexpr int retryTimeoutMs   = 600000;   // "Retry skipped": WaveShell & Co. brauchen Minuten

    ScanCoordinator (juce::StringArray filesToScan,
                     std::function<void (int, int, juce::String)> onProgress,
                     std::function<void (ScanOutcome)> onFinished,
                     int timeoutMsIn = defaultTimeoutMs,
                     std::unique_ptr<ScanProcessRunner> runnerOverride = nullptr);
    ~ScanCoordinator() override;   // signalisiert Abbruch, killt laufenden Kindprozess

    void skipCurrentFile() { skipRequest.store (true); }   // vom Skip-Button (Message-Thread)

private:
    void run() override;

    juce::StringArray files;
    std::function<void (int, int, juce::String)> progressCb;
    std::function<void (ScanOutcome)> finishedCb;
    int timeoutMs = defaultTimeoutMs;
    std::unique_ptr<ScanProcessRunner> runner;
    std::atomic<bool> skipRequest { false };

    // Lebenszeit-Wächter für bereits via callAsync eingereihte Callbacks: Destruktor
    // und die Lambdas laufen beide auf dem Message-Thread, daher ist ein simples bool
    // hinter shared_ptr racefrei; der Worker-Thread kopiert nur den shared_ptr selbst
    // (dessen Refcount threadsicher ist), fasst den bool aber nie an.
    std::shared_ptr<bool> alive = std::make_shared<bool> (true);
};
