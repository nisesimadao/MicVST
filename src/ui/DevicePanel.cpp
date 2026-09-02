#include "ui/DevicePanel.h"

namespace
{
    constexpr const char* kVBCableRenderEndpoint = "CABLE Input";
}

DevicePanel::DevicePanel (AudioEngine& e) : engine (e)
{
    for (auto* l : { &inLabel, &outLabel, &out2Label })
        l->setJustificationType (juce::Justification::centredLeft);

    addAndMakeVisible (inLabel);   addAndMakeVisible (inBox);
    addAndMakeVisible (outLabel);  addAndMakeVisible (outBox);
    addAndMakeVisible (out2Label); addAndMakeVisible (out2Box);

    inInfo.setTooltip ("Select your physical microphone here");
    outInfo.setTooltip ("Managed automatically by MicVST using VB-CABLE by VB-Audio (donationware). "
                        "Discord/OBS/Zoom should use \"CABLE Output\" as their microphone input.");
    out2Info.setTooltip ("Optional monitor output. Sends the same processed MicVST signal to "
                         "headphones, speakers, an audio interface, etc. Set to Off if you do not "
                         "want local monitoring. Using speakers near the mic can cause feedback.");
    addAndMakeVisible (inInfo);
    addAndMakeVisible (outInfo);
    addAndMakeVisible (out2Info);

    // Primary output is an implementation detail; Output2 is the user's free routing choice.
    outBox.setEnabled (false);

    bufInfo.setTooltip ("Buffer size in samples. Smaller = lower latency, higher CPU risk. "
                        "\"Auto\" uses the device default. Only shown when the devices "
                        "support multiple sizes (low-latency mode).");
    addChildComponent (bufLabel); addChildComponent (bufBox); addChildComponent (bufInfo);
    bufLabel.setJustificationType (juce::Justification::centredLeft);
    bufBox.onChange = [this] { apply(); };

    statusLabel.setJustificationType (juce::Justification::centredLeft);
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    statusLabel.setFont (juce::Font (juce::FontOptions (13.0f)));
    addAndMakeVisible (statusLabel);

    inBox.onChange = [this] { apply(); };
    out2Box.onChange = [this] { applyOutput2(); };

    engine.getDeviceManager().addChangeListener (this);
    refresh();
    startTimer (500);
}

DevicePanel::~DevicePanel()
{
    stopTimer();
    engine.getDeviceManager().removeChangeListener (this);
}

void DevicePanel::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refresh();
}

void DevicePanel::timerCallback()
{
    updateStatus();
}

void DevicePanel::refresh()
{
    updating = true;

    inBox.clear (juce::dontSendNotification);
    outBox.clear (juce::dontSendNotification);
    out2Box.clear (juce::dontSendNotification);

    auto& dm = engine.getDeviceManager();
    const auto setup = dm.getAudioDeviceSetup();

    if (auto* type = dm.getCurrentDeviceTypeObject())
    {
        const auto ins = type->getDeviceNames (true);
        for (int i = 0; i < ins.size(); ++i) inBox.addItem (ins[i], i + 1);
        const int si = ins.indexOf (setup.inputDeviceName);
        if (si >= 0) inBox.setSelectedId (si + 1, juce::dontSendNotification);

        // Primary Output is read-only: show only the effective VB-CABLE routing state.
        if (setup.outputDeviceName.containsIgnoreCase (kVBCableRenderEndpoint))
        {
            outBox.addItem (setup.outputDeviceName, 1);
            outBox.setSelectedId (1, juce::dontSendNotification);
        }
        else
        {
            outBox.addItem ("VB-CABLE backend not installed", 1);
            outBox.setSelectedId (1, juce::dontSendNotification);
        }
    }

    // Output2 is independent from the primary device pair. "Off" is always available.
    out2Box.addItem ("Off", 1);
    output2Names = engine.getOutput2DeviceNames();
    for (int i = 0; i < output2Names.size(); ++i)
        out2Box.addItem (output2Names[i], i + 2);

    const auto desiredOutput2 = engine.getOutput2Device();
    if (desiredOutput2.isEmpty())
    {
        out2Box.setSelectedId (1, juce::dontSendNotification);
    }
    else
    {
        const int index = output2Names.indexOf (desiredOutput2);
        if (index >= 0)
        {
            out2Box.setSelectedId (index + 2, juce::dontSendNotification);
        }
        else
        {
            // Keep a disconnected USB headset/interface visible instead of silently erasing
            // the saved selection. Reconnecting/reselecting it can recover the same setting.
            out2Box.addItem (desiredOutput2 + " (unavailable)", output2Names.size() + 2);
            out2Box.setSelectedId (output2Names.size() + 2, juce::dontSendNotification);
        }
    }

    // Buffer row belongs only to the primary mic -> VB-CABLE pair.
    bufBox.clear (juce::dontSendNotification);
    auto* dev = dm.getCurrentAudioDevice();
    const auto sizes = dev != nullptr ? dev->getAvailableBufferSizes() : juce::Array<int>();
    const bool showBuf = sizes.size() > 1;
    if (showBuf)
    {
        const double sr = dev->getCurrentSampleRate();
        bufBox.addItem ("Auto (default)", 1);
        for (int i = 0; i < sizes.size(); ++i)
            bufBox.addItem (juce::String (sizes[i]) + " samples ("
                                + juce::String (sizes[i] / sr * 1000.0, 1) + " ms)", i + 2);
        const int pref = engine.getPreferredBufferSize();
        const int idx = sizes.indexOf (pref);
        bufBox.setSelectedId (pref > 0 && idx >= 0 ? idx + 2 : 1, juce::dontSendNotification);

        if (pref > 0 && idx < 0)
            engine.setPreferredBufferSize (0);
    }
    if (showBuf != bufBox.isVisible())
    {
        bufLabel.setVisible (showBuf); bufBox.setVisible (showBuf); bufInfo.setVisible (showBuf);
        if (auto* p = getParentComponent()) p->resized();
    }

    updateStatus();
    updating = false;
}

