#include "CustomTextCaret.h"

CustomTextCaret::CustomTextCaret(juce::Component* keyFocusOwner)
    : juce::CaretComponent(keyFocusOwner)
{
}

void CustomTextCaret::paint(juce::Graphics& g)
{
    // getLocalBounds() is the full character-height slot JUCE reserved for the caret.
    // We draw a thin vertical bar at the left edge, using our own width and colour.
    auto bounds = getLocalBounds().toFloat();
    g.setColour(caretColour);
    g.fillRect(bounds.withWidth(caretWidth));
}