#include "StateJson.h"

#include <algorithm>
#include <cstring>

std::unique_ptr<juce::XmlElement> StateJson::read(const void* data, int sizeInBytes)
{
    const juce::String text = juce::String::createStringFromData(data, sizeInBytes).trimStart();

    const char* bytes = text.toRawUTF8();

    const std::size_t byteCount = std::strlen(bytes);

    if (! text.startsWithChar('{') || nestingDepth(bytes, byteCount) > maximumNestingDepth) {
        return nullptr;
    }

    juce::var parsed;

    if (! juce::JSON::parse(text, parsed).wasOk()) {
        return nullptr;
    }

    return toElement(parsed);
}

juce::var StateJson::toVar(const juce::XmlElement& element)
{
    auto object     = std::make_unique<juce::DynamicObject>();
    auto properties = std::make_unique<juce::DynamicObject>();

    juce::Array<juce::var> children;

    for (int index = 0; index < element.getNumAttributes(); ++index) {
        properties->setProperty(element.getAttributeName(index), element.getAttributeValue(index));
    }

    for (const juce::XmlElement* child : element.getChildIterator()) {
        children.add(toVar(*child));
    }

    object->setProperty(typeKey, element.getTagName());
    object->setProperty(propertiesKey, juce::var(properties.release()));
    object->setProperty(childrenKey, children);

    return juce::var(object.release());
}

std::unique_ptr<juce::XmlElement> StateJson::toElement(const juce::var& value)
{
    const juce::String tagName = value.getProperty(typeKey, juce::var()).toString();

    const juce::var propertiesValue = value.getProperty(propertiesKey, juce::var());
    const juce::var children        = value.getProperty(childrenKey, juce::var());

    const bool propertiesAreWrongType = ! propertiesValue.isVoid() && ! propertiesValue.isObject();
    const bool childrenAreWrongType   = ! children.isVoid() && ! children.isArray();

    if (! value.isObject() || tagName.isEmpty() || propertiesAreWrongType || childrenAreWrongType) {
        return nullptr;
    }

    auto element = std::make_unique<juce::XmlElement>(tagName);

    if (const juce::DynamicObject* properties = propertiesValue.getDynamicObject()) {
        for (const juce::NamedValueSet::NamedValue& property : properties->getProperties()) {
            element->setAttribute(property.name.toString(), property.value.toString());
        }
    }

    for (int index = 0; index < children.size(); ++index) {
        std::unique_ptr<juce::XmlElement> child = toElement(children[index]);

        if (child == nullptr) {
            return nullptr;
        }

        element->addChildElement(child.release());
    }

    return element;
}

int StateJson::nestingDepth(const char* bytes, std::size_t byteCount)
{
    int depth   = 0;
    int deepest = 0;

    bool insideString = false;
    bool escaped      = false;

    for (std::size_t index = 0; index < byteCount; ++index) {
        const char character = bytes[index];

        if (escaped) {
            escaped = false;
        }
        else if (insideString && character == '\\') {
            escaped = true;
        }
        else if (character == '"') {
            insideString = ! insideString;
        }
        else if (! insideString && (character == '{' || character == '[')) {
            ++depth;
            deepest = std::max(deepest, depth);
        }
        else if (! insideString && (character == '}' || character == ']')) {
            --depth;
        }
    }

    return deepest;
}