void DevicePanel::updateStatus()
{
    auto& dm = engine.getDeviceManager();
    const auto setup = dm.getAudioDeviceSetup();
    const bool driverReady = setup.outputDeviceName.containsIgnoreCase (kVBCableRenderEndpoint);

    juce::String st = engine.isRunning() ? juce::String ("Active")
                                         : juce::String ("Idle - device disconnected");
    if (auto* dev = dm.getCurrentAudioDevice())
    {
        const double sr  = dev->getCurrentSampleRate();
        const int    buf = dev->getCurrentBufferSizeSamples();
        st << "   |   " << juce::String (sr, 0) << " Hz";
        st << "   |   buffer " << juce::String (buf);

        const int    pluginLatency = engine.getGraph().getLatencySamples();
        const double lat = (dev->getInputLatencyInSamples() + dev->getOutputLatencyInSamples()
                            + buf + pluginLatency) / sr * 1000.0;
        st << "   |   latency " << juce::String (lat, 1) << " ms";
    }

    st << (driverReady ? "   |   virtual mic: CABLE Output"
                       : "   |   VB-CABLE missing / reboot required");

    const auto out2 = engine.getOutput2Device();
    if (out2.isNotEmpty())
        st << (engine.isOutput2Running() ? "   |   Output2: " : "   |   Output2 unavailable: ") << out2;

    statusLabel.setText (st, juce::dontSendNotification);
}

void DevicePanel::apply()
{
    if (updating) return;

    auto& dm = engine.getDeviceManager();
    auto* type = dm.getCurrentDeviceTypeObject();
    if (type == nullptr) return;

    const auto ins = type->getDeviceNames (true);
    const int ii = inBox.getSelectedId() - 1;
    juce::String input = juce::isPositiveAndBelow (ii, ins.size()) ? ins[ii] : juce::String();

    int buf = 0;
    if (bufBox.isVisible() && bufBox.getSelectedId() >= 2)
    {
        auto* dev = dm.getCurrentAudioDevice();
        const auto sizes = dev != nullptr ? dev->getAvailableBufferSizes() : juce::Array<int>();
        const int idx = bufBox.getSelectedId() - 2;
        if (juce::isPositiveAndBelow (idx, sizes.size())) buf = sizes[idx];
    }
    engine.setPreferredBufferSize (buf);
    auto* curDev = dm.getCurrentAudioDevice();
    const int effective = buf > 0 ? buf : (curDev != nullptr ? curDev->getDefaultBufferSize() : 0);

    // Primary output remains hard-wired to CABLE Input.
    engine.setDeviceConfig (input, {}, 0.0, effective);
}

void DevicePanel::applyOutput2()
{
    if (updating) return;

    juce::String requested;
    const int id = out2Box.getSelectedId();
    if (id >= 2)
    {
        const int index = id - 2;
        if (juce::isPositiveAndBelow (index, output2Names.size()))
            requested = output2Names[index];
        else
            requested = engine.getOutput2Device(); // preserve an unavailable saved device.
    }

    const auto err = engine.setOutput2Device (requested);
    if (err.isNotEmpty())
        juce::Logger::writeToLog (err);
    updateStatus();
}

void DevicePanel::resized()
{
    constexpr int rowH = 26, labelW = 84, gap = 4;
    auto r = getLocalBounds();

    auto row = [&]
    {
        auto a = r.removeFromTop (rowH);
        r.removeFromTop (gap);
        return a;
    };

    auto labelWithInfo = [labelW] (juce::Rectangle<int> area, juce::Label& lbl, InfoIcon& info)
    {
        auto col = area.removeFromLeft (labelW);
        info.setBounds (col.removeFromRight (18).reduced (2));
        lbl.setBounds (col);
        return area;
    };

    auto inRow = row();
    inBox.setBounds (labelWithInfo (inRow, inLabel, inInfo));
    auto outRow = row();
    outBox.setBounds (labelWithInfo (outRow, outLabel, outInfo));
    auto out2Row = row();
    out2Box.setBounds (labelWithInfo (out2Row, out2Label, out2Info));

    if (bufBox.isVisible())
    {
        auto bufRow = row();
        bufBox.setBounds (labelWithInfo (bufRow, bufLabel, bufInfo));
    }

    statusLabel.setBounds (row());
}

int DevicePanel::preferredHeight() const
{
    // Input + Output + Output2 + status = 4 rows; optional primary Buffer = +1 row.
    return bufBox.isVisible() ? 146 : 116;
}
