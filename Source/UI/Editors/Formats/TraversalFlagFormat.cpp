#include "ValueFormat.h"
#include "../../../Graph/ValueTreeIdentifiers.h"

const juce::String TraversalFlagFormat::instanceLetters { "abcdefghijklmnopqrstuvwxyz" };

static constexpr int maximumTraversalTypeId = 9999;

TraversalFlagFormat::TraversalFlagFormat()
    : NumberFormat(-maximumTraversalTypeId, maximumTraversalTypeId)
{
    extraProperties = { ValueTreeIdentifiers::TraversalInstance };
}

InputRestrictions TraversalFlagFormat::restrictions() const
{
    InputRestrictions limits = NumberFormat::restrictions();

    limits.allowedCharacters = "+" + limits.allowedCharacters + instanceLetters;
    limits.maxLength        += 1;

    return limits;
}

juce::String TraversalFlagFormat::text(const ValueBinding& binding, TextPurpose purpose) const
{
    const int value = static_cast<int>(binding.primary.getValue());

    if (value == 0) {
        return {};
    }

    int instance = 0;

    if (! binding.secondaries.empty()) {
        instance = static_cast<int>(binding.secondaries.front().getValue());
    }

    const int          letter       = juce::jlimit(0, TraversalKey::maxInstances - 1, instance);
    const juce::String instanceText = instanceLetters.substring(letter, letter + 1);

    if (value > 0) {
        return "+" + juce::String(value) + instanceText;
    }

    return "-" + juce::String(-value) + instanceText;
}

ParsedValue TraversalFlagFormat::parse(const juce::String& enteredText) const
{
    const juce::String trimmed = enteredText.trim();
    const bool         spawns  = trimmed.startsWithChar('+');
    const bool         removes = trimmed.startsWithChar('-');
    const ParsedValue  cleared { 0, { 0 } };

    if (! spawns && ! removes) {
        return cleared;
    }

    const std::vector<TraversalKey> keys = parseKeys(trimmed.substring(1));

    if (keys.empty()) {
        return cleared;
    }

    const int typeId = static_cast<int>(juce::jlimit(1.0, maximum, static_cast<double>(keys.front().typeId)));

    if (removes) {
        return { -typeId, { keys.front().instance } };
    }

    return { typeId, { keys.front().instance } };
}

std::vector<TraversalKey> TraversalFlagFormat::parseKeys(const juce::String& text)
{
    std::vector<TraversalKey> parsed;
    juce::String              digits;

    for (int characterIndex = 0; characterIndex <= text.length(); ++characterIndex) {
        const bool isDigit = characterIndex < text.length() && juce::CharacterFunctions::isDigit(text[characterIndex]);

        if (isDigit) {
            digits += text[characterIndex];
            continue;
        }

        if (digits.isEmpty()) {
            continue;
        }

        TraversalKey key;

        key.typeId = digits.getIntValue();

        if (characterIndex < text.length()) {
            const int instance = instanceLetters.indexOfChar(text[characterIndex]);

            if (instance != -1) {
                key.instance = instance;
            }
        }

        parsed.push_back(key);

        digits.clear();
    }

    return parsed;
}

juce::String TraversalFlagFormat::describe(const TraversalKey& key)
{
    const int          letter       = juce::jlimit(0, TraversalKey::maxInstances - 1, key.instance);
    const juce::String instanceText = instanceLetters.substring(letter, letter + 1);

    return juce::String(key.typeId) + instanceText;
}
