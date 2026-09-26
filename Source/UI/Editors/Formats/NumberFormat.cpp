//
// Created by Eli Baumgardner on 8/23/26.
//

#include "ValueFormat.h"

#include <cmath>

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
    limits.maxLength         = juce::String(static_cast<juce::int64>(maximum / displayDivisor)).length();

    if (minimum < 0.0) {
        limits.allowedCharacters = "-" + limits.allowedCharacters;
        limits.maxLength         = juce::jmax(limits.maxLength,
                                              juce::String(static_cast<juce::int64>(minimum / displayDivisor)).length());
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
    const double scaled = static_cast<double>(binding.primary.getValue()) / displayDivisor;

    juce::String number { static_cast<int>(scaled) };

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
        const double placeScale = std::pow(10.0, decimalPlaces);
        const double rounded    = std::round(entered * placeScale) / placeScale;
        return { juce::jlimit(minimum, maximum, rounded), {} };
    }

    return { static_cast<int>(clamped), {} };
}
