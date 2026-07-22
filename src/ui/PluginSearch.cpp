#include "ui/PluginSearch.h"
#include <algorithm>

juce::Array<juce::PluginDescription> filterPlugins (const juce::Array<juce::PluginDescription>& all,
                                                    const juce::String& query)
{
    const auto q = query.trim();
    juce::Array<juce::PluginDescription> out;
    for (auto& d : all)
        if (q.isEmpty() || d.name.containsIgnoreCase (q) || d.manufacturerName.containsIgnoreCase (q))
            out.add (d);

    std::sort (out.begin(), out.end(),
               [] (const juce::PluginDescription& a, const juce::PluginDescription& b)
    {
        const int m = a.manufacturerName.compareIgnoreCase (b.manufacturerName);
        if (m != 0) return m < 0;
        return a.name.compareIgnoreCase (b.name) < 0;
    });
    return out;
}
