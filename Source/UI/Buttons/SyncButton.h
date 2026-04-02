//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_SYNCBUTTON_H
#define SEQUENCETREE_SYNCBUTTON_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../CustomLookAndFeel.h"
#include "../../Util/PluginContext.h"

class SyncButton : public juce::Button {

public:
    SyncButton() : juce::Button("button") { setLookAndFeel(ComponentContext::lookAndFeel); }

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override
    {
        if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawSyncButton(g, isMouseOver, isButtonDown, *this); }
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        juce::Button::mouseDown(event);
        isSynced = !isSynced;
        repaint();
    }

    bool isSynced = true;
};

#endif //SEQUENCETREE_SYNCBUTTON_H
