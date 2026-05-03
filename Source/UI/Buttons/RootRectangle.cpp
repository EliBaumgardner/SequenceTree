//
// Created by Eli Baumgardner on 4/11/26.
//

#include "RootRectangle.h"
#include "../Theme/CustomLookAndFeel.h"

RootRectangle::RootRectangle(ApplicationContext& context) : loopLimitEditor(context)
{
    setLookAndFeel(context.lookAndFeel);

    loopLimitEditor.setMinimumValue(0);
    loopLimitEditor.setInterceptsMouseClicks(true, false);
    loopLimitEditor.setTooltip("Loop Limit");
    addAndMakeVisible(loopLimitEditor);
}

void RootRectangle::paint(juce::Graphics &g) {
    CustomLookAndFeel::get(*this).drawRootNodeRectangle(g, *this);
}

void RootRectangle::resized() {
    loopLimitEditor.setBounds(getLocalBounds());
}