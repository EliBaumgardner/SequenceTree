//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_BUTTONPANE_H
#define SEQUENCETREE_BUTTONPANE_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <optional>
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"
#include "IconButton.h"

class ButtonPane : public juce::Component {

public:

    struct Grid {
        int cellWidth;
        int cellHeight;
        int gap;
        int edgeInset;
    };

    std::function<void(const IconButton*)> onSelectionChanged;

    explicit ButtonPane(ApplicationContext& context) : applicationContext(context)
    {
        setLookAndFeel(applicationContext.lookAndFeel);
    }

    IconButton& addButton(IconButton::Painter painter, const juce::String& tooltip, std::function<void()> onClick = nullptr)
    {
        auto* const button = buttons.add(new IconButton(std::move(painter), applicationContext.lookAndFeel));

        button->setTooltip(tooltip);

        button->onClick = [this, button, action = std::move(onClick)]() { handleClick(button, action); };

        addAndMakeVisible(button);
        resized();

        return *button;
    }

    void enableToggleSelection(bool shouldToggle = true)
    {
        toggleSelection = shouldToggle;
    }

    void allowEmptySelection(bool shouldAllow = true)
    {
        emptySelectionAllowed = shouldAllow;
    }

    void useGridLayout(Grid layout)
    {
        gridLayout = layout;
        resized();
    }

    void setSelectedButton(const IconButton* selected)
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

    const IconButton* getSelectedButton() const { return selectedButton; }

    void paint(juce::Graphics& g) override
    {
        const Theme& theme = CustomLookAndFeel::get(*this);
        const auto bounds = getLocalBounds().reduced(Theme::outerButtonBoundsReduction).toFloat();

        g.setColour(theme.buttonBarColour);
        g.fillRoundedRectangle(bounds, Theme::paneCornerRadius);
    }

    void resized() override
    {
        if (buttons.isEmpty()) {
            return;
        }

        if (gridLayout.has_value()) {
            layOutAsGrid(*gridLayout);
            return;
        }

        layOutAsRow();
    }

private:

    void layOutAsRow()
    {
        const int  numButtons = buttons.size();
        const auto bounds     = getLocalBounds().reduced(2.0f);

        const int   buttonSize       = bounds.getHeight();
        const float totalButtonWidth = buttonSize * numButtons;
        const float spacing          = (bounds.getWidth() - totalButtonWidth) / (numButtons + 1);

        int x = static_cast<int>(bounds.getX() + spacing);

        for (IconButton* button : buttons) {
            button->setBounds(x, bounds.getY(), buttonSize, buttonSize);
            x += buttonSize + spacing;
        }
    }

    void layOutAsGrid(const Grid& layout)
    {
        const auto bounds = getLocalBounds().reduced(layout.edgeInset);

        if (bounds.isEmpty()) {
            return;
        }

        const int columns = juce::jmax(1, (bounds.getWidth() + layout.gap) / (layout.cellWidth + layout.gap));

        int column = 0;
        int row    = 0;

        for (IconButton* button : buttons) {
            button->setBounds(bounds.getX() + column * (layout.cellWidth  + layout.gap),
                              bounds.getY() + row    * (layout.cellHeight + layout.gap),
                              layout.cellWidth,
                              layout.cellHeight);

            if (++column >= columns) {
                column = 0;
                ++row;
            }
        }
    }

    void handleClick(const IconButton* button, const std::function<void()>& action)
    {
        if (!toggleSelection) {
            if (action) {
                action();
            }
            return;
        }

        const bool shouldDeselect = emptySelectionAllowed && button->isSelected();

        setSelectedButton(shouldDeselect ? nullptr : button);

        if (!shouldDeselect && action) {
            action();
        }
    }

    ApplicationContext& applicationContext;

    juce::OwnedArray<IconButton> buttons;

    const IconButton* selectedButton = nullptr;

    std::optional<Grid> gridLayout;

    bool toggleSelection       = false;
    bool emptySelectionAllowed = false;
};

#endif //SEQUENCETREE_BUTTONPANE_H
