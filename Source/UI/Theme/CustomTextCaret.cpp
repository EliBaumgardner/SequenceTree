#include "CustomTextCaret.h"

CustomTextCaret::CustomTextCaret(juce::Component* keyFocusOwner)
    : juce::CaretComponent(keyFocusOwner)
{
}

void CustomTextCaret::paint(juce::Graphics& graphics)
{
    auto bounds = getLocalBounds().toFloat();
    graphics.setColour(findColour(juce::CaretComponent::caretColourId, true));
    graphics.fillRect(bounds.withWidth(caretWidth));
}
