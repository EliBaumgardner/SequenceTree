//
// Created by Eli Baumgardner on 8/23/26.
//

#include "ValueFormat.h"

namespace {

const juce::String pitchNames[] = {
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

}

InputRestrictions IntFormat::restrictions() const
{
    return { 4, showSign ? "-0123456789" : "0123456789" };
}

juce::String IntFormat::displayText(const ValueBinding& binding) const
{
    const int value = (int) binding.primary.getValue();

    if (showSign && value > 0) {
        return "+" + juce::String(value);
    }

    return juce::String(value);
}

juce::String IntFormat::editText(const ValueBinding& binding) const
{
    return juce::String((int) binding.primary.getValue());
}

void IntFormat::commit(const juce::String& text, ValueBinding binding) const
{
    binding.primary.setValue((int) clamp((double) text.getIntValue()));
}

DecimalFormat::DecimalFormat()
{
    minimum = 0.1;
    maximum = std::numeric_limits<double>::max();
}

InputRestrictions DecimalFormat::restrictions() const
{
    return { 6, "0123456789." };
}

juce::String DecimalFormat::displayText(const ValueBinding& binding) const
{
    juce::String text((double) binding.primary.getValue(), 3);
    text = text.trimCharactersAtEnd("0").trimCharactersAtEnd(".");

    if (text.isEmpty()) {
        return "0";
    }

    return text;
}

void DecimalFormat::commit(const juce::String& text, ValueBinding binding) const
{
    binding.primary.setValue(clamp(text.getDoubleValue()));
}

InputRestrictions PitchFormat::restrictions() const
{
    return { 4, "0123456789" };
}

juce::String PitchFormat::displayText(const ValueBinding& binding) const
{
    const int midiNote = juce::jlimit(0, 127, (int) binding.primary.getValue());

    return pitchNames[midiNote % 12] + juce::String((midiNote / 12) - 1);
}

juce::String PitchFormat::editText(const ValueBinding& binding) const
{
    return juce::String((int) binding.primary.getValue());
}

void PitchFormat::commit(const juce::String& text, ValueBinding binding) const
{
    binding.primary.setValue((int) clamp((double) text.getIntValue()));
}

MultiplierFormat::MultiplierFormat()
{
    minimum = 1.0;
}

InputRestrictions MultiplierFormat::restrictions() const
{
    return { 4, "0123456789" };
}

juce::String MultiplierFormat::displayText(const ValueBinding& binding) const
{
    return "x" + juce::String((int) binding.primary.getValue());
}

juce::String MultiplierFormat::editText(const ValueBinding& binding) const
{
    return juce::String((int) binding.primary.getValue());
}

void MultiplierFormat::commit(const juce::String& text, ValueBinding binding) const
{
    binding.primary.setValue((int) clamp((double) text.getIntValue()));
}

PercentFormat::PercentFormat()
{
    minimum = 0.0;
    maximum = 100.0;
}

InputRestrictions PercentFormat::restrictions() const
{
    return { 3, "0123456789" };
}

juce::String PercentFormat::displayText(const ValueBinding& binding) const
{
    return juce::String((int) binding.primary.getValue()) + "%";
}

juce::String PercentFormat::editText(const ValueBinding& binding) const
{
    return juce::String((int) binding.primary.getValue());
}

void PercentFormat::commit(const juce::String& text, ValueBinding binding) const
{
    binding.primary.setValue((int) clamp((double) text.getIntValue()));
}

InputRestrictions TextFormat::restrictions() const
{
    return { 64, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-." };
}

juce::String TextFormat::displayText(const ValueBinding& binding) const
{
    return binding.primary.getValue().toString();
}

void TextFormat::commit(const juce::String& text, ValueBinding binding) const
{
    binding.primary.setValue(text.trim());
}

InputRestrictions FreeTextFormat::restrictions() const
{
    return { 0, "" };
}

juce::String FreeTextFormat::displayText(const ValueBinding& binding) const
{
    return binding.primary.getValue().toString();
}

void FreeTextFormat::commit(const juce::String& text, ValueBinding binding) const
{
    binding.primary.setValue(text);
}

InputRestrictions PlusRequiredFormat::restrictions() const
{
    return { 5, "+-0123456789" };
}

juce::String PlusRequiredFormat::displayText(const ValueBinding& binding) const
{
    const int value = (int) binding.primary.getValue();

    if (value > 0) {
        return "+" + juce::String(value);
    }

    if (value < 0) {
        return juce::String(value);
    }

    return juce::String();
}

void PlusRequiredFormat::commit(const juce::String& text, ValueBinding binding) const
{
    const juce::String trimmed = text.trim();

    const bool spawns  = trimmed.startsWithChar('+');
    const bool removes = trimmed.startsWithChar('-');

    if (! spawns && ! removes) {
        binding.primary.setValue(0);
        return;
    }

    const int magnitude = (int) clamp((double) trimmed.substring(1).getIntValue());

    binding.primary.setValue(removes ? -magnitude : magnitude);
}

InputRestrictions DualIntFormat::restrictions() const
{
    return { 9, "0123456789:" };
}

juce::String DualIntFormat::displayText(const ValueBinding& binding) const
{
    const int primaryValue = (int) binding.primary.getValue();

    if (! binding.hasSecondary()) {
        return juce::String(primaryValue);
    }

    const int secondaryValue = (int) binding.secondaries.front().getValue();

    if (secondaryValue <= 0) {
        return juce::String(primaryValue);
    }

    return juce::String(primaryValue) + ":" + juce::String(secondaryValue);
}

void DualIntFormat::commit(const juce::String& text, ValueBinding binding) const
{
    const int separatorIndex = text.indexOfChar(':');

    juce::String primaryText = text;
    juce::String secondaryText;

    if (separatorIndex >= 0) {
        primaryText   = text.substring(0, separatorIndex);
        secondaryText = text.substring(separatorIndex + 1);
    }

    int primaryValue = primaryText.getIntValue();

    if (primaryValue < (int) minimum) {
        primaryValue = (int) minimum;
    }

    binding.primary.setValue(primaryValue);

    if (! binding.hasSecondary()) {
        return;
    }

    int secondaryValue = 0;

    if (secondaryText.isNotEmpty()) {
        secondaryValue = juce::jmax(0, secondaryText.getIntValue());
    }

    binding.secondaries.front().setValue(secondaryValue);
}

std::vector<int> IntListFormat::parse(const juce::String& text)
{
    std::vector<int> parsed;
    juce::String word;

    for (int i = 0; i <= text.length(); i++) {

        const bool isDigit = i < text.length() && juce::CharacterFunctions::isDigit(text[i]);

        if (isDigit) {
            word += text[i];
        }
        else if (word.isNotEmpty()) {
            parsed.push_back(word.getIntValue());
            word.clear();
        }
    }

    return parsed;
}

InputRestrictions IntListFormat::restrictions() const
{
    return { 8, "0123456789' '," };
}

juce::String IntListFormat::displayText(const ValueBinding& binding) const
{
    return binding.primary.getValue().toString();
}

void IntListFormat::commit(const juce::String& text, ValueBinding binding) const
{
    binding.primary.setValue(text);
}
