#include "state/PluginScanCache.h"

namespace PluginScanCache
{

std::unique_ptr<juce::XmlElement> toXml (const juce::KnownPluginList& list,
                                         const juce::Array<SkippedPlugin>& skipped)
{
    auto root = std::make_unique<juce::XmlElement> ("MicVSTPluginCache");
    if (auto types = list.createXml())
        root->addChildElement (types.release());

    auto* skips = root->createNewChildElement ("SKIPPED");
    for (auto& s : skipped)
    {
        auto* e = skips->createNewChildElement ("PLUGIN");
        e->setAttribute ("file", s.file);
        e->setAttribute ("reason", s.reason);
        e->setAttribute ("fileTimeMs", juce::String (s.fileTimeMs));
    }
    return root;
}

bool fromXml (const juce::XmlElement& xml, juce::KnownPluginList& list,
              juce::Array<SkippedPlugin>& skipped)
{
    if (! xml.hasTagName ("MicVSTPluginCache")) return false;

    if (auto* types = xml.getChildByName ("KNOWNPLUGINS"))
        list.recreateFromXml (*types);

    if (auto* skips = xml.getChildByName ("SKIPPED"))
        for (auto* e : skips->getChildWithTagNameIterator ("PLUGIN"))
            skipped.add ({ e->getStringAttribute ("file"),
                           e->getStringAttribute ("reason"),
                           e->getStringAttribute ("fileTimeMs").getLargeIntValue() });
    return true;
}

bool save (const juce::File& file, const juce::KnownPluginList& list,
           const juce::Array<SkippedPlugin>& skipped)
{
    file.getParentDirectory().createDirectory();
    return toXml (list, skipped)->writeTo (file);
}

bool load (const juce::File& file, juce::KnownPluginList& list,
           juce::Array<SkippedPlugin>& skipped)
{
    if (! file.existsAsFile()) return false;
    auto xml = juce::parseXML (file);
    if (xml == nullptr || ! fromXml (*xml, list, skipped))
    {
        file.deleteFile();   // korrupt -> weg, voller Rescan folgt
        return false;
    }
    return true;
}

} // namespace PluginScanCache
