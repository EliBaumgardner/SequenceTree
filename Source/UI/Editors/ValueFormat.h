//
// Created by Eli Baumgardner on 8/23/26.
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <limits>
#include <vector>

#include "../../Graph/RTData.h"

struct InputRestrictions {
    int          maxLength = 4;
    juce::String allowedCharacters { "0123456789" };
};

struct ValueBinding {
    juce::ValueTree               tree;
    juce::Value                   primary;
    juce::Identifier              primaryId;
    std::vector<juce::Value>      secondaries;
    std::vector<juce::Identifier> secondaryIds;

    bool hasSecondary() const { return ! secondaries.empty(); }
};


class ValueFormat {
public:

    virtual ~ValueFormat() = default;

    virtual InputRestrictions restrictions() const = 0;

    virtual juce::String displayText(const ValueBinding& binding) const = 0;
    virtual juce::String editText   (const ValueBinding& binding) const { return displayText(binding); }

    virtual void commit(const juce::String& text, ValueBinding binding) const = 0;

    virtual std::vector<juce::Identifier> extraProperties() const { return {}; }

    double clamp(double value) const { return juce::jlimit(minimum, maximum, value); }

    void setMinimum(double newMinimum) { minimum = newMinimum; }
    void setMaximum(double newMaximum) { maximum = newMaximum; }

protected:

    double minimum = 1.0;
    double maximum = (double) std::numeric_limits<int>::max();
};


class IntFormat : public ValueFormat {
public:

    explicit IntFormat(bool shouldShowSign = false) : showSign(shouldShowSign) {}

    bool showsSign() const { return showSign; }

    InputRestrictions restrictions() const override;
    juce::String      displayText(const ValueBinding& binding) const override;
    juce::String      editText   (const ValueBinding& binding) const override;
    void              commit(const juce::String& text, ValueBinding binding) const override;

private:

    bool showSign;
};


class DecimalFormat : public ValueFormat {
public:

    DecimalFormat();

    InputRestrictions restrictions() const override;
    juce::String      displayText(const ValueBinding& binding) const override;
    void              commit(const juce::String& text, ValueBinding binding) const override;
};


class DecimalMultiplierFormat : public DecimalFormat {
public:

    juce::String displayText(const ValueBinding& binding) const override;
    juce::String editText   (const ValueBinding& binding) const override;
};


class PitchFormat : public ValueFormat {
public:

    InputRestrictions restrictions() const override;
    juce::String      displayText(const ValueBinding& binding) const override;
    juce::String      editText   (const ValueBinding& binding) const override;
    void              commit(const juce::String& text, ValueBinding binding) const override;
};


class MultiplierFormat : public ValueFormat {
public:

    MultiplierFormat();

    InputRestrictions restrictions() const override;
    juce::String      displayText(const ValueBinding& binding) const override;
    juce::String      editText   (const ValueBinding& binding) const override;
    void              commit(const juce::String& text, ValueBinding binding) const override;
};


class PercentFormat : public ValueFormat {
public:

    PercentFormat();

    InputRestrictions restrictions() const override;
    juce::String      displayText(const ValueBinding& binding) const override;
    juce::String      editText   (const ValueBinding& binding) const override;
    void              commit(const juce::String& text, ValueBinding binding) const override;
};


class ArrowDurationFormat : public ValueFormat {
public:

    static constexpr int millisecondsPerPercent = 10;

    explicit ArrowDurationFormat(bool showsPercent);

    InputRestrictions restrictions() const override;
    juce::String      displayText(const ValueBinding& binding) const override;
    juce::String      editText   (const ValueBinding& binding) const override;
    void              commit(const juce::String& text, ValueBinding binding) const override;

private:

    bool percent;
};


class GreekLetterFormat : public ValueFormat {
public:

    static const int letterCount;

    GreekLetterFormat();

    InputRestrictions restrictions() const override;
    juce::String      displayText(const ValueBinding& binding) const override;
    juce::String      editText   (const ValueBinding& binding) const override;
    void              commit(const juce::String& text, ValueBinding binding) const override;
};


class TextFormat : public ValueFormat {
public:

    InputRestrictions restrictions() const override;
    juce::String      displayText(const ValueBinding& binding) const override;
    void              commit(const juce::String& text, ValueBinding binding) const override;
};


class FreeTextFormat : public ValueFormat {
public:

    InputRestrictions restrictions() const override;
    juce::String      displayText(const ValueBinding& binding) const override;
    void              commit(const juce::String& text, ValueBinding binding) const override;
};

class TraversalFlagFormat : public ValueFormat {
public:

    InputRestrictions             restrictions() const override;
    juce::String                  displayText(const ValueBinding& binding) const override;
    void                          commit(const juce::String& text, ValueBinding binding) const override;
    std::vector<juce::Identifier> extraProperties() const override;
};


class DualIntFormat : public ValueFormat {
public:

    explicit DualIntFormat(const juce::Identifier& secondaryPropertyID)
        : secondaryId(secondaryPropertyID) {}

    InputRestrictions             restrictions() const override;
    juce::String                  displayText(const ValueBinding& binding) const override;
    void                          commit(const juce::String& text, ValueBinding binding) const override;
    std::vector<juce::Identifier> extraProperties() const override { return { secondaryId }; }

private:

    juce::Identifier secondaryId;
};


class TraversalRefListFormat : public ValueFormat {
public:

    static std::vector<TraversalKey> parse(const juce::String& text);

    static juce::String describe(const TraversalKey& key);

    InputRestrictions restrictions() const override;
    juce::String      displayText(const ValueBinding& binding) const override;
    void              commit(const juce::String& text, ValueBinding binding) const override;
};
