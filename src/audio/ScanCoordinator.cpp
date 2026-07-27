#include "audio/ScanCoordinator.h"

ScanOutcome runScan (const juce::StringArray& files, ScanProcessRunner& runner, int timeoutMs,
                     std::function<void (int, int, const juce::String&)> progress,
                     std::function<bool()> shouldExit,
                     std::atomic<bool>* skipRequest)
{
    ScanOutcome out;
    const int total = files.size();

    for (int i = 0; i < total; ++i)
    {
        if (shouldExit && shouldExit()) return out;   // completed bleibt false

        if (skipRequest != nullptr)
            skipRequest->store (false);   // Skip-Wunsch gilt immer nur für die aktuelle Datei

        const auto& file = files[i];
        if (progress)
            progress (i + 1, total, juce::File (file).getFileNameWithoutExtension());

        auto r = runner.run (file, timeoutMs,
                             shouldExit ? shouldExit : [] { return false; },
                             [skipRequest] { return skipRequest != nullptr && skipRequest->load(); });

        if (r.status == ScanProcessResult::Status::aborted)
            return out;

        if (r.status == ScanProcessResult::Status::ok)
        {
            juce::Array<juce::PluginDescription> descs;
            if (parseScanResultXml (r.resultXmlText, descs))
            {
                out.found.addArray (descs);
                continue;
            }
            r.status = ScanProcessResult::Status::failed;   // ok ohne brauchbares XML = failed
        }

        const auto reason = r.status == ScanProcessResult::Status::timeout       ? "unresponsive"
                          : r.status == ScanProcessResult::Status::skippedByUser ? "skipped"
                                                                                 : "failed";
        out.skipped.add ({ file, reason,
                           effectiveModTime (juce::File (file)).toMilliseconds() });
    }

    out.completed = true;
    return out;
}

bool parseScanResultXml (const juce::String& xmlText, juce::Array<juce::PluginDescription>& out)
{
    auto xml = juce::parseXML (xmlText);
    if (xml == nullptr || ! xml->hasTagName ("MicVSTScanResult")) return false;

    bool any = false;
    for (auto* child : xml->getChildIterator())
    {
        juce::PluginDescription d;
        if (d.loadFromXml (*child)) { out.add (d); any = true; }
    }
    return any;
}

juce::StringArray filterFilesNeedingScan (const juce::StringArray& allFiles,
                                          const juce::KnownPluginList& list,
                                          juce::AudioPluginFormat& format,
                                          juce::Array<SkippedPlugin>& skipped)
{
    // Stale Skips entfernen: Datei geändert (Plugin-Update) -> neuer Versuch.
    for (int i = skipped.size(); --i >= 0;)
    {
        const juce::File f (skipped[i].file);
        if (! f.exists() || effectiveModTime (f).toMilliseconds() != skipped[i].fileTimeMs)
        {
            juce::Logger::writeToLog ("Scan: Skip aufgehoben (Datei geändert/entfernt): " + skipped[i].file);
            skipped.remove (i);
        }
    }

    auto isSkipped = [&] (const juce::String& file)
    {
        for (auto& s : skipped) if (s.file == file) return true;
        return false;
    };

    juce::StringArray out;
    for (auto& f : allFiles)
    {
        if (isSkipped (f) || list.isListingUpToDate (f, format))
            continue;
        // Grund + Zeitstempel loggen: macht den Dauer-Rescan-Fall in log.txt remote
        // diagnostizierbar (cache = gespeicherte mtime, datei = aktuelle effectiveModTime).
        juce::String detail;
        if (auto t = list.getTypeForFile (f))
            detail = " (cache=" + juce::String (t->lastFileModTime.toMilliseconds())
                   + " datei=" + juce::String (effectiveModTime (juce::File (f)).toMilliseconds()) + ")";
        juce::Logger::writeToLog (juce::String ("Scan ")
            + (list.getTypeForFile (f) == nullptr ? "(neu): " : "(geändert): ") + f + detail);
        out.add (f);
    }
    return out;
}

void mergeScanResults (juce::KnownPluginList& list, const ScanOutcome& outcome)
{
    juce::StringArray scannedFiles;
    for (auto& d : outcome.found)
        scannedFiles.addIfNotAlreadyThere (d.fileOrIdentifier);
    for (auto& t : list.getTypes())
        if (scannedFiles.contains (t.fileOrIdentifier))
            list.removeType (t);

    for (auto d : outcome.found)   // Kopie: lastFileModTime wird neu gestempelt
    {
        d.lastFileModTime = effectiveModTime (juce::File (d.fileOrIdentifier));
        list.addType (d);
    }
}

