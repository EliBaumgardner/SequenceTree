//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_UNDOBUTTON_H
#define SEQUENCETREE_UNDOBUTTON_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"

class UndoButton : public juce::Component, public juce::SettableTooltipClient {

    bool isDown = false;
    public:

    bool isHovered = false;
    std::function<void()> onClick;
    UndoButton(ApplicationContext& context) { setLookAndFeel(context.lookAndFeel); setTooltip("Undo"); }

    void paint(juce::Graphics& g) override
    {
        CustomLookAndFeel::get(*this).drawUndoButton(g, *this, isDown);
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

#endif //SEQUENCETREE_UNDOBUTTON_H
