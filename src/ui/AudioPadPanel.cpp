#include "ui/AudioPadPanel.h"
#include "ui/GlobalHotkey.h"

namespace
{
    juce::String displayNameFor (const AudioPadState& s, int index)
    {
        if (s.name.trim().isNotEmpty()) return s.name.trim();
        if (s.filePath.isNotEmpty()) return juce::File (s.filePath).getFileNameWithoutExtension();
        return "Pad " + juce::String (index + 1) + "\nDrop audio";
    }

    juce::String routeShortName (AudioPadRoute route)
    {
        if (route == AudioPadRoute::preFx) return "PRE";
        if (route == AudioPadRoute::output2Only) return "MON";
        return "POST";
    }
}

AudioPadPanel::PadButton::PadButton (int index)
    : juce::Button ("Audio Pad " + juce::String (index + 1))
{
    setWantsKeyboardFocus (false);
}

void AudioPadPanel::PadButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    auto bg = customColour != 0 ? juce::Colour (customColour) : juce::Colour (0xff303236);
    if (missing) bg = bg.withMultipliedSaturation (0.25f).darker (0.25f);
    if (down) bg = bg.darker (0.25f);
    else if (highlighted) bg = bg.brighter (0.08f);

    g.setColour (bg);
    g.fillRoundedRectangle (bounds, 5.0f);

    if (playing)
    {
        auto progressArea = bounds.removeFromBottom (3.0f);
        progressArea.setWidth (progressArea.getWidth() * juce::jlimit (0.0f, 1.0f, progress));
        g.setColour (juce::Colours::orange.withAlpha (0.85f));
        g.fillRoundedRectangle (progressArea, 1.5f);
    }

    g.setColour (playing ? juce::Colours::orange : juce::Colour (0xff55585e));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 5.0f, playing ? 1.5f : 1.0f);

    g.setColour (juce::Colours::lightgrey.withAlpha (0.75f));
    g.setFont (juce::Font (juce::FontOptions (9.5f)));
    g.drawText (routeShortName (route), 6, 2, 36, 12, juce::Justification::centredLeft, false);
    if (hotkey.isNotEmpty())
        g.drawText (hotkey, getWidth() - 74, 2, 68, 12, juce::Justification::centredRight, false);

    g.setColour (missing ? juce::Colours::grey : juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (12.5f)));
    g.drawFittedText (getButtonText(), getLocalBounds().reduced (6, 12),
                      juce::Justification::centred, 2, 0.85f);
}

void AudioPadPanel::PadButton::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        if (onRightClick) onRightClick();
        return;
    }
    juce::Button::mouseDown (e);
}

