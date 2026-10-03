#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ValueEditor.h"
#include "../Theme/Theme.h"

class CaptionLabel : public juce::Label
{
public:

    CaptionLabel()
    {
        setJustificationType(juce::Justification::centredLeft);
        setBorderSize({});
        setMinimumHorizontalScale(1.0f);
    }

    void parentHierarchyChanged() override
    {
        if (const auto* theme = dynamic_cast<const Theme*>(&getLookAndFeel())) {
            setColour(juce::Label::textColourId, theme->captionColour);
        }
    }
};

class LabeledEditor : public juce::Component
{
public:

    explicit LabeledEditor(const ApplicationContext& context)
        : editor(context)
    {
        addAndMakeVisible(label);
        addAndMakeVisible(editor);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        label.setBounds(bounds.removeFromLeft(labelWidth));

        bounds.removeFromLeft(gap);

        editor.setBounds(bounds);
    }

    CaptionLabel label;
    ValueEditor  editor;

    int labelWidth = 0;
    int gap        = 0;
};
