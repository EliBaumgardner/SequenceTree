#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Editors/ValueEditor.h"
#include "../../Util/ApplicationContext.h"

class RootRectangle : public juce::Component
{
public:

    explicit RootRectangle(const ApplicationContext& context);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    ValueEditor traversalEditor;
};
