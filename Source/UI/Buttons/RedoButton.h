//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_REDOBUTTON_H
#define SEQUENCETREE_REDOBUTTON_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../CustomLookAndFeel.h"
#include "../../Util/PluginContext.h"

class RedoButton : public juce::Component {

public:

    bool isDown = false;
    bool isHovered = false;

    std::function<void()> onClick;
    RedoButton() { setLookAndFeel(ComponentContext::lookAndFeel); }

    void paint(juce::Graphics& g) override
    {
        CustomLookAndFeel::get(*this).drawRedoButton(g, *this, isDown);
    }

    void mouseEnter(const juce::MouseEvent&) override { isHovered = true;  repaint(); }
    void mouseExit (const juce::MouseEvent&) override { isHovered = false; repaint(); }

    void mouseDown(const juce::MouseEvent &event) override
    {
        isDown = true;
        onClick();
        repaint();
    }

    void mouseUp(const juce::MouseEvent &event) override {
        isDown = false;
        repaint();
    };

};

#endif //SEQUENCETREE_REDOBUTTON_H
