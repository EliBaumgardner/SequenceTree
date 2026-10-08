#include "ButtonPane.h"
#include "../Theme/CustomLookAndFeel.h"

void ButtonPane::paint(juce::Graphics& graphics)
{
    CustomLookAndFeel::get(*this).drawPane(graphics, getLocalBounds().toFloat());
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

    const int   buttonCount = buttons.size();
    const auto  bounds      = getLocalBounds().reduced(juce::roundToInt(getHeight() * Theme::contentInsetRatio));
    const float buttonWidth = juce::jmax(0.0f, (bounds.getWidth() - Theme::buttonGap * (buttonCount - 1)) / static_cast<float>(buttonCount));
    float       buttonX     = static_cast<float>(bounds.getX());

    for (IconButton* button : buttons) {
        button->setBounds(juce::roundToInt(buttonX), bounds.getY(), juce::roundToInt(buttonWidth), bounds.getHeight());

        buttonX += buttonWidth + Theme::buttonGap;
    }
}

IconButton& ButtonPane::addButton(IconButton::Icon icon, const juce::String& tooltip, std::function<void()> onClick)
{
    auto* const button = buttons.add(new IconButton());

    button->icon = icon;

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

int ButtonPane::idealWidth(int height) const
{
    const int inset       = juce::roundToInt(height * Theme::contentInsetRatio);
    const int buttonWidth = juce::roundToInt((height - inset * 2) * Theme::buttonAspectRatio);

    return inset * 2 + buttons.size() * buttonWidth + juce::jmax(0, buttons.size() - 1) * Theme::buttonGap;
}
