#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Util/ApplicationContext.h"

class ColourPicker : public juce::Component
{
public:

    enum class DragTarget { None, SaturationBrightness, Hue };

    explicit ColourPicker(const ApplicationContext& context);
    ~ColourPicker() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    void showColour(juce::Colour colour);
    void renderSaturationBrightnessImage();

    static constexpr int   presetCount          = 8;
    static constexpr float heightToWidthRatio   = 1.41f;
    static constexpr float paddingRatio         = 0.05f;
    static constexpr float sectionGapRatio      = 0.04f;
    static constexpr float hueStripHeightRatio  = 0.08f;
    static constexpr float swatchRowHeightRatio = 0.14f;
    static constexpr float presetRowHeightRatio = 0.1f;
    static constexpr float presetGapRatio       = 0.02f;
    static constexpr float presetGlyphRatio     = 0.4f;
    static constexpr float cursorRadiusRatio    = 0.035f;
    static constexpr float cursorRingWidth      = 1.5f;
    static constexpr float hexTextHeightRatio   = 0.45f;

    std::function<void(juce::Colour)> onColourPicked;

    juce::Colour originalColour = juce::Colours::white;

    float hue        = 0.0f;
    float saturation = 0.0f;
    float brightness = 1.0f;

    DragTarget dragTarget = DragTarget::None;

    juce::Rectangle<int> saturationBrightnessArea;
    juce::Rectangle<int> hueArea;
    juce::Rectangle<int> originalArea;
    juce::Rectangle<int> currentArea;
    juce::Rectangle<int> presetArea;

    juce::Image saturationBrightnessImage;
    juce::Image hueImage;

private:

    const ApplicationContext& applicationContext;
};

class ColourSelector : public juce::Component, public juce::SettableTooltipClient
{
public:

    enum class Shape { Square, Circle };

    explicit ColourSelector(const ApplicationContext& context);

    void paint(juce::Graphics& graphics) override;

    void mouseDown(const juce::MouseEvent& event) override;

    static constexpr float pickerWidthRatio = 0.3f;

    std::function<void(juce::Colour)> onColourPicked;

    juce::Colour colour = juce::Colours::white;
    Shape        shape  = Shape::Square;

    ColourPicker                      picker;
    std::unique_ptr<juce::CallOutBox> pickerBox;
};
