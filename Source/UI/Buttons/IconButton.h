//
// Created by Eli Baumgardner on 7/20/26.
//

#ifndef SEQUENCETREE_ICONBUTTON_H
#define SEQUENCETREE_ICONBUTTON_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Util/ApplicationContext.h"

class IconButton : public juce::Component {
public:

    std::function<void(juce::Graphics& g)> repainted;
    std::function<void()> onClick;

    IconButton(ApplicationContext& context) : context(context) {
        setLookAndFeel(context.lookAndFeel);
    }

    void paint(juce::Graphics& g) override {
        repainted(g);
    };

    void mouseUp(const juce::MouseEvent& e) override {
        if (onClick && getLocalBounds().contains(e.getPosition())) {
            onClick();
        }
    }

private:
    ApplicationContext& context;
};

#endif //SEQUENCETREE_ICONBUTTON_H
