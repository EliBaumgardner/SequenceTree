#include "ItemSelector.h"
#include "../Theme/CustomLookAndFeel.h"

ItemSelector::ItemSelector(juce::UndoManager& undoManager)
    : labelEditor(undoManager)
{
    button.icon = &CustomLookAndFeel::drawDisplayArrowIcon;

    labelEditor.autoFitText = true;
    labelEditor.editable    = false;
    labelEditor.fontStyle   = Theme::FontStyle::Regular;

    labelEditor.setFormat(std::make_unique<TextFormat>(TextFormat::labelTextLength, TextFormat::labelCharacters));
    labelEditor.setInterceptsMouseClicks(false, false);
    labelEditor.setJustification(juce::Justification::centredLeft);

    button.setTooltip("Display Options");

    button.onClick = [this]() { showMenu(); };

    addAndMakeVisible(button);
    addAndMakeVisible(labelEditor);
}

void ItemSelector::paint(juce::Graphics& graphics)
{
    CustomLookAndFeel& lookAndFeel   = CustomLookAndFeel::get(*this);
    const auto         contentBounds = getLocalBounds().reduced(juce::roundToInt(getHeight() * selectorInsetRatio));

    lookAndFeel.drawPane(graphics, getLocalBounds().toFloat());

    if (leadingIcon != nullptr) {
        (lookAndFeel.*leadingIcon)(graphics, contentBounds.withWidth(contentBounds.getHeight()).toFloat(), ButtonState {});
    }
}

void ItemSelector::resized()
{
    auto contentBounds = getLocalBounds().reduced(juce::roundToInt(getHeight() * selectorInsetRatio));

    if (leadingIcon != nullptr) {
        contentBounds.removeFromLeft(contentBounds.getHeight());
    }

    button.setBounds(contentBounds.removeFromRight(contentBounds.getHeight()));

    labelEditor.setBounds(contentBounds);
    labelEditor.commitText(selectedLabel);
}

void ItemSelector::showMenu()
{
    ContextMenu menu;

    button.setSelected(true);

    repaint();

    for (const Item& item : items) {
        menu.addItem(item.label, ContextMenu::ItemKind::Action, [this, itemId = item.id]() { handleResult(itemId); });
    }

    menu.onDismissed = [this]() {
        button.setSelected(false);

        repaint();
    };

    menu.show(button);
}

void ItemSelector::handleResult(int itemId)
{
    const Item* const item = findItem(itemId);

    if (itemId == 0 || item == nullptr) {
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

const ItemSelector::Item* ItemSelector::findItem(int itemId) const
{
    for (const Item& item : items) {
        if (item.id == itemId) {
            return &item;
        }
    }

    return nullptr;
}

void ItemSelector::addItem(int itemId, juce::String label, Action onChosen)
{
    items.push_back({ itemId, std::move(label), std::move(onChosen) });
}

void ItemSelector::removeItem(int itemId)
{
    std::erase_if(items, [itemId](const Item& item) { return item.id == itemId; });

    if (selectedItemId == itemId) {
        selectedItemId = 0;

        selectedLabel.clear();

        resized();
    }
}

void ItemSelector::clearItems()
{
    selectedItemId = 0;

    items.clear();
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
