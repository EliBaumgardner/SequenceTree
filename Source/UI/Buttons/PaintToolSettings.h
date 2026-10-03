#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Util/ApplicationContext.h"
#include "../Menus/ColourSelector.h"
#include "../Editors/LabeledEditor.h"
#include "../Canvas/ValueField.h"
#include "IconButton.h"

class PaintToolSettings : public juce::Component
{
public:

    static constexpr float minBrushFlow = 0.002f;
    static constexpr float maxBrushFlow = 0.25f;

    static constexpr float minBrushRadius = 1.0f;
    static constexpr float maxBrushRadius = 200.0f;

    static constexpr float cellWidthRatio  = 0.09f;
    static constexpr float labelWidthRatio = 0.144f;

    const ApplicationContext& context;

    juce::Colour pitchColour    = juce::Colours::red;
    juce::Colour velocityColour = juce::Colours::green;
    juce::Colour durationColour = juce::Colours::blue;

    ValueField::PaintLayer paintLayer = ValueField::PaintLayer::Pitch;

    IconButton     paintTool;
    ColourSelector colourSelector { context };
    LabeledEditor  sizeField      { context };
    LabeledEditor  flowField      { context };

    explicit PaintToolSettings(const ApplicationContext& context);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void componentCallBack();

    void configureValueFields(ValueField &valueField, float brushFlow);

    juce::Colour& paintLayerColour();
    void setPaintMode(ValueField::PaintLayer layer);
};
