//
// Created by Eli Baumgardner on 8/23/26.
//

#include "ValueFormat.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Util/NodeInfo.h"

static const juce::String pitchNames[] = {
    juce::String(L"C"),
    juce::String(L"C♯"),
    juce::String(L"D"),
    juce::String(L"D♯"),
    juce::String(L"E"),
    juce::String(L"F"),
    juce::String(L"F♯"),
    juce::String(L"G"),
    juce::String(L"G♯"),
    juce::String(L"A"),
    juce::String(L"A♯"),
    juce::String(L"B")
};

static const juce::String greekLetters[] = {
    juce::String(L"α"),
    juce::String(L"β"),
    juce::String(L"γ"),
    juce::String(L"δ"),
    juce::String(L"ε"),
    juce::String(L"ζ"),
    juce::String(L"η"),
    juce::String(L"θ"),
    juce::String(L"ι"),
    juce::String(L"κ"),
    juce::String(L"λ"),
    juce::String(L"μ"),
    juce::String(L"ν"),
    juce::String(L"ξ"),
    juce::String(L"ο"),
    juce::String(L"π"),
    juce::String(L"ρ"),
    juce::String(L"σ"),
    juce::String(L"τ"),
    juce::String(L"υ"),
    juce::String(L"φ"),
    juce::String(L"χ"),
    juce::String(L"ψ"),
    juce::String(L"ω")
};

static const int greekLetterCount = (int) (sizeof(greekLetters) / sizeof(greekLetters[0]));

static const juce::String instanceLetters { "abcdefghijklmnopqrstuvwxyz" };

static const int semitonesPerOctave = 12;

const juce::String TextFormat::labelCharacters {
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-."
};

NumberFormat::NumberFormat(double lowest, double highest, int places)
{
    minimum       = lowest;
    maximum       = highest;
    decimalPlaces = places;
}

InputRestrictions NumberFormat::restrictions() const
{
    InputRestrictions limits;

    limits.allowedCharacters = "0123456789";
    limits.maxLength         = juce::String((juce::int64) (maximum / displayDivisor)).length();

    if (minimum < 0.0) {
        limits.allowedCharacters = "-" + limits.allowedCharacters;
        limits.maxLength         = juce::jmax(limits.maxLength,
                                              juce::String((juce::int64) (minimum / displayDivisor)).length());
    }

    if (showsPositiveSign) {
        limits.allowedCharacters = "+" + limits.allowedCharacters;
    }

    if (decimalPlaces > 0) {
        limits.allowedCharacters += ".";
        limits.maxLength         += decimalPlaces + 1;
    }

    return limits;
}

juce::String NumberFormat::text(const ValueBinding& binding, TextPurpose purpose) const
{
    const double scaled = (double) binding.primary.getValue() / displayDivisor;

    juce::String number { (int) scaled };

    if (decimalPlaces > 0) {
        number = juce::String(scaled, decimalPlaces).trimCharactersAtEnd("0").trimCharactersAtEnd(".");
    }

    if (purpose == TextPurpose::Editing) {
        return number;
    }

    if (showsPositiveSign && scaled > 0.0) {
        number = "+" + number;
    }

    return prefix + number + suffix;
}

ParsedValue NumberFormat::parse(const juce::String& enteredText) const
{
    const double entered = enteredText.getDoubleValue() * displayDivisor;
    const double clamped = juce::jlimit(minimum, maximum, entered);

    if (decimalPlaces > 0) {
        return { clamped, {} };
    }

    return { (int) clamped, {} };
}

PitchFormat::PitchFormat() : NumberFormat(minimumMidiPitch, maximumMidiPitch) {}

juce::String PitchFormat::text(const ValueBinding& binding, TextPurpose purpose) const
{
    if (purpose == TextPurpose::Editing) {
        return NumberFormat::text(binding, purpose);
    }

    const int midiNote = juce::jlimit(minimumMidiPitch, maximumMidiPitch, (int) binding.primary.getValue());
    const int octave   = (midiNote / semitonesPerOctave) - 1;

    return pitchNames[midiNote % semitonesPerOctave] + juce::String(octave);
}

GreekLetterFormat::GreekLetterFormat() : NumberFormat(0, greekLetterCount - 1) {}

juce::String GreekLetterFormat::text(const ValueBinding& binding, TextPurpose purpose) const
{
    if (purpose == TextPurpose::Editing) {
        return NumberFormat::text(binding, purpose);
    }

    const int letter = juce::jlimit(0, greekLetterCount - 1, (int) binding.primary.getValue());

    return greekLetters[letter];
}

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
    const int value = (int) binding.primary.getValue();

    if (value == 0) {
        return {};
    }

    int instance = 0;

    if (! binding.secondaries.empty()) {
        instance = (int) binding.secondaries.front().getValue();
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

    const bool spawns  = trimmed.startsWithChar('+');
    const bool removes = trimmed.startsWithChar('-');

    ParsedValue cleared { 0, { 0 } };

    if (! spawns && ! removes) {
        return cleared;
    }

    const std::vector<TraversalKey> keys = TraversalRefListFormat::parseKeys(trimmed.substring(1));

    if (keys.empty()) {
        return cleared;
    }

    const int typeId = (int) juce::jlimit(1.0, maximum, (double) keys.front().typeId);

    if (removes) {
        return { -typeId, { keys.front().instance } };
    }

    return { typeId, { keys.front().instance } };
}

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

static constexpr int traversalRefListLength = 24;

TraversalRefListFormat::TraversalRefListFormat()
    : TextFormat(traversalRefListLength, "0123456789 ," + instanceLetters)
{
    trimsWhitespace = false;
}

std::vector<TraversalKey> TraversalRefListFormat::parseKeys(const juce::String& text)
{
    std::vector<TraversalKey> parsed;
    juce::String digits;

    for (int i = 0; i <= text.length(); i++) {

        const bool isDigit = i < text.length() && juce::CharacterFunctions::isDigit(text[i]);

        if (isDigit) {
            digits += text[i];
            continue;
        }

        if (digits.isEmpty()) {
            continue;
        }

        TraversalKey key;
        key.typeId = digits.getIntValue();

        if (i < text.length()) {
            const int instance = instanceLetters.indexOfChar(text[i]);

            if (instance != -1) {
                key.instance = instance;
            }
        }

        parsed.push_back(key);
        digits.clear();
    }

    return parsed;
}

juce::String TraversalRefListFormat::describe(const TraversalKey& key)
{
    const int          letter       = juce::jlimit(0, TraversalKey::maxInstances - 1, key.instance);
    const juce::String instanceText = instanceLetters.substring(letter, letter + 1);

    return juce::String(key.typeId) + instanceText;
}
