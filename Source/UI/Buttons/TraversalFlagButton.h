//
// Created by Eli Baumgardner on 6/30/26.
//

#ifndef SEQUENCETREE_TRAVERSALFLAGBUTTON_H
#define SEQUENCETREE_TRAVERSALFLAGBUTTON_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"

class TraversalFlagButton : public juce::Component, public juce::SettableTooltipClient {

public:

    bool isSelected = false;

    std::function<void()> onClick;

    TraversalFlagButton(ApplicationContext& context) { setLookAndFeel(context.lookAndFeel); setTooltip("Traversal Flag Mode"); }

    void paint(juce::Graphics& g) override
    {
        CustomLookAndFeel::get(*this).drawTraversalFlagButton(g, *this);
    }

    void mouseDown(const juce::MouseEvent& e) override { onClick(); }
};

#endif //SEQUENCETREE_TRAVERSALFLAGBUTTON_H
