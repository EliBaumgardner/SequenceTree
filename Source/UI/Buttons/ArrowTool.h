//
// Created by Eli Baumgardner.
//

#ifndef SEQUENCETREE_ARROWTOOL_H
#define SEQUENCETREE_ARROWTOOL_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Util/ApplicationContext.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"

class ArrowTool : public juce::Component {

public:

    ApplicationContext& context;

    bool isSelected = false;

    ArrowTool(ApplicationContext& context) : context(context) {
        setLookAndFeel(context.lookAndFeel);
    }

    void paint(juce::Graphics& g) override {
        CustomLookAndFeel::get(*this).drawArrowTool(g, *this);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isLeftButtonDown()) {
            isSelected = !isSelected;
            context.canvas->danglingArrowLayer.setArrowMode(isSelected);
            repaint();
        }
    }
};

#endif //SEQUENCETREE_ARROWTOOL_H
