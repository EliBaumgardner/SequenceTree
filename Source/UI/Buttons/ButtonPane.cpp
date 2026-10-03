#include "ButtonPane.h"
#include "../Theme/CustomLookAndFeel.h"

ButtonPane::ButtonPane(const ApplicationContext& context) : applicationContext(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);
}

void ButtonPane::paint(juce::Graphics& graphics)
{
    const Theme& theme  = CustomLookAndFeel::get(*this);
    const auto   bounds = getLocalBounds().reduced(Theme::outerButtonBoundsReduction).toFloat();

    graphics.setColour(theme.buttonBarColour);
    graphics.fillRoundedRectangle(bounds, Theme::paneCornerRadius);
}

void ButtonPane::resized()
{
    if (buttons.isEmpty()) {
        return;
    }

    if (gridLayout.has_value()) {
        const Grid& grid   = *gridLayout;
        const auto  bounds = getLocalBounds().reduced(grid.edgeInset);
        int         column = 0;
        int         row    = 0;

        if (bounds.isEmpty()) {
            return;
        }

        const int columnCount = juce::jmax(1, (bounds.getWidth() + grid.gap) / (grid.cellWidth + grid.gap));

        for (IconButton* button : buttons) {
            button->setBounds(bounds.getX() + column * (grid.cellWidth + grid.gap), bounds.getY() + row * (grid.cellHeight + grid.gap),
                              grid.cellWidth, grid.cellHeight);

            if (++column >= columnCount) {
                column = 0;
                ++row;
            }
        }

        return;
    }

    const int   buttonCount    = buttons.size();
    const auto  bounds         = getLocalBounds().reduced(juce::roundToInt(getHeight() * Theme::contentInsetRatio));
    const float widthPerButton = bounds.getWidth() / (buttonCount + (buttonCount + 1) * Theme::iconGapRatio);
    const int   buttonSize     = juce::jmax(0, juce::jmin(bounds.getHeight(), static_cast<int>(widthPerButton)));
    const float spacing        = (bounds.getWidth() - buttonSize * buttonCount) / static_cast<float>(buttonCount + 1);
    const int   buttonY        = bounds.getCentreY() - buttonSize / 2;
    float       buttonX        = bounds.getX() + spacing;

    for (IconButton* button : buttons) {
        button->setBounds(juce::roundToInt(buttonX), buttonY, buttonSize, buttonSize);

        buttonX += buttonSize + spacing;
    }
}

IconButton& ButtonPane::addButton(IconButton::Icon icon, const juce::String& tooltip, std::function<void()> onClick)
{
    auto* const button = buttons.add(new IconButton());

    button->icon = icon;

    button->setLookAndFeel(applicationContext.lookAndFeel);

    button->setTooltip(tooltip);

    button->onClick = [this, button, action = std::move(onClick)]() {
        if (selection == Selection::Momentary) {
            if (action) {
                action();
            }

            return;
        }

        if (selection == Selection::ExclusiveOrNone && button->state.isSelected) {
            setSelectedButton(nullptr);

            return;
        }

        setSelectedButton(button);

        if (action) {
            action();
        }
    };

    addAndMakeVisible(button);

    resized();

    return *button;
}

void ButtonPane::setSelectedButton(const IconButton* selected)
{
    if (selectedButton == selected) {
        return;
    }

    selectedButton = selected;

    for (IconButton* button : buttons) {
        button->setSelected(button == selected);
    }

    if (onSelectionChanged) {
        onSelectionChanged(selected);
    }
}
