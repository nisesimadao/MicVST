#include "ui/MainComponent.h"
#include "audio/PluginChain.h"

namespace
{
    juce::String howToText()
    {
        return
            "How to use MicVST\n"
            "\n"
            "1)  Pick your devices\n"
            "    Input    = your physical microphone\n"
            "    Output   = CABLE Input (managed automatically)\n"
            "    Output 2 = optional headphones / speakers for local monitoring\n"
            "\n"
            "2)  Build your effect chain\n"
            "    - + Plugin opens the built-in DSP and VST3 picker.\n"
            "    - Drag effects to reorder, Bypass to disable, double-click to edit.\n"
            "\n"
            "3)  Audio Pads / Soundboard\n"
            "    - Expand Audio Pads and drag WAV / MP3 / FLAC / OGG / AIFF files onto a pad.\n"
            "    - Click a pad or assign a global Windows hotkey such as F8 or Ctrl+Shift+1.\n"
            "    - Post FX plays the clip after your effects. Pre FX sends it through the DSP/VST chain.\n"
            "    - Output2 only is audible locally but is NOT sent to Discord.\n"
            "    - Each pad has volume, loop, retrigger mode and fade in/out settings.\n"
            "\n"
            "4)  Use it in Discord / OBS / Zoom / games\n"
            "    Select CABLE Output as the microphone.\n"
            "\n"
            "5)  Tray / startup\n"
            "    Closing the window keeps MicVST running in the tray. Pad global hotkeys keep working\n"
            "    while the window is hidden. Run at startup launches MicVST silently on Windows login.\n"
            "\n"
            "Auto-Update-Check\n"
            "    If enabled, MicVST asks GitHub once per startup whether a newer version exists.\n"
            "    No telemetry is sent and there is no forced auto-installer.";
    }

    const char* const kRepoUrl = "https://github.com/nisesimadao/MicVST";

    class HowToContent : public juce::Component
    {
    public:
        HowToContent()
        {
            body.setMultiLine (true);
            body.setReadOnly (true);
            body.setCaretVisible (false);
            body.setScrollbarsShown (true);
            body.setPopupMenuEnabled (false);
            body.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff1e1e1e));
            body.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
            body.setColour (juce::TextEditor::textColourId, juce::Colours::white);
            body.setFont (juce::Font (juce::FontOptions (15.0f)));
            body.setText (howToText(), false);
            addAndMakeVisible (body);

            setupLink (vbCable, "Download VB-Cable  (vb-audio.com/Cable)", "https://vb-audio.com/Cable/");
            setupLink (github,  "Check for updates on GitHub", kRepoUrl);
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced (12);
            github.setBounds (r.removeFromBottom (22));
            r.removeFromBottom (6);
            vbCable.setBounds (r.removeFromBottom (22));
            r.removeFromBottom (10);
            body.setBounds (r);
        }

    private:
        void setupLink (juce::HyperlinkButton& b, const juce::String& text, const juce::String& url)
        {
            b.setButtonText (text);
            b.setURL (juce::URL (url));
            b.setJustificationType (juce::Justification::centredLeft);
            b.setColour (juce::HyperlinkButton::textColourId, juce::Colours::orange);
            addAndMakeVisible (b);
        }

        juce::TextEditor body;
        juce::HyperlinkButton vbCable, github;
    };

    class HowToWindow : public juce::DocumentWindow
    {
    public:
        HowToWindow()
            : juce::DocumentWindow ("MicVST - How To", juce::Colours::darkgrey,
                                    juce::DocumentWindow::closeButton)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new HowToContent(), false);
            setResizable (true, false);
            centreWithSize (590, 610);
        }
        void closeButtonPressed() override { setVisible (false); }
    };
}

