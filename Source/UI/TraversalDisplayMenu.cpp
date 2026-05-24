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

            resized();
        });
    };
}

void TraversalDisplayMenu::paint(juce::Graphics& g)
{
    CustomLookAndFeel::get(*this).drawTraversalDisplayMenu(g, *this);
}

void TraversalDisplayMenu::addTraversalToMenu(int traversalId)
{
    juce::String itemName = "Traversal " + juce::String(traversalId);
    menu.addItem(traversalId, itemName);
}