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

    std::function<void()> onClick;
    RedoButton() { setLookAndFeel(ComponentContext::lookAndFeel); }

    void paint(juce::Graphics& g) override
    {
        if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawRedoButton(g, *this,isDown); }
    }

    void mouseDown(const juce::MouseEvent &event) override
    {
        isDown = true;
        onClick();
        repaint();
    }

    void mouseUp(const juce::MouseEvent &event) override {
        isDown = false;
        onClick();
        repaint();
    };

};

#endif //SEQUENCETREE_REDOBUTTON_H
