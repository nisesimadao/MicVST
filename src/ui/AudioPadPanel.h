#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
#include "audio/AudioEngine.h"

class AudioPadPanel : public juce::Component,
                      public juce::FileDragAndDropTarget,
                      private juce::Timer
{
public:
    explicit AudioPadPanel (AudioEngine& engine);
    ~AudioPadPanel() override;

    void resized() override;
    int preferredHeight() const { return expanded ? 306 : 30; }

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    std::function<void()> onPreferredHeightChanged;

private:
    class PadButton : public juce::Button
    {
    public:
        explicit PadButton (int index);
        void paintButton (juce::Graphics&, bool highlighted, bool down) override;
        void mouseDown (const juce::MouseEvent&) override;

        std::function<void()> onRightClick;
        float progress = 0.0f;
        bool playing = false;
        bool missing = false;
        juce::String hotkey;
        AudioPadRoute route = AudioPadRoute::postFx;
        juce::uint32 customColour = 0;
    };

    void timerCallback() override;
    void setExpanded (bool shouldExpand);
    void selectPad (int index);
    void refreshPads();
    void refreshInspector();
    void commitInspector();
    void loadIntoPad (int index, const juce::File& file);
    void chooseFileForPad (int index);
    void showPadMenu (int index);
    void showColourMenu();
    int padAt (juce::Point<int> p) const;

    AudioEngine& engine;
    AudioPadEngine& pads;
    bool expanded = false;
    bool updating = false;
    int selectedPad = 0;
    std::array<bool, AudioPadEngine::padCount> hotkeyWasDown {};

    juce::TextButton expandButton { "Audio Pads  >" };
    juce::Label masterLabel { {}, "Master" };
    juce::Slider masterSlider;
    juce::TextButton stopAllButton { "Stop all" };

    std::array<std::unique_ptr<PadButton>, AudioPadEngine::padCount> padButtons;

    juce::Label selectedLabel { {}, "Pad 1" };
    juce::TextEditor nameEditor;
    juce::TextButton loadButton { "Load" };
    juce::TextButton clearButton { "Clear" };

    juce::Label volumeLabel { {}, "Vol" };
    juce::Slider volumeSlider;
    juce::ToggleButton loopToggle { "Loop" };
    juce::ComboBox routeBox;
    juce::ComboBox retriggerBox;

    juce::Label hotkeyLabel { {}, "Hotkey" };
    juce::TextEditor hotkeyEditor;
    juce::Label fadeInLabel { {}, "In ms" };
    juce::Slider fadeInSlider;
    juce::Label fadeOutLabel { {}, "Out ms" };
    juce::Slider fadeOutSlider;
    juce::TextButton colourButton { "Color" };

    std::unique_ptr<juce::FileChooser> chooser;
};
