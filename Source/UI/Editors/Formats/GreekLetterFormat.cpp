//
// Created by Eli Baumgardner on 8/23/26.
//

#include "ValueFormat.h"

#include <iterator>

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

static constexpr int greekLetterCount = static_cast<int>(std::size(greekLetters));

GreekLetterFormat::GreekLetterFormat() : NumberFormat(0, greekLetterCount - 1) {}

juce::String GreekLetterFormat::text(const ValueBinding& binding, TextPurpose purpose) const
{
    if (purpose == TextPurpose::Editing) {
        return NumberFormat::text(binding, purpose);
    }

    const int letter = juce::jlimit(0, greekLetterCount - 1, (int) binding.primary.getValue());

    return greekLetters[letter];
}
