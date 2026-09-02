#include "state/Persistence.h"

namespace ids
{
    const juce::Identifier root ("MicVST"), inDev ("inputDevice"), outDev ("outputDevice"), out2Dev ("output2Device"),
        sr ("sampleRate"), userBuf ("userBufferSize"), folders ("pluginFolders"), window ("windowState"),
        updEnabled ("updateCheckEnabled"), updAsked ("updateCheckAsked"), updLast ("lastNotifiedVersion"),
        plugins ("plugins"), plugin ("plugin"), fileId ("fileOrId"), byp ("bypassed"), blob ("state"),
        pads ("audioPads"), pad ("pad"), padMaster ("audioPadMasterVolume"), padName ("name"),
        padFile ("filePath"), padVolume ("volume"), padLoop ("loop"), padRoute ("route"),
        padRetrigger ("retrigger"), padHotkey ("hotkey"), padFadeIn ("fadeInMs"),
        padFadeOut ("fadeOutMs"), padColour ("colourARGB");
}

juce::ValueTree toValueTree (const MicVSTState& s)
{
    juce::ValueTree t (ids::root);
    t.setProperty (ids::inDev, s.inputDevice, nullptr);
    t.setProperty (ids::outDev, s.outputDevice, nullptr);
    t.setProperty (ids::out2Dev, s.output2Device, nullptr);
    t.setProperty (ids::sr, s.sampleRate, nullptr);
    t.setProperty (ids::userBuf, s.bufferSize, nullptr);
    t.setProperty (ids::folders, s.pluginFolders.joinIntoString ("\n"), nullptr);
    t.setProperty (ids::window, s.windowState, nullptr);
    t.setProperty (ids::updEnabled, s.updateCheckEnabled, nullptr);
    t.setProperty (ids::updAsked, s.updateCheckAsked, nullptr);
    t.setProperty (ids::updLast, s.lastNotifiedVersion, nullptr);
    t.setProperty (ids::padMaster, s.audioPadMasterVolume, nullptr);

    juce::ValueTree list (ids::plugins);
    for (auto& p : s.plugins)
    {
        juce::ValueTree pt (ids::plugin);
        pt.setProperty (ids::fileId, p.fileOrId, nullptr);
        pt.setProperty (ids::byp, p.bypassed, nullptr);
        pt.setProperty (ids::blob, p.state.toBase64Encoding(), nullptr);
        list.appendChild (pt, nullptr);
    }
    t.appendChild (list, nullptr);

    juce::ValueTree padList (ids::pads);
    for (const auto& p : s.audioPads)
    {
        juce::ValueTree pt (ids::pad);
        pt.setProperty (ids::padName, p.name, nullptr);
        pt.setProperty (ids::padFile, p.filePath, nullptr);
        pt.setProperty (ids::padVolume, p.volume, nullptr);
        pt.setProperty (ids::padLoop, p.loop, nullptr);
        pt.setProperty (ids::padRoute, (int) p.route, nullptr);
        pt.setProperty (ids::padRetrigger, (int) p.retrigger, nullptr);
        pt.setProperty (ids::padHotkey, p.hotkey, nullptr);
        pt.setProperty (ids::padFadeIn, p.fadeInMs, nullptr);
        pt.setProperty (ids::padFadeOut, p.fadeOutMs, nullptr);
        pt.setProperty (ids::padColour, (juce::int64) p.colourARGB, nullptr);
        padList.appendChild (pt, nullptr);
    }
    t.appendChild (padList, nullptr);
    return t;
}

MicVSTState fromValueTree (const juce::ValueTree& t)
{
    MicVSTState s;
    s.inputDevice   = t.getProperty (ids::inDev);
    s.outputDevice  = t.getProperty (ids::outDev);
    s.output2Device = t.getProperty (ids::out2Dev).toString();
    s.sampleRate    = t.getProperty (ids::sr, 48000.0);
    // Migration v1.0.x: der alte Key "bufferSize" (immer 128) wird bewusst ignoriert --
    // im Shared-Modus war er nie wirksam. Bestandsnutzer starten mit Auto.
    s.bufferSize   = t.getProperty (ids::userBuf, 0);
    {
        const auto f = t.getProperty (ids::folders).toString();
        if (f.isNotEmpty()) { s.pluginFolders.addLines (f); s.pluginFolders.removeEmptyStrings(); }
    }
    s.windowState = t.getProperty (ids::window).toString();
    s.updateCheckEnabled  = t.getProperty (ids::updEnabled, false);
    s.updateCheckAsked    = t.getProperty (ids::updAsked, false);
    s.lastNotifiedVersion = t.getProperty (ids::updLast).toString();
    s.audioPadMasterVolume = (float) (double) t.getProperty (ids::padMaster, 1.0);

    auto list = t.getChildWithName (ids::plugins);
    for (auto pt : list)
    {
        PluginEntryState p;
        p.fileOrId = pt.getProperty (ids::fileId);
        p.bypassed = pt.getProperty (ids::byp, false);
        p.state.fromBase64Encoding (pt.getProperty (ids::blob).toString());
        s.plugins.add (p);
    }

    auto padList = t.getChildWithName (ids::pads);
    for (auto pt : padList)
    {
        AudioPadState p;
        p.name = pt.getProperty (ids::padName).toString();
        p.filePath = pt.getProperty (ids::padFile).toString();
        p.volume = (float) (double) pt.getProperty (ids::padVolume, 0.85);
        p.loop = pt.getProperty (ids::padLoop, false);
        p.route = (AudioPadRoute) juce::jlimit (0, 2, (int) pt.getProperty (ids::padRoute, 0));
        p.retrigger = (AudioPadRetrigger) juce::jlimit (0, 2, (int) pt.getProperty (ids::padRetrigger, 0));
        p.hotkey = pt.getProperty (ids::padHotkey).toString();
        p.fadeInMs = (float) (double) pt.getProperty (ids::padFadeIn, 0.0);
        p.fadeOutMs = (float) (double) pt.getProperty (ids::padFadeOut, 10.0);
        p.colourARGB = (juce::uint32) (juce::int64) pt.getProperty (ids::padColour, (juce::int64) 0);
        s.audioPads.add (p);
    }
    return s;
}

juce::File configFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
              .getChildFile ("MicVST").getChildFile ("config.xml");
}

bool saveState (const MicVSTState& s)
{
    auto f = configFile();
    f.getParentDirectory().createDirectory();
    if (auto xml = toValueTree (s).createXml())
        return xml->writeTo (f);
    return false;
}

MicVSTState loadState()
{
    auto f = configFile();
    if (! f.existsAsFile()) return {};
    if (auto xml = juce::XmlDocument::parse (f))
        return fromValueTree (juce::ValueTree::fromXml (*xml));
    return {};
}
