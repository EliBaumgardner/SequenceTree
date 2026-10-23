#pragma once

#include <ArsNovaUI/Editors/Formats/ValueFormat.h>
#include "../../../Graph/RTData.h"

/*
    The way a traversal flag's value is typed and shown. The value is a whole number from -9999
    to 9999, the traversal id, plus an instance number kept in the TraversalInstance property
    beside it. The text for it is a sign, the id and a letter for the instance (a for 0, b for 1
    and so on): "+3b" is id 3, instance 1, spawning it, and "-3b" removes it. A value of 0 shows
    nothing, and text that does not start with + or - clears the value.

    Two static functions work on lists of traversals rather than one value. parseKeys reads every
    number in a text, each with an optional instance letter after it, into TraversalKeys, so
    "1 2b, 3" gives three; describe writes one TraversalKey as "2b". RootRectangle accepts the
    letters in instanceLetters, RootNode reads and writes its traversal list with parseKeys and
    describe, and AllowedTraversalsMenu labels its rows with describe. TraversalFlagNode uses
    the format itself.
*/

class TraversalFlagFormat : public ArsNovaUI::NumberFormat
{
public:

    TraversalFlagFormat();

    ArsNovaUI::InputRestrictions restrictions() const override;
    juce::String                 text (const ArsNovaUI::ValueBinding& binding, ArsNovaUI::TextPurpose purpose) const override;
    ArsNovaUI::ParsedValue       parse(const ArsNovaUI::ValueBinding& binding, const juce::String& enteredText) const override;

    static std::vector<TraversalKey> parseKeys(const juce::String& text);
    static juce::String              describe (const TraversalKey& key);

    static const juce::String instanceLetters;
};
