//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_ITEMSELECTOR_H
#define SEQUENCETREE_ITEMSELECTOR_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

#include "../Theme/CustomTextEditor.h"
#include "../Buttons/IconButton.h"

struct ApplicationContext;

class ItemSelector : public juce::Component, public juce::SettableTooltipClient {

public:

    using Action = std::function<void()>;

    explicit ItemSelector(ApplicationContext& context);

    void addItem(int itemId, juce::String label, Action onChosen = nullptr);
    void removeItem(int itemId);
    void clearItems();

    void setSelectedItem(int itemId);

    int  getSelectedItemId() const { return selectedItemId; }

    const juce::String& getSelectedLabel() const { return selectedLabel; }

    std::function<void(int)> onItemSelected;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:

    struct Item {
        int          id;
        juce::String label;
        Action       action;
    };

    void showMenu();
    void handleResult(int itemId);

    const Item* findItem(int itemId) const;

    ApplicationContext& applicationContext;

    std::unique_ptr<IconButton> button;
    CustomTextEditor display;

    std::vector<Item> items;

    int          selectedItemId = 0;
    juce::String selectedLabel;
};

#endif //SEQUENCETREE_ITEMSELECTOR_H
