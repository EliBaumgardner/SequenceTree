#pragma once

#include <ArsNovaUI/Editors/Formats/ValueFormat.h>
#include "../../../Graph/RTData.h"

class TraversalFlagFormat : public ArsNovaUI::NumberFormat
{
public:

    TraversalFlagFormat();

    ArsNovaUI::InputRestrictions restrictions() const override;
    juce::String                 text (const ArsNovaUI::ValueBinding& binding, ArsNovaUI::TextPurpose purpose) const override;
    ArsNovaUI::ParsedValue       parse(const ArsNovaUI::ValueBinding& binding, const juce::String& enteredText) const override;

    static std::vector<TraversalKey> parseKeys(const juce::String& text);
    static juce::String              describe (const TraversalKey& key);

    static const juce::String instanceLetters;
};
