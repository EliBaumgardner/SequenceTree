//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_DISPLAYBUTTON_H
#define SEQUENCETREE_DISPLAYBUTTON_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"

class DisplayButton : public juce::Component, public juce::SettableTooltipClient {

    public:

    bool isSelected = false;
    std::function<void()> onClick;

    DisplayButton(ApplicationContext& context) { setLookAndFeel(context.lookAndFeel); setTooltip("Display Options"); };

    void paint(juce::Graphics& g) override
    {
        const Theme& theme = CustomLookAndFeel::get(*this);
        auto bounds = getLocalBounds().toFloat().reduced(Theme::outerButtonBoundsReduction);

        g.setColour(isSelected ? theme.buttonColour.darker() : theme.buttonColour);

        float triangleHeight = bounds.getHeight() * 0.9f;
        float centreY = bounds.getCentreY();
        float topY    = centreY - triangleHeight / 2.0f;
        float bottomY = centreY + triangleHeight / 2.0f;

        juce::Path vPath;
        vPath.startNewSubPath(bounds.getX(), topY);
        vPath.lineTo(bounds.getCentreX(), bottomY);
        vPath.lineTo(bounds.getRight(), topY);
        vPath.closeSubPath();

        g.fillPath(vPath);
    }

    void mouseDown(const juce::MouseEvent& e) override { onClick(); }
};

#endif //SEQUENCETREE_DISPLAYBUTTON_H