AudioPadPanel::AudioPadPanel (AudioEngine& e)
    : engine (e), pads (e.getAudioPads())
{
    addAndMakeVisible (expandButton);
    expandButton.setTooltip ("Open the 4 x 4 soundboard");
    expandButton.onClick = [this] { setExpanded (! expanded); };

    masterLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (masterLabel);
    masterSlider.setRange (0.0, 1.5, 0.01);
    masterSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    masterSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 46, 18);
    masterSlider.setValue (pads.getMasterVolume(), juce::dontSendNotification);
    masterSlider.setTooltip ("Master volume for all Audio Pads");
    masterSlider.onValueChange = [this]
    {
        if (updating) return;
        pads.setMasterVolume ((float) masterSlider.getValue());
        engine.requestPersist();
    };
    addAndMakeVisible (masterSlider);

    stopAllButton.setTooltip ("Fade-stop all currently playing pads");
    stopAllButton.onClick = [this] { pads.stopAll (false); };
    addAndMakeVisible (stopAllButton);

    for (int i = 0; i < AudioPadEngine::padCount; ++i)
    {
        auto b = std::make_unique<PadButton> (i);
        b->onClick = [this, i]
        {
            selectPad (i);
            pads.triggerPad (i);
        };
        b->onRightClick = [this, i]
        {
            selectPad (i);
            showPadMenu (i);
        };
        addAndMakeVisible (*b);
        padButtons[(size_t) i] = std::move (b);
    }

    selectedLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (selectedLabel);
    nameEditor.setTextToShowWhenEmpty ("Pad name", juce::Colours::grey);
    nameEditor.setSelectAllWhenFocused (true);
    nameEditor.onReturnKey = [this] { commitInspector(); nameEditor.giveAwayKeyboardFocus(); };
    nameEditor.onFocusLost = [this] { commitInspector(); };
    addAndMakeVisible (nameEditor);

    loadButton.onClick = [this] { chooseFileForPad (selectedPad); };
    clearButton.onClick = [this]
    {
        pads.clearPad (selectedPad);
        engine.requestPersist();
        refreshPads();
        refreshInspector();
    };
    addAndMakeVisible (loadButton);
    addAndMakeVisible (clearButton);

    volumeLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (volumeLabel);
    volumeSlider.setRange (0.0, 1.5, 0.01);
    volumeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 18);
    volumeSlider.onValueChange = [this] { commitInspector(); };
    addAndMakeVisible (volumeSlider);

    loopToggle.onClick = [this] { commitInspector(); };
    loopToggle.setTooltip ("Loop this pad until it is stopped");
    addAndMakeVisible (loopToggle);

    routeBox.addItem ("Post FX", 1);
    routeBox.addItem ("Pre FX", 2);
    routeBox.addItem ("Output2 only", 3);
    routeBox.setTooltip ("Post FX: bypass effects | Pre FX: run through DSP/VST | Output2 only: local monitor only");
    routeBox.onChange = [this] { commitInspector(); };
    addAndMakeVisible (routeBox);

    retriggerBox.addItem ("Restart", 1);
    retriggerBox.addItem ("Stop", 2);
    retriggerBox.addItem ("Ignore", 3);
    retriggerBox.setTooltip ("What another trigger does while the pad is already playing");
    retriggerBox.onChange = [this] { commitInspector(); };
    addAndMakeVisible (retriggerBox);

    hotkeyLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (hotkeyLabel);
    hotkeyEditor.setTextToShowWhenEmpty ("F8 / Ctrl+Shift+1", juce::Colours::grey);
    hotkeyEditor.setSelectAllWhenFocused (true);
    hotkeyEditor.setTooltip ("Global Windows hotkey. Examples: F8, Numpad1, Ctrl+Shift+1, Alt+Q");
    hotkeyEditor.onReturnKey = [this] { commitInspector(); hotkeyEditor.giveAwayKeyboardFocus(); };
    hotkeyEditor.onFocusLost = [this] { commitInspector(); };
    addAndMakeVisible (hotkeyEditor);

    auto setupFade = [] (juce::Slider& s)
    {
        s.setRange (0.0, 2000.0, 1.0);
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 48, 18);
    };
    setupFade (fadeInSlider);
    setupFade (fadeOutSlider);
    fadeInSlider.onValueChange = [this] { commitInspector(); };
    fadeOutSlider.onValueChange = [this] { commitInspector(); };
    fadeInLabel.setJustificationType (juce::Justification::centredRight);
    fadeOutLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (fadeInLabel); addAndMakeVisible (fadeInSlider);
    addAndMakeVisible (fadeOutLabel); addAndMakeVisible (fadeOutSlider);

    colourButton.onClick = [this] { showColourMenu(); };
    addAndMakeVisible (colourButton);

    setExpanded (false);
    selectPad (0);
    startTimerHz (30);
}

AudioPadPanel::~AudioPadPanel()
{
    stopTimer();
}

void AudioPadPanel::setExpanded (bool shouldExpand)
{
    expanded = shouldExpand;
    expandButton.setButtonText (expanded ? "Audio Pads  v" : "Audio Pads  >");

    juce::Component* inspectorControls[] = {
        &masterLabel, &masterSlider, &stopAllButton,
        &selectedLabel, &nameEditor, &loadButton, &clearButton, &volumeLabel,
        &volumeSlider, &loopToggle, &routeBox, &retriggerBox, &hotkeyLabel,
        &hotkeyEditor, &fadeInLabel, &fadeInSlider, &fadeOutLabel, &fadeOutSlider,
        &colourButton
    };
    for (auto* c : inspectorControls)
        c->setVisible (expanded);
    for (auto& b : padButtons)
        b->setVisible (expanded);

    if (onPreferredHeightChanged) onPreferredHeightChanged();
    resized();
}

void AudioPadPanel::selectPad (int index)
{
    selectedPad = juce::jlimit (0, AudioPadEngine::padCount - 1, index);
    refreshInspector();
}

void AudioPadPanel::refreshPads()
{
    for (int i = 0; i < AudioPadEngine::padCount; ++i)
    {
        auto state = pads.getPadState (i);
        auto& b = *padButtons[(size_t) i];
        b.setButtonText (displayNameFor (state, i));
        b.playing = pads.isPlaying (i);
        b.progress = pads.progress (i);
        b.hotkey = state.hotkey;
        b.route = state.route;
        b.customColour = state.colourARGB;
        b.missing = state.filePath.isNotEmpty() && ! juce::File (state.filePath).existsAsFile();
        b.repaint();
    }
}

