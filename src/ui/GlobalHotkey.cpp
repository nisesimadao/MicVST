#include "ui/GlobalHotkey.h"

#if JUCE_WINDOWS
 #define WIN32_LEAN_AND_MEAN
 #define NOMINMAX
 #include <windows.h>
#endif

namespace
{
   #if JUCE_WINDOWS
    bool down (int vk) { return (GetAsyncKeyState (vk) & 0x8000) != 0; }

    int keyToVk (juce::String key)
    {
        key = key.trim().toUpperCase();
        if (key.length() == 1)
        {
            const auto c = key[0];
            if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) return (int) c;
        }
        if (key.startsWith ("F"))
        {
            const int n = key.substring (1).getIntValue();
            if (n >= 1 && n <= 24) return VK_F1 + n - 1;
        }
        if (key.startsWithIgnoreCase ("NUMPAD"))
        {
            const int n = key.substring (6).getIntValue();
            if (n >= 0 && n <= 9) return VK_NUMPAD0 + n;
        }
        if (key == "SPACE") return VK_SPACE;
        if (key == "INSERT" || key == "INS") return VK_INSERT;
        if (key == "DELETE" || key == "DEL") return VK_DELETE;
        if (key == "HOME") return VK_HOME;
        if (key == "END") return VK_END;
        if (key == "PAGEUP" || key == "PGUP") return VK_PRIOR;
        if (key == "PAGEDOWN" || key == "PGDN") return VK_NEXT;
        return 0;
    }
   #endif
}

bool isGlobalHotkeyDown (const juce::String& description)
{
   #if JUCE_WINDOWS
    if (description.trim().isEmpty()) return false;
    auto parts = juce::StringArray::fromTokens (description, "+", "");
    parts.trim();
    parts.removeEmptyStrings();
    if (parts.isEmpty()) return false;

    bool needCtrl = false, needAlt = false, needShift = false;
    juce::String key;
    for (auto& part : parts)
    {
        if (part.equalsIgnoreCase ("CTRL") || part.equalsIgnoreCase ("CONTROL")) needCtrl = true;
        else if (part.equalsIgnoreCase ("ALT")) needAlt = true;
        else if (part.equalsIgnoreCase ("SHIFT")) needShift = true;
        else key = part;
    }

    const int vk = keyToVk (key);
    if (vk == 0) return false;
    if (needCtrl && ! down (VK_CONTROL)) return false;
    if (needAlt && ! down (VK_MENU)) return false;
    if (needShift && ! down (VK_SHIFT)) return false;
    return down (vk);
   #else
    (void) description;
    return false;
   #endif
}
