//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_PLAYBUTTON_H
#define SEQUENCETREE_PLAYBUTTON_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../CustomLookAndFeel.h"
#include "../../Util/PluginContext.h"

class PlayButton : public juce::Button {

public:

    std::function<void()> onClick;

    PlayButton() : juce::Button("button") { setLookAndFeel(ComponentContext::lookAndFeel); }

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override
    {
        CustomLookAndFeel::get(*this).drawPlayButton(g, isMouseOver, isButtonDown, *this);
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        juce::Button::mouseDown(event);
        isOn = !isOn;
        repaint();
        onClick();
    }

    bool isOn = true;
};

#endif //SEQUENCETREE_PLAYBUTTON_H
