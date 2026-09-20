//
// Created by Eli Baumgardner on 8/23/26.
//

#include "ValueFormat.h"

const juce::String TextFormat::labelCharacters {
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-."
};

TextFormat::TextFormat(int longestText, const juce::String& characters)
    : maxLength(longestText), allowedCharacters(characters) {}

InputRestrictions TextFormat::restrictions() const
{
    InputRestrictions limits;

    limits.maxLength         = maxLength;
    limits.allowedCharacters = allowedCharacters;

    return limits;
}

juce::String TextFormat::text(const ValueBinding& binding, TextPurpose purpose) const
{
    const juce::var stored = binding.primary.getValue();

    if (! stored.isString()) {
        return {};
    }

    return stored.toString();
}

ParsedValue TextFormat::parse(const juce::String& enteredText) const
{
    if (trimsWhitespace) {
        return { enteredText.trim(), {} };
    }

    return { enteredText, {} };
}