MainComponent::MainComponent (AudioEngine& e) : engine (e)
{
    devicePanel = std::make_unique<DevicePanel> (engine);
    addAndMakeVisible (*devicePanel);
    addAndMakeVisible (inMeter);
    addAndMakeVisible (outMeter);
    addAndMakeVisible (dbScale);
    addAndMakeVisible (inLabel);
    addAndMakeVisible (outLabel);
    inLabel.setJustificationType (juce::Justification::centredLeft);
    outLabel.setJustificationType (juce::Justification::centredLeft);

    audioPadPanel = std::make_unique<AudioPadPanel> (engine);
    audioPadPanel->onPreferredHeightChanged = [this] { resized(); };
    addAndMakeVisible (*audioPadPanel);

    pluginList = std::make_unique<PluginListView> (engine);
    addAndMakeVisible (*pluginList);

    addAndMakeVisible (howToBtn);
    howToBtn.setTooltip ("Quick setup guide");
    howToBtn.onClick = [this] { showHowTo(); };

    currentVersion = juce::JUCEApplication::getInstance()->getApplicationVersion();
    versionLink.setButtonText ("v" + currentVersion);
    versionLink.setURL (juce::URL (kRepoUrl));
    versionLink.setTooltip ("MicVST on GitHub");
    versionLink.setJustificationType (juce::Justification::centredLeft);
    versionLink.setColour (juce::HyperlinkButton::textColourId, juce::Colours::grey);
    addAndMakeVisible (versionLink);

    updateToggle.setTooltip ("Check GitHub on startup whether a newer version exists (opt-in, no data collected)");
    updateToggle.onClick = [this]
    {
        const bool on = updateToggle.getToggleState();
        if (onUpdateCheckToggled) onUpdateCheckToggled (on);
        if (on) startUpdateCheck();
    };
    addAndMakeVisible (updateToggle);

    addAndMakeVisible (autostartToggle);
    autostartToggle.setTooltip ("Launch MicVST silently into the tray when Windows starts");
    autostartToggle.setToggleState (AutostartRegistry::isEnabled(), juce::dontSendNotification);
    autostartToggle.onClick = [this] { AutostartRegistry::setEnabled (autostartToggle.getToggleState()); };

    cableHint.setColour (juce::HyperlinkButton::textColourId, juce::Colours::orange);
    cableHint.setJustificationType (juce::Justification::centredLeft);
    cableHint.setTooltip ("https://vb-audio.com/Cable/");
    addChildComponent (cableHint);

    engine.onStatusChanged = [this] { updateCableHint(); };
    updateCableHint();

    setSize (600, 600);
    startTimerHz (30);
}

MainComponent::~MainComponent()
{
    stopTimer();
    engine.onStatusChanged = nullptr;
}

void MainComponent::timerCallback()
{
    inMeter.setLevel (engine.inputLevel());
    outMeter.setLevel (engine.outputLevel());
    autostartToggle.setToggleState (AutostartRegistry::isEnabled(), juce::dontSendNotification);
}

void MainComponent::showHowTo()
{
    if (howToWindow == nullptr)
        howToWindow.reset (new HowToWindow());
    howToWindow->setVisible (true);
    howToWindow->toFront (true);
}

void MainComponent::setUpdateCheckEnabled (bool on, bool runIfOn)
{
    updateToggle.setToggleState (on, juce::dontSendNotification);
    if (on && runIfOn) startUpdateCheck();
}

void MainComponent::startUpdateCheck()
{
    juce::Component::SafePointer<MainComponent> safe (this);
    updateChecker.start (currentVersion, [safe] (UpdateChecker::Result r)
    {
        if (auto* self = safe.getComponent())
        {
            self->showUpdateAvailable (r.latestVersion, r.releaseUrl);
            if (self->onUpdateFound) self->onUpdateFound (r.latestVersion, r.releaseUrl);
        }
    });
}

void MainComponent::showUpdateAvailable (const juce::String& latestVersion, const juce::String& url)
{
    versionLink.setButtonText ("v" + currentVersion + " > Update available!");
    versionLink.setURL (juce::URL (url));
    versionLink.setTooltip ("MicVST " + latestVersion + " is available on GitHub - click to open");
    versionLink.setColour (juce::HyperlinkButton::textColourId, juce::Colours::orange);
    resized();
}

void MainComponent::updateCableHint()
{
    const bool noCable = engine.detectCableOutput().isEmpty();
    if (noCable != cableHint.isVisible())
    {
        cableHint.setVisible (noCable);
        resized();
    }
}

void MainComponent::resized()
{
    auto r = getLocalBounds().reduced (8);
    auto bottomRow = r.removeFromBottom (24);
    autostartToggle.setBounds (bottomRow.removeFromRight (120));
    bottomRow.removeFromRight (6);
    updateToggle.setBounds (bottomRow.removeFromRight (150));
    bottomRow.removeFromRight (6);
    howToBtn.setBounds (bottomRow.removeFromRight (80));
    versionLink.setBounds (bottomRow);
    versionLink.changeWidthToFitText();
    r.removeFromBottom (4);

    constexpr int labelW = 40;
    auto inRow = r.removeFromTop (22);
    inLabel.setBounds (inRow.removeFromLeft (labelW));
    inMeter.setBounds (inRow.reduced (2, 1));
    r.removeFromTop (4);
    auto outRow = r.removeFromTop (22);
    outLabel.setBounds (outRow.removeFromLeft (labelW));
    outMeter.setBounds (outRow.reduced (2, 1));
    auto scaleRow = r.removeFromTop (16);
    scaleRow.removeFromLeft (labelW);
    dbScale.setBounds (scaleRow.reduced (2, 0));
    r.removeFromTop (8);

    if (cableHint.isVisible())
    {
        cableHint.setBounds (r.removeFromTop (22));
        r.removeFromTop (4);
    }

    devicePanel->setBounds (r.removeFromTop (devicePanel->preferredHeight()));
    r.removeFromTop (6);

    audioPadPanel->setBounds (r.removeFromTop (audioPadPanel->preferredHeight()));
    r.removeFromTop (6);

    pluginList->setBounds (r);
}