bool pathIsInsideAnyFolder (const juce::String& path, const juce::StringArray& folders)
{
    const juce::File f (path);
    for (auto& folder : folders)
        if (folder.isNotEmpty()
            && (f == juce::File (folder) || f.isAChildOf (juce::File (folder))))
            return true;
    return false;
}

juce::Time effectiveModTime (const juce::File& vst3FileOrBundle)
{
    if (! vst3FileOrBundle.isDirectory())
        return vst3FileOrBundle.getLastModificationTime();

    juce::Time newest;
    for (auto& inner : vst3FileOrBundle.findChildFiles (juce::File::findFiles, true, "*.vst3"))
        newest = juce::jmax (newest, inner.getLastModificationTime());
    return newest != juce::Time() ? newest : vst3FileOrBundle.getLastModificationTime();
}

// ============================ echter Kindprozess-Runner ============================

namespace
{
    // Startet MicVST.exe --scan <plugin> --out <tmpfile> und wartet in 100-ms-Schritten,
    // damit App-Ende (shouldAbort) den Kindprozess sofort killen kann.
    struct ChildProcessRunner : ScanProcessRunner
    {
        ScanProcessResult run (const juce::String& pluginPath, int timeoutMs,
                               std::function<bool()> shouldAbort,
                               std::function<bool()> shouldSkip) override
        {
            const auto exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
            const auto outFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                    .getNonexistentChildFile ("micvst_scan", ".xml");

            juce::ChildProcess proc;
            if (! proc.start (juce::StringArray { exe.getFullPathName(), "--scan", pluginPath,
                                                  "--out", outFile.getFullPathName() }, 0))
                return { ScanProcessResult::Status::failed, {} };

            // Überlauf-sichere Deadline (getMillisecondCounter wraps nach ~49.7 Tagen)
            const auto startMs = juce::Time::getMillisecondCounter();
            while (proc.isRunning())
            {
                if (shouldAbort()) { proc.kill(); outFile.deleteFile();
                                     return { ScanProcessResult::Status::aborted, {} }; }
                if (shouldSkip()) { proc.kill(); outFile.deleteFile();
                                    return { ScanProcessResult::Status::skippedByUser, {} }; }
                if (juce::Time::getMillisecondCounter() - startMs >= (juce::uint32) timeoutMs)
                {   proc.kill(); outFile.deleteFile();
                    return { ScanProcessResult::Status::timeout, {} }; }
                juce::Thread::sleep (100);
            }

            ScanProcessResult r;
            if (proc.getExitCode() == 0 && outFile.existsAsFile())
            {
                r.status = ScanProcessResult::Status::ok;
                r.resultXmlText = outFile.loadFileAsString();
            }
            outFile.deleteFile();
            return r;
        }
    };
}

ScanCoordinator::ScanCoordinator (juce::StringArray filesToScan,
                                  std::function<void (int, int, juce::String)> onProgress,
                                  std::function<void (ScanOutcome)> onFinished,
                                  int timeoutMsIn,
                                  std::unique_ptr<ScanProcessRunner> runnerOverride)
    : juce::Thread ("PluginScan"),
      files (std::move (filesToScan)),
      progressCb (std::move (onProgress)),
      finishedCb (std::move (onFinished)),
      timeoutMs (timeoutMsIn),
      runner (runnerOverride != nullptr ? std::move (runnerOverride)
                                        : std::make_unique<ChildProcessRunner>())
{
    startThread();
}

ScanCoordinator::~ScanCoordinator()
{
    // Vor stopThread markieren: bereits via callAsync eingereihte Lambdas (siehe run())
    // prüfen *alive und verwerfen sich selbst, statt den Callback auf einem ggf. schon
    // zerstörten Owner aufzurufen. stopThread kann solche Nachrichten nicht zurückziehen.
    *alive = false;
    stopThread (5000);   // shouldExit -> Runner killt den Kindprozess binnen ~100 ms
}

void ScanCoordinator::run()
{
    auto outcome = runScan (files, *runner, timeoutMs,
        [this] (int cur, int total, const juce::String& name)
        {
            if (progressCb)
                juce::MessageManager::callAsync ([alive = alive, cb = progressCb, cur, total, name]
                                                 { if (*alive) cb (cur, total, name); });
        },
        [this] { return threadShouldExit(); }, &skipRequest);

    if (outcome.completed && finishedCb)
        juce::MessageManager::callAsync ([alive = alive, cb = finishedCb, outcome]
                                         { if (*alive) cb (outcome); });
}
