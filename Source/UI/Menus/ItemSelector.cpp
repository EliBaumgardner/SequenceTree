#include "ItemSelector.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"

ItemSelector::ItemSelector(const ApplicationContext& context)
    : labelEditor(context),
      applicationContext(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    button.icon = &CustomLookAndFeel::drawDisplayArrowIcon;

    button.setLookAndFeel(context.lookAndFeel);

    labelEditor.autoFitText = true;
    labelEditor.editable    = false;

    labelEditor.setFormat(std::make_unique<TextFormat>(TextFormat::labelTextLength, TextFormat::labelCharacters));
    labelEditor.setInterceptsMouseClicks(false, false);

    button.setTooltip("Display Options");

    button.onClick = [this]() { showMenu(); };

    addAndMakeVisible(button);
    addAndMakeVisible(labelEditor);
}

void ItemSelector::paint(juce::Graphics& graphics)
{
    const Theme& theme  = CustomLookAndFeel::get(*this);
    const auto   bounds = getLocalBounds().toFloat().reduced(Theme::outerButtonBoundsReduction);

    graphics.setColour(theme.buttonBarColour);
    graphics.fillRoundedRectangle(bounds, Theme::paneCornerRadius);
}

void ItemSelector::resized()
{
    auto       contentBounds = getLocalBounds().reduced(juce::roundToInt(getHeight() * selectorInsetRatio));
    const auto displayWidth  = juce::roundToInt(contentBounds.getWidth() * labelWidthRatio);

    labelEditor.setBounds(contentBounds.removeFromLeft(displayWidth));
    labelEditor.commitText(selectedLabel);

    button.setBounds(contentBounds);
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

void ItemSelector::showMenu()
{
    ContextMenu menu(applicationContext);

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
