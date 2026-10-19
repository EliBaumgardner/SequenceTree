#include "RootRectangle.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include <ArsNovaUI/Editors/Formats/ValueFormat.h>
#include "../Editors/Formats/TraversalFlagFormat.h"

static constexpr int traversalRefListLength = 24;

RootRectangle::RootRectangle(juce::UndoManager& undoManager) : traversalEditor(undoManager)
{
    auto traversalFormat = std::make_unique<ArsNovaUI::TextFormat>(traversalRefListLength, "0123456789 ," + TraversalFlagFormat::instanceLetters);
    traversalFormat->trimsWhitespace = false;

    traversalEditor.setInterceptsMouseClicks(true, false);
    traversalEditor.setTooltip("Starting Traversal");
    traversalEditor.setFormat(std::move(traversalFormat));

    addAndMakeVisible(traversalEditor);
}

void RootRectangle::paint(juce::Graphics& graphics)
{
    ArsNovaUI::CustomLookAndFeel::get(*this).drawRaisedPanel(graphics, getLocalBounds().toFloat());
}

void RootRectangle::resized()
{
    traversalEditor.setBounds(getLocalBounds());
}
