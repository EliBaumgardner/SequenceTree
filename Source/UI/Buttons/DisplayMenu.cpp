//
// Created by Eli Baumgardner on 11/9/25.
//

#include "DisplayMenu.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"
#include "../Canvas/NodeCanvas.h"
#include "ButtonConstants.h"

DisplayMenu::DisplayMenu(ApplicationContext& context)
    : applicationContext(context),
      display(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    button = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawDisplayArrowIcon(g, bounds, state);
        }, context.lookAndFeel);

    button->setTooltip("Display Options");
    addAndMakeVisible(display);
    addAndMakeVisible(button.get());

    menu.addItem(1, "show pitch");
    menu.addItem(2, "show velocity");
    menu.addItem(3, "show countLimit");
    menu.addItem(4, "show channel");
    menu.addItem(5, "show repeatValue");

    button->onClick = [this]() {
        button->setSelected(true);
        repaint();
        menu.showMenuAsync (juce::PopupMenu::Options(), [this] (int result)
        {
            button->setSelected(false);
            repaint();
            switch (result)
            {
                case 1: selectedOption = "show pitch";        applicationContext.canvas->nodeManager.setDisplayMode(NodeDisplayMode::Pitch);        applicationContext.currentDisplayMode = NodeDisplayMode::Pitch;        break;
                case 2: selectedOption = "show velocity";     applicationContext.canvas->nodeManager.setDisplayMode(NodeDisplayMode::Velocity);     applicationContext.currentDisplayMode = NodeDisplayMode::Velocity;     break;
                case 3: selectedOption = "show countLimit";   applicationContext.canvas->nodeManager.setDisplayMode(NodeDisplayMode::CountLimit);   applicationContext.currentDisplayMode = NodeDisplayMode::CountLimit;   break;
                case 4: selectedOption = "show channel";      applicationContext.canvas->nodeManager.setDisplayMode(NodeDisplayMode::Channel);      applicationContext.currentDisplayMode = NodeDisplayMode::Channel;      break;
                case 5: selectedOption = "show repeatValue";  applicationContext.canvas->nodeManager.setDisplayMode(NodeDisplayMode::RepeatValue);  applicationContext.currentDisplayMode = NodeDisplayMode::RepeatValue;  break;
                default: break;
            }
            if (result != 0 && applicationContext.onDisplayModeChanged) {
                applicationContext.onDisplayModeChanged(applicationContext.currentDisplayMode);
            }
            resized();
        });
    };
}

void DisplayMenu::paint(juce::Graphics& g)
{
    const Theme& theme = CustomLookAndFeel::get(*this);
    auto bounds = getLocalBounds().toFloat().reduced(Theme::outerButtonBoundsReduction);
    g.setColour(theme.buttonBarColour);
    g.fillRoundedRectangle(bounds, Theme::paneCornerRadius);
}

void DisplayMenu::resized()
{
    auto contentBounds = getLocalBounds().reduced(buttonContentBounds);
    auto displayWidth = contentBounds.getWidth() * 2/3;

    display.setBounds(contentBounds.removeFromLeft(displayWidth));
    display.setText(selectedOption);
    display.refit();

    button->setBounds(contentBounds);
}
