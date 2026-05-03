#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Custom caret — controls width and colour independently of the LookAndFeel.
// JUCE positions and resizes this component automatically; we just override paint.
class CustomTextCaret : public juce::CaretComponent
{
public:
    explicit CustomTextCaret(juce::Component* keyFocusOwner);
    void paint(juce::Graphics& g) override;

    juce::Colour caretColour { juce::Colours::white };
    float        caretWidth  { 2.0f };   // pixel width of the drawn bar
};