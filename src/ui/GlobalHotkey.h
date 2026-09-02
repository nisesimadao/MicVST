#pragma once
#include <juce_core/juce_core.h>

// Polls a compact hotkey description such as "F8", "Ctrl+Shift+1" or "Alt+Q".
// Windows uses GetAsyncKeyState so pads still work while MicVST is hidden in the tray.
bool isGlobalHotkeyDown (const juce::String& description);
