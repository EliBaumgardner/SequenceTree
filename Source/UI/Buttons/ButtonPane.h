//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_BUTTONPANE_H
#define SEQUENCETREE_BUTTONPANE_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"
#include "../../Input/NodeController.h"
#include "NodeButton.h"
#include "ModulatorButton.h"
#include "TraversalFlagButton.h"


class ButtonPane : public juce::Component {

    ApplicationContext& applicationContext;

    public:

    NodeButton nodeButton;
    ModulatorButton modulatorButton;
    TraversalFlagButton traversalFlagButton;

    ButtonPane(ApplicationContext& context)
        : applicationContext(context), nodeButton(context), modulatorButton(context), traversalFlagButton(context)
    {
        setLookAndFeel(applicationContext.lookAndFeel);
        addAndMakeVisible(nodeButton);
        addAndMakeVisible(modulatorButton);
        addAndMakeVisible(traversalFlagButton);

        NodeController& nodeController = *applicationContext.nodeController;

        nodeButton.onClick = [this, &nodeController]() {

            jassert(&nodeController);

            nodeButton.isSelected = true;
            nodeButton.repaint();

            modulatorButton.isSelected = false;
            modulatorButton.repaint();

            traversalFlagButton.isSelected = false;
            traversalFlagButton.repaint();

            nodeController.nodeControllerMode = NodeController::NodeControllerMode::Node;
        };

        modulatorButton.onClick = [this, &nodeController]() {

            jassert(&nodeController);

            modulatorButton.isSelected = true;
            modulatorButton.repaint();

            nodeButton.isSelected = false;
            nodeButton.repaint();

            traversalFlagButton.isSelected = false;
            traversalFlagButton.repaint();

            nodeController.nodeControllerMode = NodeController::NodeControllerMode::Modulator;
        };

        traversalFlagButton.onClick = [this, &nodeController]() {

            jassert(&nodeController);

            traversalFlagButton.isSelected = true;
            traversalFlagButton.repaint();

            nodeButton.isSelected = false;
            nodeButton.repaint();

            modulatorButton.isSelected = false;
            modulatorButton.repaint();

            nodeController.nodeControllerMode = NodeController::NodeControllerMode::TraversalFlag;
        };
    }

    void paint(juce::Graphics& g) override
    {
        CustomLookAndFeel::get(*this).drawButtonPane(g, *this);
    }

    void resized () override
    {
        auto bounds = getLocalBounds().reduced(2.0f);
        int buttonSize = bounds.getHeight();
        int numButtons = 3;
        float totalButtonWidth = buttonSize * numButtons;

        float spacing = (bounds.getWidth() - totalButtonWidth) / (numButtons + 1);

        int x = static_cast<int>(bounds.getX() + spacing);
        nodeButton.setBounds(x, bounds.getY(), buttonSize, buttonSize);

        x += buttonSize + spacing;
        modulatorButton.setBounds(x, bounds.getY(), buttonSize,buttonSize);

        x += buttonSize + spacing;
        traversalFlagButton.setBounds(x, bounds.getY(), buttonSize, buttonSize);
    }
};

#endif //SEQUENCETREE_BUTTONPANE_H
