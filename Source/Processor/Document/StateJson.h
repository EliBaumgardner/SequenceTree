#pragma once

#include <juce_core/juce_core.h>

#include <cstddef>
#include <memory>

/*
    Saves the plugin's state as JSON text and reads it back. The state is a ValueTree, which
    ValueTree::createXml turns into an XmlElement; toVar turns that element into a JSON value,
    which JSON::toString writes out, and read turns JSON text back into an element for
    ValueTree::fromXml, so every property keeps exactly the text the XML form gave it. Each
    element becomes an object with its tag as "type", its attributes as the "properties" object,
    and its child elements as the "children" array.

    read returns nothing when the bytes are not JSON, which includes the binary form older
    versions saved with AudioProcessor::copyXmlToBinary, so the caller falls back to
    AudioProcessor::getXmlFromBinary for those. It also returns nothing when the text nests deeper
    than maximumNestingDepth, which it checks before parsing so a hostile file cannot overflow the
    parser's stack, and when an element's "properties" is not an object or its "children" is not
    an array.

    SequenceTreeAudioProcessor uses it in getStateInformation and setStateInformation, which also
    write and read the .sqtree patch files.
*/

class StateJson
{
public:

    static juce::var toVar(const juce::XmlElement& element);

    static std::unique_ptr<juce::XmlElement> read(const void* data, int sizeInBytes);

    static constexpr const char* patchExtension = ".sqtree";

    static constexpr int maximumNestingDepth = 256;

private:

    static std::unique_ptr<juce::XmlElement> toElement(const juce::var& value);

    static int nestingDepth(const char* bytes, std::size_t byteCount);

    static constexpr const char* typeKey       = "type";
    static constexpr const char* propertiesKey = "properties";
    static constexpr const char* childrenKey   = "children";
};
