//
// Created by Eli Baumgardner on 4/6/26.
//

#ifndef SEQUENCETREE_RESETBUTTON_H
#define SEQUENCETREE_RESETBUTTON_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"

class ResetButton : public juce::Component, public juce::SettableTooltipClient {

    bool isDown = false;

public:

    bool isHovered = false;
    std::function<void()> onClick;
    ResetButton(ApplicationContext& context) { setLookAndFeel(context.lookAndFeel); setTooltip("Reset"); }

    void paint(juce::Graphics& g) override
    {
        CustomLookAndFeel::get(*this).drawResetButton(g, *this, isDown);
    }

    void mouseEnter(const juce::MouseEvent&) override { isHovered = true;  repaint(); }
    void mouseExit (const juce::MouseEvent&) override { isHovered = false; repaint(); }

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