//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_DISPLAYMENU_H
#define SEQUENCETREE_DISPLAYMENU_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../CustomLookAndFeel.h"
#include "../../Util/PluginContext.h"
#include "../Node/NodeCanvas.h"
#include "../CustomTextEditor.h"
#include "DisplayButton.h"
#include "ButtonConstants.h"

class DisplayMenu : public juce::Component {

    public:

    DisplayButton button;
    CustomTextEditor display;
    juce::PopupMenu menu;
    juce::String selectedOption = "";

    DisplayMenu()
    {
        setLookAndFeel(ComponentContext::lookAndFeel);
        addAndMakeVisible(display);
        addAndMakeVisible(button);

        menu.addItem(1, "show pitch");
        menu.addItem(2, "show velocity");
        menu.addItem(3, "show duration");
        menu.addItem(4, "show countLimit");

        button.onClick = [this]() {
            button.isSelected = true;
            repaint();
            menu.showMenuAsync (juce::PopupMenu::Options(), [this] (int result)
            {
                button.isSelected = false;
                repaint();
                switch (result)
                {
                    case 1: selectedOption = "show pitch";      ComponentContext::canvas->setSelectionMode(NodeDisplayMode::Pitch); break;
                    case 2: selectedOption = "show velocity";   ComponentContext::canvas->setSelectionMode(NodeDisplayMode::Velocity); break;
                    case 3: selectedOption = "show duration";   ComponentContext::canvas->setSelectionMode(NodeDisplayMode::Duration); break;
                    case 4: selectedOption = "show countLimit"; ComponentContext::canvas->setSelectionMode(NodeDisplayMode::CountLimit); break;
                    default: break;
                }
                resized();
            });
        };
    }

    void paint(juce::Graphics& g) override
    {
        if(auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawDisplayMenu(g,*this); };
    }

    void resized() override
    {
        auto contentBounds = getLocalBounds().reduced(buttonContentBounds);
        auto displayWidth = contentBounds.getWidth() * 2/3;

        display.setBounds(contentBounds.removeFromLeft(displayWidth));
        display.setText(selectedOption);
        display.refit();

        button.setBounds(contentBounds);
    }
};

#endif //SEQUENCETREE_DISPLAYMENU_H
