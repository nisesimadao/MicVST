#include "ui/PluginPicker.h"

PluginPickerComponent::PluginPickerComponent (juce::Array<juce::PluginDescription> plugins,
                                              std::function<void (const juce::PluginDescription&)> onChosenIn)
    : all (std::move (plugins)), onChosen (std::move (onChosenIn))
{
    searchBox.setTextToShowWhenEmpty ("Search plugins...", juce::Colours::grey);
    searchBox.setFont (juce::Font (juce::FontOptions (14.0f)));
    searchBox.onTextChange = [this] { rebuildItems(); };
    searchBox.addKeyListener (this);
    addAndMakeVisible (searchBox);

    list.setRowHeight (22);
    list.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff1e1e1e));
    addAndMakeVisible (list);

    rebuildItems();
    setSize (320, 420);
}

void PluginPickerComponent::parentHierarchyChanged()
{
    if (isShowing())
        searchBox.grabKeyboardFocus();
}

void PluginPickerComponent::resized()
{
    auto r = getLocalBounds().reduced (6);
    searchBox.setBounds (r.removeFromTop (26));
    r.removeFromTop (6);
    list.setBounds (r);
}

void PluginPickerComponent::rebuildItems()
{
    filtered = filterPlugins (all, searchBox.getText());
    items.clear();

    const bool grouped = searchBox.getText().trim().isEmpty();
    juce::String lastManu ("\x01");   // unmöglicher Startwert, damit auch "" eine Kopfzeile bekommt
    for (int i = 0; i < filtered.size(); ++i)
    {
        const auto& d = filtered.getReference (i);
        if (grouped && d.manufacturerName != lastManu)
        {
            lastManu = d.manufacturerName;
            items.push_back ({ true, lastManu.isEmpty() ? "(unknown)" : lastManu, -1 });
        }
        items.push_back ({ false, grouped ? d.name : d.name + "  -  " + d.manufacturerName, i });
    }

    list.updateContent();
    list.selectRow (firstSelectableRow());
    list.scrollToEnsureRowIsOnscreen (list.getSelectedRow());
    list.repaint();
}

int PluginPickerComponent::firstSelectableRow() const
{
    for (int i = 0; i < (int) items.size(); ++i)
        if (! items[(size_t) i].isHeader) return i;
    return -1;
}

void PluginPickerComponent::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (! juce::isPositiveAndBelow (row, (int) items.size())) return;
    const auto& it = items[(size_t) row];

    if (it.isHeader)
    {
        g.setColour (juce::Colours::grey);
        g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        g.drawText (it.text, 6, 0, w - 12, h, juce::Justification::centredLeft);
        return;
    }
    if (selected)
    {
        g.setColour (juce::Colour (0xff2a4a7a));
        g.fillRect (0, 0, w, h);
    }
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (14.0f)));
    g.drawText (it.text, 18, 0, w - 24, h, juce::Justification::centredLeft);
}

void PluginPickerComponent::chooseRow (int row)
{
    if (! juce::isPositiveAndBelow (row, (int) items.size())) return;
    const auto& it = items[(size_t) row];
    if (it.isHeader || ! juce::isPositiveAndBelow (it.filteredIndex, filtered.size())) return;
    const auto chosen = filtered[it.filteredIndex];   // Kopie: dismiss zerstört uns gleich
    dismiss();
    if (onChosen) onChosen (chosen);
}

void PluginPickerComponent::moveSelection (int delta)
{
    const int n = (int) items.size();
    int row = list.getSelectedRow();
    for (int step = 0; step < n; ++step)   // Kopfzeilen überspringen, an den Enden stoppen
    {
        row += delta;
        if (! juce::isPositiveAndBelow (row, n)) return;
        if (! items[(size_t) row].isHeader)
        {
            list.selectRow (row);
            list.scrollToEnsureRowIsOnscreen (row);
            return;
        }
    }
}

void PluginPickerComponent::dismiss()
{
    if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
        box->dismiss();
}

bool PluginPickerComponent::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    if (key == juce::KeyPress::downKey)   { moveSelection (1);  return true; }
    if (key == juce::KeyPress::upKey)     { moveSelection (-1); return true; }
    if (key == juce::KeyPress::returnKey) { chooseRow (list.getSelectedRow()); return true; }
    if (key == juce::KeyPress::escapeKey) { dismiss(); return true; }
    return false;   // alles andere tippt ins Suchfeld
}
