#include <juce_gui_basics/juce_gui_basics.h>
#include "audio/AudioEngine.h"
#include "state/Persistence.h"
#include "state/AutostartRegistry.h"
#include "ui/MainComponent.h"
#include "ui/TrayIcon.h"

#if JUCE_WINDOWS
 extern "C" __declspec (dllimport) unsigned int __stdcall SetErrorMode (unsigned int);
 static constexpr unsigned int kSemFailCriticalErrors = 0x0001;
 static constexpr unsigned int kSemNoGpFaultErrorBox  = 0x0002;
 static constexpr unsigned int kSemNoOpenFileErrorBox = 0x8000;
 #include <excpt.h>

static unsigned int findTypesWithSehGuard (juce::VST3PluginFormat& format,
                                           juce::OwnedArray<juce::PluginDescription>& types,
                                           const juce::String& path)
{
    __try
    {
        format.findAllTypesForFile (types, path);
        return 0;
    }
    __except (1)
    {
        return (unsigned int) _exception_code();
    }
}
#endif

class MicVSTApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return "MicVST"; }
    const juce::String getApplicationVersion() override { return MICVST_VERSION; }

    bool moreThanOneInstanceAllowed() override
    {
        return juce::JUCEApplicationBase::getCommandLineParameterArray().contains ("--scan");
    }

    static int runScanChildMode()
    {
       #if JUCE_WINDOWS
        SetErrorMode (kSemFailCriticalErrors | kSemNoGpFaultErrorBox | kSemNoOpenFileErrorBox);
       #endif

        auto args = juce::JUCEApplicationBase::getCommandLineParameterArray();
        juce::String pluginPath, outPath;
        for (int i = 0; i < args.size() - 1; ++i)
        {
            if (args[i] == "--scan") pluginPath = args[i + 1].unquoted();
            if (args[i] == "--out")  outPath    = args[i + 1].unquoted();
        }
        if (pluginPath.isEmpty() || outPath.isEmpty()) return 2;

        juce::VST3PluginFormat format;
        juce::OwnedArray<juce::PluginDescription> types;
       #if JUCE_WINDOWS
        const unsigned int crashCode = findTypesWithSehGuard (format, types, pluginPath);
       #else
        const unsigned int crashCode = 0;
        format.findAllTypesForFile (types, pluginPath);
       #endif
        if (crashCode == 0 && types.isEmpty()) return 1;

        juce::XmlElement root ("MicVSTScanResult");
        if (crashCode != 0)
            root.setAttribute ("crashCode",
                               "0x" + juce::String::toHexString ((int) crashCode).toUpperCase());
        for (auto* t : types)
            root.addChildElement (t->createXml().release());
        if (! root.writeTo (juce::File (outPath))) return 1;
        return crashCode != 0 ? 3 : 0;
    }

    void initialise (const juce::String& commandLine) override
    {
        if (juce::JUCEApplicationBase::getCommandLineParameterArray().contains ("--scan"))
        {
            setApplicationReturnValue (runScanChildMode());
            quit();
            return;
        }

        auto logFile = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                          .getChildFile ("MicVST").getChildFile ("log.txt");
        logFile.getParentDirectory().createDirectory();
        logger.reset (new juce::FileLogger (logFile, "MicVST Log"));
        juce::Logger::setCurrentLogger (logger.get());

        const bool silent = commandLine.contains ("--tray");
        engine = std::make_unique<AudioEngine>();

        const bool firstRun = ! configFile().existsAsFile();
        MicVSTState state = loadState();

        updateCheckEnabled  = state.updateCheckEnabled;
        updateCheckAsked    = state.updateCheckAsked;
        lastNotifiedVersion = state.lastNotifiedVersion;

        engine->setPluginFolders (state.pluginFolders);
        engine->loadPluginCache();
        if (firstRun)
        {
            state.outputDevice = engine->detectCableOutput();
            juce::Logger::writeToLog (state.outputDevice.isEmpty()
                ? "No virtual cable found -> output 'none'"
                : "Host output (cable) = " + state.outputDevice);

            auto& dm = engine->getDeviceManager();
            dm.setCurrentAudioDeviceType (dm.preferredTypeName(), true);
            if (auto* type = dm.getCurrentAudioDeviceTypeObject())
            {
                type->scanForDevices();
                auto ins = type->getDeviceNames (true);
                const int def = type->getDefaultDeviceIndex (true);
                state.inputDevice = juce::isPositiveAndBelow (def, ins.size()) ? ins[def]
                                  : (ins.isEmpty() ? juce::String() : ins[0]);
            }
            juce::Logger::writeToLog ("Input = " + state.inputDevice);
        }
        engine->applyState (state);

        mainWindow = std::make_unique<MainWindow> ("MicVST", *engine, state.windowState);
        mainWindow->setVisible (! silent);

        tray = std::make_unique<TrayIcon>();
        tray->onToggleWindow = [this] { toggleWindow(); };
        tray->onQuit         = [this] { systemRequestedQuit(); };
        tray->isAutostartOn  = [] { return AutostartRegistry::isEnabled(); };
        tray->setAutostart   = [] (bool on) { AutostartRegistry::setEnabled (on); };

        mainWindow->onHide = [this] { maybeShowTrayHint(); persistState(); };
        engine->onDeviceChanged = [this] { persistState(); };
        engine->onStateChanged  = [this] { persistState(); };
        engine->onFactoryResetRequested = [this] { factoryReset(); };

        if (auto* mc = mainWindow->getContent())
        {
            mc->onUpdateCheckToggled = [this] (bool on) { updateCheckEnabled = on; persistState(); };
            mc->onUpdateFound = [this] (const juce::String& latestVersion, const juce::String&)
            {
                if (latestVersion == lastNotifiedVersion) return;
                lastNotifiedVersion = latestVersion;
                persistState();
                if (tray != nullptr)
                    tray->showInfoBubble ("Update available",
                        "MicVST " + latestVersion + " is available - click the version number to update.");
            };
            mc->setUpdateCheckEnabled (updateCheckEnabled, true);
        }

        juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
            .getChildFile ("MicVST").getChildFile ("plugin_scan.tmp").deleteFile();

        engine->startBackgroundScan();

        if (! silent && ! updateCheckAsked)
            askUpdateConsent();
    }

    void anotherInstanceStarted (const juce::String&) override
    {
        if (mainWindow != nullptr) { mainWindow->setVisible (true); mainWindow->toFront (true); }
    }

    void systemRequestedQuit() override
    {
        persistState();
        quit();
    }

    void shutdown() override
    {
        tray = nullptr;
        mainWindow = nullptr;
        engine = nullptr;
        juce::Logger::setCurrentLogger (nullptr);
        logger = nullptr;
    }

