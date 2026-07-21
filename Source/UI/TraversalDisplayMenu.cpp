//
// Created by Eli Baumgardner on 5/23/26.
//

#include "TraversalDisplayMenu.h"
#include "../Util/ApplicationContext.h"
#include "Theme/CustomLookAndFeel.h"

TraversalDisplayMenu::TraversalDisplayMenu(ApplicationContext& context)
    : DisplayMenu(context), applicationContext(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    menu.clear();

    button.onClick = [this]() {

        button.isSelected = true;
        repaint();

        menu.showMenuAsync (juce::PopupMenu::Options(), [this] (int result)
        {
            button.isSelected = false;
            repaint();

            juce::PopupMenu::MenuItemIterator it(menu);

            while (it.next()) {
                if (it.getItem().itemID == result) {
                    selectedOption = it.getItem().text;
                }
            }

            if (result != 0 && onTraversalSelected) {
                onTraversalSelected(result);
            }

            resized();
        });
    };
}

void TraversalDisplayMenu::paint(juce::Graphics& g)
{
    const Theme& theme = CustomLookAndFeel::get(*this);
    auto bounds = getLocalBounds().toFloat().reduced(Theme::outerButtonBoundsReduction);
    g.setColour(theme.buttonBarColour);
    g.fillRoundedRectangle(bounds, Theme::paneCornerRadius);
}

void TraversalDisplayMenu::addTraversalToMenu(int traversalId)
{
    DBG("adding traversal to menu");
    juce::String itemName = "Traversal " + juce::String(traversalId);
    menu.addItem(traversalId, itemName);
}

void TraversalDisplayMenu::removeTraversalFromMenu(int traversalId) {

    juce::String itemName = "Traversal " + juce::String(traversalId);

    juce::PopupMenu remaining;
    juce::PopupMenu::MenuItemIterator it(menu);

    while (it.next()) {
        if (it.getItem().itemID != traversalId)
            remaining.addItem(it.getItem());
    }

    menu = std::move(remaining);

    if (selectedOption == itemName) {
        selectedOption = "";
        resized();
    }
}