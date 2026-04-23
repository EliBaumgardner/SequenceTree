//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_DISPLAYBUTTON_H
#define SEQUENCETREE_DISPLAYBUTTON_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../CustomLookAndFeel.h"
#include "../../Util/PluginContext.h"

class DisplayButton : public juce::Component, public juce::SettableTooltipClient {

    public:

    bool isSelected = false;
    std::function<void()> onClick;

    DisplayButton() { setLookAndFeel(ComponentContext::lookAndFeel); setTooltip("Display Options"); };

    void paint(juce::Graphics& g) override
    {
        CustomLookAndFeel::get(*this).drawDisplayButton(g, *this);
    }

    void mouseDown(const juce::MouseEvent& e) override { onClick(); }
};

#endif //SEQUENCETREE_DISPLAYBUTTON_H
