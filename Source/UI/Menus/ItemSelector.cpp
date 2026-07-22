//
// Created by Eli Baumgardner on 11/9/25.
//

#include "ItemSelector.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"
#include "../Buttons/ButtonConstants.h"

#include <algorithm>

ItemSelector::ItemSelector(ApplicationContext& context)
    : applicationContext(context),
      display(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    button = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawDisplayArrowIcon(g, bounds, state);
        }, context.lookAndFeel);

    button->setTooltip("Display Options");
    button->onClick = [this]() { showMenu(); };

    addAndMakeVisible(display);
    addAndMakeVisible(button.get());
}

void ItemSelector::addItem(int itemId, juce::String label, Action onChosen)
{
    items.push_back({ itemId, std::move(label), std::move(onChosen) });
}

void ItemSelector::removeItem(int itemId)
{
    items.erase(std::remove_if(items.begin(), items.end(),
                               [itemId](const Item& item) { return item.id == itemId; }),
                items.end());

    if (selectedItemId == itemId) {
        selectedItemId = 0;
        selectedLabel.clear();
        resized();
    }
}

void ItemSelector::clearItems()
{
    items.clear();
    selectedItemId = 0;
    selectedLabel.clear();
    resized();
}

void ItemSelector::setSelectedItem(int itemId)
{
    const Item* item = findItem(itemId);
    if (item == nullptr) {
        return;
    }

    selectedItemId = item->id;
    selectedLabel  = item->label;

    resized();
    repaint();
}

const ItemSelector::Item* ItemSelector::findItem(int itemId) const
{
    for (const Item& item : items) {
        if (item.id == itemId) {
            return &item;
        }
    }

    return nullptr;
}

void ItemSelector::showMenu()
{
    button->setSelected(true);
    repaint();

    juce::PopupMenu menu;

    for (const Item& item : items) {
        menu.addItem(item.id, item.label);
    }

    menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
        button->setSelected(false);
        repaint();

        handleResult(result);
    });
}

void ItemSelector::handleResult(int itemId)
{
    if (itemId == 0) {
        return;
    }

    const Item* item = findItem(itemId);
    if (item == nullptr) {
        return;
    }

    selectedItemId = item->id;
    selectedLabel  = item->label;

    if (item->action) {
        item->action();
    }

    if (onItemSelected) {
        onItemSelected(itemId);
    }

    resized();
}

void ItemSelector::paint(juce::Graphics& g)
{
    const Theme& theme = CustomLookAndFeel::get(*this);
    auto bounds = getLocalBounds().toFloat().reduced(Theme::outerButtonBoundsReduction);
    g.setColour(theme.buttonBarColour);
    g.fillRoundedRectangle(bounds, Theme::paneCornerRadius);
}

void ItemSelector::resized()
{
    auto contentBounds = getLocalBounds().reduced(buttonContentBounds);
    auto displayWidth = contentBounds.getWidth() * 2/3;

    display.setBounds(contentBounds.removeFromLeft(displayWidth));
    display.setText(selectedLabel);
    display.refit();

    button->setBounds(contentBounds);
}