void AudioPadPanel::refreshInspector()
{
    updating = true;
    const auto state = pads.getPadState (selectedPad);
    selectedLabel.setText ("Pad " + juce::String (selectedPad + 1), juce::dontSendNotification);
    nameEditor.setText (state.name, false);
    volumeSlider.setValue (state.volume, juce::dontSendNotification);
    loopToggle.setToggleState (state.loop, juce::dontSendNotification);
    routeBox.setSelectedId ((int) state.route + 1, juce::dontSendNotification);
    retriggerBox.setSelectedId ((int) state.retrigger + 1, juce::dontSendNotification);
    hotkeyEditor.setText (state.hotkey, false);
    fadeInSlider.setValue (state.fadeInMs, juce::dontSendNotification);
    fadeOutSlider.setValue (state.fadeOutMs, juce::dontSendNotification);
    updating = false;
}

void AudioPadPanel::commitInspector()
{
    if (updating) return;
    auto state = pads.getPadState (selectedPad);
    state.name = nameEditor.getText().trim();
    state.volume = (float) volumeSlider.getValue();
    state.loop = loopToggle.getToggleState();
    if (routeBox.getSelectedId() > 0) state.route = (AudioPadRoute) (routeBox.getSelectedId() - 1);
    if (retriggerBox.getSelectedId() > 0) state.retrigger = (AudioPadRetrigger) (retriggerBox.getSelectedId() - 1);
    state.hotkey = hotkeyEditor.getText().trim();
    state.fadeInMs = (float) fadeInSlider.getValue();
    state.fadeOutMs = (float) fadeOutSlider.getValue();
    pads.setPadState (selectedPad, state, false);
    engine.requestPersist();
    refreshPads();
}

void AudioPadPanel::loadIntoPad (int index, const juce::File& file)
{
    const auto err = pads.loadFile (index, file);
    if (err.isNotEmpty())
    {
        juce::NativeMessageBox::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                      "Could not load audio", err);
        return;
    }
    selectedPad = index;
    engine.requestPersist();
    refreshPads();
    refreshInspector();
}

void AudioPadPanel::chooseFileForPad (int index)
{
    chooser = std::make_unique<juce::FileChooser> ("Choose audio for Pad " + juce::String (index + 1),
                                                    juce::File(),
                                                    "*.wav;*.mp3;*.flac;*.ogg;*.aif;*.aiff");
    juce::Component::SafePointer<AudioPadPanel> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe, index] (const juce::FileChooser& fc)
        {
            if (auto* self = safe.getComponent())
            {
                const auto file = fc.getResult();
                if (file.existsAsFile()) self->loadIntoPad (index, file);
            }
        });
}

void AudioPadPanel::showPadMenu (int index)
{
    const auto state = pads.getPadState (index);
    juce::PopupMenu menu;
    menu.addItem (1, "Load / replace audio...");
    menu.addItem (2, "Stop");
    menu.addItem (3, "Loop", true, state.loop);

    juce::PopupMenu routeMenu;
    routeMenu.addItem (10, "Post FX", true, state.route == AudioPadRoute::postFx);
    routeMenu.addItem (11, "Pre FX", true, state.route == AudioPadRoute::preFx);
    routeMenu.addItem (12, "Output2 only", true, state.route == AudioPadRoute::output2Only);
    menu.addSubMenu ("Route", routeMenu);

    juce::PopupMenu retriggerMenu;
    retriggerMenu.addItem (20, "Restart", true, state.retrigger == AudioPadRetrigger::restart);
    retriggerMenu.addItem (21, "Stop", true, state.retrigger == AudioPadRetrigger::stop);
    retriggerMenu.addItem (22, "Ignore", true, state.retrigger == AudioPadRetrigger::ignore);
    menu.addSubMenu ("Retrigger", retriggerMenu);

    menu.addItem (30, "Pad color...");
    menu.addSeparator();
    menu.addItem (40, "Clear pad", state.filePath.isNotEmpty());

    juce::Component::SafePointer<AudioPadPanel> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (padButtons[(size_t) index].get()),
        [safe, index] (int result)
        {
            auto* self = safe.getComponent();
            if (self == nullptr || result == 0) return;
            self->selectedPad = index;
            auto s = self->pads.getPadState (index);
            if (result == 1) { self->chooseFileForPad (index); return; }
            if (result == 2) { self->pads.stopPad (index); return; }
            if (result == 3) s.loop = ! s.loop;
            if (result >= 10 && result <= 12) s.route = (AudioPadRoute) (result - 10);
            if (result >= 20 && result <= 22) s.retrigger = (AudioPadRetrigger) (result - 20);
            if (result == 30) { self->showColourMenu(); return; }
            if (result == 40)
            {
                self->pads.clearPad (index);
                self->engine.requestPersist();
                self->refreshPads(); self->refreshInspector();
                return;
            }
            self->pads.setPadState (index, s, false);
            self->engine.requestPersist();
            self->refreshPads(); self->refreshInspector();
        });
}

