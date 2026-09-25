//
// Created by Eli Baumgardner on 8/23/26.
//

#include "ValueFormat.h"
#include "../../../Util/NodeInfo.h"

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

static constexpr int semitonesPerOctave = 12;

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
