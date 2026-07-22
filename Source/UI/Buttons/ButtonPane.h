//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_BUTTONPANE_H
#define SEQUENCETREE_BUTTONPANE_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"
#include "IconButton.h"

class ButtonPane : public juce::Component {

public:

    explicit ButtonPane(ApplicationContext& context) : applicationContext(context)
    {
        setLookAndFeel(applicationContext.lookAndFeel);
    }

    IconButton& addButton(IconButton::Painter painter, const juce::String& tooltip, std::function<void()> onClick)
    {
        auto* button = buttons.add(new IconButton(std::move(painter), applicationContext.lookAndFeel));

        button->setTooltip(tooltip);

        button->onClick = [this, button, action = std::move(onClick)]() {
            if (toggleSelection) {
                setSelectedButton(button);
            }
            if (action) {
                action();
            }
        };

        addAndMakeVisible(button);
        resized();

        return *button;
    }

    void enableToggleSelection(bool shouldToggle = true)
    {
        toggleSelection = shouldToggle;
    }

    void setSelectedButton(const IconButton* selected)
    {
        for (IconButton* button : buttons) {
            button->setSelected(button == selected);
        }
    }

    void paint(juce::Graphics& g) override
    {
        const Theme& theme = CustomLookAndFeel::get(*this);
        auto bounds = getLocalBounds().reduced(Theme::outerButtonBoundsReduction).toFloat();

        g.setColour(theme.buttonBarColour);
        g.fillRoundedRectangle(bounds, Theme::paneCornerRadius);
    }

    void resized() override
    {
        const int numButtons = buttons.size();

        if (numButtons == 0) {
            return;
        }

        auto bounds = getLocalBounds().reduced(2.0f);

        int   buttonSize       = bounds.getHeight();
        float totalButtonWidth = buttonSize * numButtons;
        float spacing          = (bounds.getWidth() - totalButtonWidth) / (numButtons + 1);

        int x = static_cast<int>(bounds.getX() + spacing);

        for (IconButton* button : buttons) {
            button->setBounds(x, bounds.getY(), buttonSize, buttonSize);
            x += buttonSize + spacing;
        }
    }

private:

    ApplicationContext& applicationContext;

    juce::OwnedArray<IconButton> buttons;

    bool toggleSelection = false;
};

#endif //SEQUENCETREE_BUTTONPANE_H
