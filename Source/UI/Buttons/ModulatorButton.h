//
// Created by Eli Baumgardner on 4/8/26.
//


#ifndef SEQUENCETREE_MODULATORBUTTON_H
#define SEQUENCETREE_MODULATORBUTTON_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../CustomLookAndFeel.h"
#include "../../Util/PluginContext.h"

class ModulatorButton : public juce::Component, public juce::SettableTooltipClient {

public:

    bool isSelected = false;

    std::function<void()> onClick;

    ModulatorButton() { setLookAndFeel(ComponentContext::lookAndFeel); setTooltip("Modulator Mode"); }

    void paint(juce::Graphics& g) override
    {
        CustomLookAndFeel::get(*this).drawModulatorButton(g, *this);
    }

    void mouseDown(const juce::MouseEvent& e) override { onClick(); }
};

#endif //SEQUENCETREE_MODULATORBUTTON_H