private:
    void persistState()
    {
        if (suppressPersist || engine == nullptr) return;
        auto s = engine->captureState();
        if (mainWindow != nullptr) s.windowState = mainWindow->getWindowStateAsString();
        s.updateCheckEnabled  = updateCheckEnabled;
        s.updateCheckAsked    = updateCheckAsked;
        s.lastNotifiedVersion = lastNotifiedVersion;
        saveState (s);
    }

    void factoryReset()
    {
        suppressPersist = true;
        configFile().deleteFile();
        AudioEngine::pluginCacheFile().deleteFile();
        juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
            .getChildFile ("MicVST").getChildFile ("tray_hint_shown").deleteFile();
        juce::Logger::writeToLog ("Factory-Reset: Daten gelöscht, App beendet sich");
        systemRequestedQuit();
    }

    void askUpdateConsent()
    {
        updateCheckAsked = true;
        persistState();
        juce::NativeMessageBox::showYesNoBox (
            juce::MessageBoxIconType::QuestionIcon,
            "Check for updates?",
            "Should MicVST check GitHub for a newer version on startup?\n\n"
            "Only a single request is sent to GitHub - no data is collected and there is no "
            "auto-installer. You can change this anytime with the \"Auto-Update-Check\" box.",
            nullptr,
            juce::ModalCallbackFunction::create ([this] (int result)
            {
                const bool yes = (result == 1);
                updateCheckEnabled = yes;
                persistState();
                if (mainWindow != nullptr)
                    if (auto* mc = mainWindow->getContent())
                        mc->setUpdateCheckEnabled (yes, true);
            }));
    }

    void toggleWindow()
    {
        if (mainWindow == nullptr) return;
        const bool show = ! mainWindow->isVisible();
        mainWindow->setVisible (show);
        if (show) mainWindow->toFront (true);
        else      maybeShowTrayHint();
    }

    void maybeShowTrayHint()
    {
        if (tray == nullptr) return;
        auto marker = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                        .getChildFile ("MicVST").getChildFile ("tray_hint_shown");
        if (marker.existsAsFile()) return;
        marker.create();
        tray->showInfoBubble ("MicVST", "Still running in the tray - right-click the icon for options.");
    }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow (juce::String name, AudioEngine& engine, const juce::String& windowState)
            : DocumentWindow (name, juce::Colours::darkgrey, DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            content = new MainComponent (engine);
            setContentOwned (content, false);
            setResizable (true, false);
            // v1.4 has a collapsible 4x4 soundboard. A little more vertical room keeps the
            // plugin rack useful even while Audio Pads are expanded.
            setResizeLimits (600, 560, 1600, 1400);
            if (windowState.isNotEmpty())
                restoreWindowStateFromString (windowState);
            else
                centreWithSize (600, 620);
        }
        std::function<void()> onHide;
        void closeButtonPressed() override { setVisible (false); if (onHide) onHide(); }
        MainComponent* getContent() const { return content; }
    private:
        MainComponent* content = nullptr;
    };

    std::unique_ptr<juce::FileLogger> logger;
    std::unique_ptr<AudioEngine> engine;
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<TrayIcon> tray;

    bool updateCheckEnabled = false;
    bool updateCheckAsked   = false;
    juce::String lastNotifiedVersion;
    bool suppressPersist = false;
};

START_JUCE_APPLICATION (MicVSTApplication)
