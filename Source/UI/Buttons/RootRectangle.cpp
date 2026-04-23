//
// Created by Eli Baumgardner on 4/11/26.
//

#include "RootRectangle.h"
#include "../Util/PluginContext.h"
#include "../UI/CustomLookAndFeel.h"

RootRectangle::RootRectangle() {
    setLookAndFeel(ComponentContext::lookAndFeel);

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