void AudioPadPanel::showColourMenu()
{
    juce::PopupMenu menu;
    menu.addItem (100, "Default");
    menu.addItem (101, "Slate");
    menu.addItem (102, "Forest");
    menu.addItem (103, "Plum");
    menu.addItem (104, "Cocoa");
    menu.addItem (105, "Ocean");
    menu.addItem (106, "Amber");

    juce::Component::SafePointer<AudioPadPanel> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&colourButton),
        [safe] (int result)
        {
            auto* self = safe.getComponent();
            if (self == nullptr || result < 100) return;
            static constexpr juce::uint32 colours[] =
            {
                0, 0xff3b4148, 0xff35473c, 0xff493a4b, 0xff4b4037, 0xff344850, 0xff514633
            };
            const int idx = juce::jlimit (0, 6, result - 100);
            auto s = self->pads.getPadState (self->selectedPad);
            s.colourARGB = colours[idx];
            self->pads.setPadState (self->selectedPad, s, false);
            self->engine.requestPersist();
            self->refreshPads();
        });
}

int AudioPadPanel::padAt (juce::Point<int> p) const
{
    for (int i = 0; i < AudioPadEngine::padCount; ++i)
        if (padButtons[(size_t) i]->getBounds().contains (p)) return i;
    return -1;
}

bool AudioPadPanel::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
        if (AudioPadEngine::supportsFile (juce::File (f))) return true;
    return false;
}

void AudioPadPanel::filesDropped (const juce::StringArray& files, int x, int y)
{
    int target = padAt ({ x, y });
    if (target < 0) target = selectedPad;
    for (auto& path : files)
    {
        if (target >= AudioPadEngine::padCount) break;
        const juce::File file (path);
        if (! AudioPadEngine::supportsFile (file)) continue;
        loadIntoPad (target++, file);
    }
}

void AudioPadPanel::timerCallback()
{
    refreshPads();

    const bool editing = hotkeyEditor.hasKeyboardFocus (true);
    for (int i = 0; i < AudioPadEngine::padCount; ++i)
    {
        const auto hotkey = pads.getPadState (i).hotkey;
        const bool down = ! editing && hotkey.isNotEmpty() && isGlobalHotkeyDown (hotkey);
        if (down && ! hotkeyWasDown[(size_t) i]) pads.triggerPad (i);
        hotkeyWasDown[(size_t) i] = down;
    }
}

void AudioPadPanel::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop (28);
    expandButton.setBounds (header.removeFromLeft (128));
    if (! expanded) return;

    stopAllButton.setBounds (header.removeFromRight (72));
    header.removeFromRight (4);
    masterSlider.setBounds (header.removeFromRight (132));
    masterLabel.setBounds (header.removeFromRight (52));
    r.removeFromTop (5);

    auto grid = r.removeFromTop (180);
    constexpr int cols = 4, rows = 4, gap = 4;
    const int cellW = (grid.getWidth() - gap * (cols - 1)) / cols;
    const int cellH = (grid.getHeight() - gap * (rows - 1)) / rows;
    for (int row = 0; row < rows; ++row)
        for (int col = 0; col < cols; ++col)
        {
            const int i = row * cols + col;
            padButtons[(size_t) i]->setBounds (grid.getX() + col * (cellW + gap),
                                               grid.getY() + row * (cellH + gap), cellW, cellH);
        }
    r.removeFromTop (5);

    auto line1 = r.removeFromTop (26);
    selectedLabel.setBounds (line1.removeFromLeft (54));
    line1.removeFromLeft (4);
    clearButton.setBounds (line1.removeFromRight (54));
    line1.removeFromRight (4);
    loadButton.setBounds (line1.removeFromRight (54));
    line1.removeFromRight (4);
    nameEditor.setBounds (line1);
    r.removeFromTop (3);

    auto line2 = r.removeFromTop (26);
    volumeLabel.setBounds (line2.removeFromLeft (30));
    volumeSlider.setBounds (line2.removeFromLeft (132));
    line2.removeFromLeft (6);
    loopToggle.setBounds (line2.removeFromLeft (58));
    line2.removeFromLeft (6);
    routeBox.setBounds (line2.removeFromLeft (112));
    line2.removeFromLeft (6);
    retriggerBox.setBounds (line2.removeFromLeft (104));
    r.removeFromTop (3);

    auto line3 = r.removeFromTop (26);
    hotkeyLabel.setBounds (line3.removeFromLeft (48));
    hotkeyEditor.setBounds (line3.removeFromLeft (126));
    line3.removeFromLeft (6);
    fadeInLabel.setBounds (line3.removeFromLeft (38));
    fadeInSlider.setBounds (line3.removeFromLeft (112));
    line3.removeFromLeft (6);
    fadeOutLabel.setBounds (line3.removeFromLeft (46));
    fadeOutSlider.setBounds (line3.removeFromLeft (112));
    line3.removeFromLeft (6);
    colourButton.setBounds (line3.removeFromLeft (58));
}
