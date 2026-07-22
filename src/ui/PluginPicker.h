#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/PluginSearch.h"

// Popup-Inhalt für "+ Plugin" (in einer CallOutBox): Suchfeld oben, darunter die Liste.
// Leere Suche: nach Hersteller gruppiert (nicht wählbare Kopfzeilen). Tippen filtert live
// über Name + Hersteller (flache Liste "Name — Hersteller"). Enter/Doppelklick wählt,
// Pfeiltasten navigieren (überspringen Kopfzeilen), Esc schließt.
class PluginPickerComponent : public juce::Component,
                              private juce::ListBoxModel,
                              private juce::KeyListener
{
public:
    PluginPickerComponent (juce::Array<juce::PluginDescription> plugins,
                           std::function<void (const juce::PluginDescription&)> onChosenIn);
    void resized() override;
    void parentHierarchyChanged() override;   // Fokus ins Suchfeld, sobald wir auf dem Desktop sind

private:
    struct Item { bool isHeader; juce::String text; int filteredIndex; };

    void rebuildItems();
    void chooseRow (int row);
    void moveSelection (int delta);
    int  firstSelectableRow() const;
    void dismiss();

    // ListBoxModel
    int  getNumRows() override { return (int) items.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override { chooseRow (row); }
    void returnKeyPressed (int row) override { chooseRow (row); }

    // KeyListener am Suchfeld: Pfeile/Enter/Esc abfangen, Rest tippt normal weiter.
    bool keyPressed (const juce::KeyPress&, juce::Component*) override;

    juce::Array<juce::PluginDescription> all, filtered;
    std::vector<Item> items;
    std::function<void (const juce::PluginDescription&)> onChosen;
    juce::TextEditor searchBox;
    juce::ListBox list { {}, this };
};
