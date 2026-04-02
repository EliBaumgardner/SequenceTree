//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_BUTTONPANE_H
#define SEQUENCETREE_BUTTONPANE_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../CustomLookAndFeel.h"
#include "../../Util/PluginContext.h"
#include "../../Logic/NodeController.h"
#include "NodeButton.h"
#include "ConnectorButton.h"

class ButtonPane : public juce::Component {

    public:

    NodeButton nodeButton;
    ConnectorButton connectorButton;

    ButtonPane()
    {
        setLookAndFeel(ComponentContext::lookAndFeel);
        addAndMakeVisible(nodeButton);
        addAndMakeVisible(connectorButton);

        NodeController& nodeController = *ComponentContext::nodeController;

        nodeButton.onClick = [this,&nodeController]() {

            jassert(&nodeController);

            nodeButton.isSelected = true;
            nodeButton.repaint();

            connectorButton.isSelected = false;
            connectorButton.repaint();

            nodeController.nodeControllerMode = NodeController::NodeControllerMode::Node;
        };

        connectorButton.onClick = [this, &nodeController]() {

            jassert(&nodeController);

            connectorButton.isSelected = true;
            connectorButton.repaint();

            nodeButton.isSelected = false;
            nodeButton.repaint();

            nodeController.nodeControllerMode = NodeController::NodeControllerMode::Connector;
        };
    }

    void paint(juce::Graphics& g) override
    {
        if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawButtonPane(g, *this);}
    }

    void resized () override
    {
        auto bounds = getLocalBounds().reduced(2.0f);
        int buttonSize = bounds.getHeight();
        int numButtons = 2;
        float totalButtonWidth = buttonSize * numButtons;

        float spacing = (bounds.getWidth() - totalButtonWidth) / (numButtons + 1);

        int x = static_cast<int>(bounds.getX() + spacing);
        nodeButton.setBounds(x, bounds.getY(), buttonSize, buttonSize);

        x += buttonSize + spacing;
        connectorButton.setBounds(x, bounds.getY(), buttonSize, buttonSize);
    }
};

#endif //SEQUENCETREE_BUTTONPANE_H
