//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_NODEBUTTON_H
#define SEQUENCETREE_NODEBUTTON_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../CustomLookAndFeel.h"
#include "../../Util/PluginContext.h"

class NodeButton : public juce::Component {

    public:

    std::function<void()> onClick;
    bool isSelected = false;

    NodeButton() { setLookAndFeel(ComponentContext::lookAndFeel); }

    void paint (juce::Graphics &g) override
    {
        CustomLookAndFeel::get(*this).drawNodeButton(g, *this);
    }

    void mouseDown(const juce::MouseEvent& e) override { onClick(); }
};

#endif //SEQUENCETREE_NODEBUTTON_H
