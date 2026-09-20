//
// Created by Eli Baumgardner on 8/23/26.
//

#include "ValueFormat.h"

DualIntFormat::DualIntFormat(double lowest, double highest, const juce::Identifier& secondaryPropertyID)
    : NumberFormat(lowest, highest)
{
    extraProperties = { secondaryPropertyID };
}

InputRestrictions DualIntFormat::restrictions() const
{
    InputRestrictions limits = NumberFormat::restrictions();

    limits.allowedCharacters += ":";
    limits.maxLength          = limits.maxLength * 2 + 1;

    return limits;
}

juce::String DualIntFormat::text(const ValueBinding& binding, TextPurpose purpose) const
{
    const int primaryValue = (int) binding.primary.getValue();

    if (binding.secondaries.empty()) {
        return juce::String(primaryValue);
    }

    const int secondaryValue = (int) binding.secondaries.front().getValue();

    if (secondaryValue <= 0) {
        return juce::String(primaryValue);
    }

    return juce::String(primaryValue) + ":" + juce::String(secondaryValue);
}

ParsedValue DualIntFormat::parse(const juce::String& enteredText) const
{
    const int separatorIndex = enteredText.indexOfChar(':');

    juce::String primaryText = enteredText;
    juce::String secondaryText;

    if (separatorIndex >= 0) {
        primaryText   = enteredText.substring(0, separatorIndex);
        secondaryText = enteredText.substring(separatorIndex + 1);
    }

    const int primaryValue   = (int) juce::jlimit(minimum, maximum, (double) primaryText.getIntValue());
    const int secondaryValue = (int) juce::jlimit(0.0,     maximum, (double) secondaryText.getIntValue());

    return { primaryValue, { secondaryValue } };
}
