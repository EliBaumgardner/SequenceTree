//
// Created by Eli Baumgardner on 11/9/25.
//

#include "ItemSelector.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"

#include <algorithm>

ItemSelector::ItemSelector(ApplicationContext& context)
    : applicationContext(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    button = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawDisplayArrowIcon(g, bounds, state);
        }, context.lookAndFeel);

    button->setTooltip("Display Options");
    button->onClick = [this]() { showMenu(); };

    labelEditor = std::make_unique<ValueEditor>(context);
    labelEditor->setFormat(std::make_unique<TextFormat>(TextFormat::labelTextLength,
                                                        TextFormat::labelCharacters));
    labelEditor->autoFitText = true;
    labelEditor->editable = false;
    labelEditor->setInterceptsMouseClicks(false, false);

    addAndMakeVisible(button.get());
    addAndMakeVisible(labelEditor.get());
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
    const Item* const item = findItem(itemId);
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
    menu.setLookAndFeel(applicationContext.lookAndFeel);

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

    const Item* const item = findItem(itemId);
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
    const auto bounds = getLocalBounds().toFloat().reduced(Theme::outerButtonBoundsReduction);
    g.setColour(theme.buttonBarColour);
    g.fillRoundedRectangle(bounds, Theme::paneCornerRadius);
}

void ItemSelector::resized()
{
    auto contentBounds = getLocalBounds().reduced(juce::roundToInt(getHeight() * contentInsetRatio));
    const auto displayWidth = juce::roundToInt(contentBounds.getWidth() * labelWidthRatio);

    labelEditor->setBounds(contentBounds.removeFromLeft(displayWidth));
    labelEditor->commitText(selectedLabel);

    button->setBounds(contentBounds);
}
