//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_CONNECTORBUTTON_H
#define SEQUENCETREE_CONNECTORBUTTON_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"

class ConnectorButton : public juce::Component {
    public:

    std::function<void()> onClick;
    bool isSelected = false;

    ConnectorButton(ApplicationContext& context) { setLookAndFeel(context.lookAndFeel); }
    void paint(juce::Graphics& g) override
    {
        CustomLookAndFeel::get(*this).drawTraverserButton(g, *this);
    }

    void mouseDown(const juce::MouseEvent& e) override { onClick(); }
};

#endif //SEQUENCETREE_CONNECTORBUTTON_H
