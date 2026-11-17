#include "StateJson.h"

std::unique_ptr<juce::XmlElement> StateJson::read(const void* data, int sizeInBytes)
{
    const juce::String text = juce::String::createStringFromData(data, sizeInBytes).trimStart();

    if (! text.startsWithChar('{')) {
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

    if (! value.isObject() || tagName.isEmpty()) {
        return nullptr;
    }

    auto element = std::make_unique<juce::XmlElement>(tagName);

    if (const juce::DynamicObject* properties = value.getProperty(propertiesKey, juce::var()).getDynamicObject()) {
        for (const juce::NamedValueSet::NamedValue& property : properties->getProperties()) {
            element->setAttribute(property.name.toString(), property.value.toString());
        }
    }

    const juce::var children = value.getProperty(childrenKey, juce::var());

    for (int index = 0; index < children.size(); ++index) {
        std::unique_ptr<juce::XmlElement> child = toElement(children[index]);

        if (child == nullptr) {
            return nullptr;
        }

        element->addChildElement(child.release());
    }

    return element;
}
