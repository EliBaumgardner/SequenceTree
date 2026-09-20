//
// Created by Eli Baumgardner on 4/11/26.
//

#include "RootRectangle.h"
#include "../Theme/CustomLookAndFeel.h"

RootRectangle::RootRectangle(ApplicationContext& context) : traversalEditor(context)
{
    setLookAndFeel(context.lookAndFeel);

    traversalEditor.setInterceptsMouseClicks(true, false);
    traversalEditor.setTooltip("Starting Traversal");
    traversalEditor.setFormat(std::make_unique<TraversalRefListFormat>());
    addAndMakeVisible(traversalEditor);
}

void RootRectangle::paint(juce::Graphics &g) {
    CustomLookAndFeel::get(*this).drawRootRectangle(g, getLocalBounds().toFloat());
}

void RootRectangle::resized() {
    traversalEditor.setBounds(getLocalBounds());
}