//
// Created by Eli Baumgardner on 4/11/26.
//

#include "RootRectangle.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Editors/Formats/ValueFormat.h"

static constexpr int traversalRefListLength = 24;

RootRectangle::RootRectangle(const ApplicationContext& context) : traversalEditor(context)
{
    setLookAndFeel(context.lookAndFeel);

    auto traversalFormat = std::make_unique<TextFormat>(traversalRefListLength,
                                                        "0123456789 ," + TraversalFlagFormat::instanceLetters);
    traversalFormat->trimsWhitespace = false;

    traversalEditor.setInterceptsMouseClicks(true, false);
    traversalEditor.setTooltip("Starting Traversal");
    traversalEditor.setFormat(std::move(traversalFormat));
    addAndMakeVisible(traversalEditor);
}

void RootRectangle::paint(juce::Graphics &g) {
    CustomLookAndFeel::get(*this).drawRootRectangle(g, getLocalBounds().toFloat());
}

void RootRectangle::resized() {
    traversalEditor.setBounds(getLocalBounds());
}