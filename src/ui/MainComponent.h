#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "audio/AudioEngine.h"
#include "ui/LevelMeterComponent.h"
#include "ui/PluginListView.h"
#include "ui/DevicePanel.h"
#include "ui/AudioPadPanel.h"
#include "state/AutostartRegistry.h"
#include "net/UpdateChecker.h"

class MainComponent : public juce::Component, private juce::Timer
{
public:
    explicit MainComponent (AudioEngine& engine);
    ~MainComponent() override;
    void resized() override;

    void setUpdateCheckEnabled (bool on, bool runIfOn);

    std::function<void (bool)> onUpdateCheckToggled;
    std::function<void (const juce::String& latestVersion, const juce::String& url)> onUpdateFound;

private:
    void timerCallback() override;
    void updateCableHint();
    void showHowTo();
    void startUpdateCheck();
    void showUpdateAvailable (const juce::String& latestVersion, const juce::String& url);
    AudioEngine& engine;
    std::unique_ptr<DevicePanel> devicePanel;
    std::unique_ptr<AudioPadPanel> audioPadPanel;
    LevelMeterComponent inMeter, outMeter;
    DbScaleComponent dbScale;
    juce::Label inLabel { {}, "In" }, outLabel { {}, "Out" };
    std::unique_ptr<PluginListView> pluginList;
    juce::HyperlinkButton versionLink;
    juce::String currentVersion;
    juce::ToggleButton updateToggle { "Auto-Update-Check" };
    UpdateChecker updateChecker;
    juce::TextButton howToBtn { "How To" };
    std::unique_ptr<juce::DocumentWindow> howToWindow;
    juce::ToggleButton autostartToggle { "Run at startup" };
    juce::HyperlinkButton cableHint { "No virtual audio cable found - click to install VB-Cable",
                                      juce::URL ("https://vb-audio.com/Cable/") };
    juce::TooltipWindow tooltipWindow { nullptr, 100 };
};
