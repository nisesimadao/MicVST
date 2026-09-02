#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "audio/AudioEngine.h"

// Kleines "?"-Icon, das bei Hover einen Hilfetext zeigt (via TooltipWindow im MainComponent).
struct InfoIcon : juce::Component, juce::SettableTooltipClient
{
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const float d = juce::jmin (b.getWidth(), b.getHeight()) - 1.5f;   // Durchmesser = kleinere Seite
        auto circle = juce::Rectangle<float> (d, d).withCentre (b.getCentre());
        g.setColour (juce::Colours::grey);
        g.drawEllipse (circle, 1.2f);
        g.setColour (juce::Colours::lightgrey);
        g.setFont (d * 0.66f);
        g.drawText ("?", getLocalBounds(), juce::Justification::centred);
    }
};

// Kompakte Geräteauswahl:
// - Input: physisches Mikrofon
// - Output: MicVST-verwaltetes CABLE Input (read-only)
// - Output 2: optionaler frei wählbarer Monitoring-Ausgang (Kopfhörer/Lautsprecher/etc.)
// Darunter optional eine Buffer-Zeile für den primären Mic->VB-CABLE-Pfad und eine
// dauerhaft sichtbare Statuszeile.
class DevicePanel : public juce::Component,
                    private juce::ChangeListener,
                    private juce::Timer
{
public:
    explicit DevicePanel (AudioEngine&);
    ~DevicePanel() override;
    void resized() override;

    int preferredHeight() const;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void refresh();
    void updateStatus();
    void apply();
    void applyOutput2();

    AudioEngine& engine;
    juce::Label inLabel { {}, "Input" }, outLabel { {}, "Output" }, out2Label { {}, "Output 2" };
    InfoIcon inInfo, outInfo, out2Info;
    juce::ComboBox inBox, outBox, out2Box;
    juce::StringArray output2Names;

    juce::Label bufLabel { {}, "Buffer" };
    InfoIcon bufInfo;
    juce::ComboBox bufBox;
    juce::Label statusLabel;

    bool updating = false;
};
