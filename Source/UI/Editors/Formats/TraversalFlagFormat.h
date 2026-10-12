#pragma once

#include <ArsNovaUI/Editors/Formats/ValueFormat.h>
#include "../../../Graph/RTData.h"

class TraversalFlagFormat : public NumberFormat
{
public:

    TraversalFlagFormat();

    InputRestrictions restrictions() const override;
    juce::String      text (const ValueBinding& binding, TextPurpose purpose) const override;
    ParsedValue       parse(const ValueBinding& binding, const juce::String& enteredText) const override;

    static std::vector<TraversalKey> parseKeys(const juce::String& text);
    static juce::String              describe (const TraversalKey& key);

    static const juce::String instanceLetters;
};
