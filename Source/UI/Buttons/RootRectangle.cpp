//
// Created by Eli Baumgardner on 4/11/26.
//

#include "RootRectangle.h"
#include "../Theme/CustomLookAndFeel.h"

RootRectangle::RootRectangle(ApplicationContext& context) : traversalEditor(context)
{
    setLookAndFeel(context.lookAndFeel);

    traversalEditor.setMinimumValue(0);
    traversalEditor.setInterceptsMouseClicks(true, false);
    traversalEditor.setTooltip("Loop Limit");
    traversalEditor.acceptMultipleValues();
    addAndMakeVisible(traversalEditor);
}

void RootRectangle::paint(juce::Graphics &g) {
    CustomLookAndFeel::get(*this).drawRootRectangle(g, getLocalBounds().toFloat());
}

void RootRectangle::resized() {
    traversalEditor.setBounds(getLocalBounds());
}