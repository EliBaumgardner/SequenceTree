//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_NODEBUTTON_H
#define SEQUENCETREE_NODEBUTTON_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"

class NodeButton : public juce::Component, public juce::SettableTooltipClient {

    public:

    std::function<void()> onClick;
    bool isSelected = false;

    NodeButton(ApplicationContext& context) { setLookAndFeel(context.lookAndFeel); setTooltip("Node Mode"); }

    void paint (juce::Graphics &g) override
    {
        CustomLookAndFeel::get(*this).drawNodeButton(g, *this);
    }

    void mouseDown(const juce::MouseEvent& e) override { onClick(); }
};

#endif //SEQUENCETREE_NODEBUTTON_H
