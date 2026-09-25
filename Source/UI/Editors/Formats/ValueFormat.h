//
// Created by Eli Baumgardner on 8/23/26.
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <limits>
#include <vector>

#include "../../../Graph/RTData.h"

struct InputRestrictions {
    int          maxLength = 4;
    juce::String allowedCharacters { "0123456789" };
};

struct ValueBinding {
    const juce::Value&              primary;
    const std::vector<juce::Value>& secondaries;
};

struct ParsedValue {
    juce::var              primary;
    std::vector<juce::var> secondaries;
};

enum class TextPurpose : uint8_t { Display, Editing };


class ValueFormat {
public:

    virtual ~ValueFormat() = default;

    virtual InputRestrictions restrictions() const = 0;
    virtual juce::String      text (const ValueBinding& binding, TextPurpose purpose) const = 0;
    virtual ParsedValue       parse(const juce::String& enteredText) const = 0;

    std::vector<juce::Identifier> extraProperties;

    double minimum       = 0.0;
    double maximum       = static_cast<double>(std::numeric_limits<int>::max());
    int    decimalPlaces = 0;
};


class NumberFormat : public ValueFormat {
public:

    NumberFormat(double lowest, double highest, int places = 0);

    InputRestrictions restrictions() const override;
    juce::String      text (const ValueBinding& binding, TextPurpose purpose) const override;
    ParsedValue       parse(const juce::String& enteredText) const override;

    juce::String prefix;
    juce::String suffix;

    double displayDivisor    = 1.0;
    bool   showsPositiveSign = false;
};


class PitchFormat : public NumberFormat {
public:

    PitchFormat();

    juce::String text(const ValueBinding& binding, TextPurpose purpose) const override;
};


class GreekLetterFormat : public NumberFormat {
public:

    GreekLetterFormat();

    juce::String text(const ValueBinding& binding, TextPurpose purpose) const override;
};


class DualIntFormat : public NumberFormat {
public:

    DualIntFormat(double lowest, double highest, const juce::Identifier& secondaryPropertyID);

    InputRestrictions restrictions() const override;
    juce::String      text (const ValueBinding& binding, TextPurpose purpose) const override;
    ParsedValue       parse(const juce::String& enteredText) const override;
};


class TraversalFlagFormat : public NumberFormat {
public:

    TraversalFlagFormat();

    InputRestrictions restrictions() const override;
    juce::String      text (const ValueBinding& binding, TextPurpose purpose) const override;
    ParsedValue       parse(const juce::String& enteredText) const override;

    static std::vector<TraversalKey> parseKeys(const juce::String& text);
    static juce::String              describe (const TraversalKey& key);

    static const juce::String instanceLetters;
};


class TextFormat : public ValueFormat {
public:

    TextFormat(int longestText, const juce::String& characters);

    InputRestrictions restrictions() const override;
    juce::String      text (const ValueBinding& binding, TextPurpose purpose) const override;
    ParsedValue       parse(const juce::String& enteredText) const override;

    static constexpr int      labelTextLength = 64;
    static const juce::String labelCharacters;

    int          maxLength;
    juce::String allowedCharacters;
    bool         trimsWhitespace = true;
};

