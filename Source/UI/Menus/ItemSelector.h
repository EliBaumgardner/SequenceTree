#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

#include "../Editors/ValueEditor.h"
#include "../Buttons/IconButton.h"
#include "ContextMenu.h"

struct ApplicationContext;

class ItemSelector : public juce::Component, public juce::SettableTooltipClient
{
public:

    using Action = std::function<void()>;

    explicit ItemSelector(const ApplicationContext& context);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void addItem(int itemId, juce::String label, Action onChosen = nullptr);
    void removeItem(int itemId);
    void clearItems();

    void setSelectedItem(int itemId);

    std::function<void(int)> onItemSelected;

    juce::String selectedLabel;

    ValueEditor labelEditor;

    static constexpr float selectorInsetRatio = 0.14f;
    static constexpr float labelWidthRatio    = 2.0f / 3.0f;

private:

    struct Item
    {
        int          id;
        juce::String label;
        Action       action;
    };

    void showMenu();
    void handleResult(int itemId);

    const Item* findItem(int itemId) const;

    const ApplicationContext& applicationContext;

    IconButton button;

    std::vector<Item> items;

    int selectedItemId = 0;
};
