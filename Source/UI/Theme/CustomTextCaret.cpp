#include "CustomTextCaret.h"

CustomTextCaret::CustomTextCaret(juce::Component* keyFocusOwner)
    : juce::CaretComponent(keyFocusOwner)
{
}

void CustomTextCaret::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(findColour(juce::CaretComponent::caretColourId, true));
    g.fillRect(bounds.withWidth(caretWidth));
}
