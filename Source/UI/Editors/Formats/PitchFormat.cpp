#include "ValueFormat.h"
#include "../../../Util/NodeInfo.h"

PitchFormat::PitchFormat() : NumberFormat(minimumMidiPitch, maximumMidiPitch)
{
}

static constexpr int maximumPitchTextLength = 4;

InputRestrictions PitchFormat::restrictions() const
{
    InputRestrictions limits;

    limits.allowedCharacters = "abcdefgABCDEFG+-0123456789";
    limits.maxLength         = maximumPitchTextLength;

    return limits;
}

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

juce::String PitchFormat::text(const ValueBinding& binding, TextPurpose purpose) const
{
    const int          midiNote = juce::jlimit(minimumMidiPitch, maximumMidiPitch, static_cast<int>(binding.primary.getValue()));
    const int          octave   = (midiNote / semitonesPerOctave) - 1;
    const juce::String name     = pitchNames[midiNote % semitonesPerOctave] + juce::String(octave);

    if (purpose == TextPurpose::Editing) {
        return name.replace(juce::String(L"♯"), "+");
    }

    return name;
}

static const juce::String naturalLetters { "cdefgab" };

static constexpr int naturalSemitones[] = { 0, 2, 4, 5, 7, 9, 11 };

ParsedValue PitchFormat::parse(const ValueBinding& binding, const juce::String& enteredText) const
{
    const int          currentNote = juce::jlimit(minimumMidiPitch, maximumMidiPitch, static_cast<int>(binding.primary.getValue()));
    const juce::String typed       = enteredText.trim().toLowerCase();
    const int          letterIndex = naturalLetters.indexOfChar(typed[0]);
    juce::String       octaveText  = typed.substring(1);
    int                accidental  = 0;
    int                octave      = (currentNote / semitonesPerOctave) - 1;

    if (letterIndex < 0) {
        return { currentNote, {} };
    }

    if (octaveText.startsWithChar('+')) {
        accidental = 1;
        octaveText = octaveText.substring(1);
    }
    else if (octaveText.startsWithChar('-')) {
        accidental = -1;
        octaveText = octaveText.substring(1);
    }

    if (octaveText.isNotEmpty() && juce::String(octaveText.getIntValue()) != octaveText) {
        return { currentNote, {} };
    }

    if (octaveText.isNotEmpty()) {
        octave = octaveText.getIntValue();
    }

    return { juce::jlimit(minimumMidiPitch, maximumMidiPitch, (octave + 1) * semitonesPerOctave + naturalSemitones[letterIndex] + accidental), {} };
}
