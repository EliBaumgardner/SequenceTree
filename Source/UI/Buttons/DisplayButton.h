//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_DISPLAYBUTTON_H
#define SEQUENCETREE_DISPLAYBUTTON_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../CustomLookAndFeel.h"
#include "../../Util/PluginContext.h"

class DisplayButton : public juce::Component {

    public:

    bool isSelected = false;
    std::function<void()> onClick;

    DisplayButton() { setLookAndFeel(ComponentContext::lookAndFeel); };

    void paint(juce::Graphics& g) override
    {
        if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawDisplayButton(g, *this); }
    }

    void mouseDown(const juce::MouseEvent& e) override { onClick(); }
};

#endif //SEQUENCETREE_DISPLAYBUTTON_H
