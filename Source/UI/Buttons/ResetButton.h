//
// Created by Eli Baumgardner on 4/6/26.
//

#ifndef SEQUENCETREE_RESETBUTTON_H
#define SEQUENCETREE_RESETBUTTON_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../CustomLookAndFeel.h"
#include "../../Util/PluginContext.h"

class ResetButton : public juce::Component {

    bool isDown = false;

public:

    std::function<void()> onClick;
    ResetButton() { setLookAndFeel(ComponentContext::lookAndFeel); }

    void paint(juce::Graphics& g) override
    {
        if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawResetButton(g, *this, isDown); }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        isDown = true;
        repaint();
        onClick();
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        isDown = false;
        repaint();
    }
};

#endif //SEQUENCETREE_RESETBUTTON_H