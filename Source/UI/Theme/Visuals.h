#pragma once

#include <juce_graphics/juce_graphics.h>
#include <map>
#include <vector>

enum class SymbolMark { None, Symbol, Missing };

struct SymbolStroke
{
    juce::Path path;
    float      width = 0.0f;
};

struct CanvasGrid
{
    juce::Rectangle<float> bounds;
    juce::AffineTransform  modelTransform;
    juce::AffineTransform  viewTransform;
    juce::Point<float>     origin;
    float                  spacing   = 0.0f;
    bool                   visible   = false;
    bool                   originSet = false;
};

struct NodeVisual
{
    juce::Rectangle<float>             bounds;
    juce::Colour                       colour;
    const std::map<int, juce::Colour>& highlights;
    bool                               isHovered    = false;
    bool                               isSelected   = false;
    bool                               isOutlined   = false;
    bool                               hasOuterRing = false;
    bool                               hasInnerRim  = false;
    juce::Colour                       outerRingColour;
};

struct ArrowGeometry
{
    juce::Point<float> centre;
    juce::Point<float> start;
    juce::Point<float> tip;
    juce::Point<float> direction;
    juce::Point<float> chord;
    juce::Point<float> control1;
    juce::Point<float> control2;

    float length   = 0.0f;
    bool  straight = true;
    bool  drawHead = false;
    bool  valid    = false;
};

struct ArrowLabel
{
    juce::Point<float> centre;
    float              angle = 0.0f;
};

struct ArrowTrail
{
    enum class Stroke { Solid, Dashed };

    juce::Colour colour;
    float        progress = 0.0f;
    Stroke       stroke   = Stroke::Solid;
};

struct ArrowVisual
{
    enum class ShaftStyle { Solid, Dashed, Ruler };
    enum class HeadStyle  { Filled, Outlined };

    ArrowGeometry           geometry;
    ArrowLabel              label;
    juce::Path              shaft;
    juce::Point<float>      origin;
    juce::String            labelText;
    std::vector<ArrowTrail> trails;
    ShaftStyle              shaftStyle   = ShaftStyle::Solid;
    HeadStyle               head         = HeadStyle::Filled;
    float                   headLength   = 0.0f;
    float                   rulerOpacity = 0.0f;
    float                   rulerSpacing = 0.0f;
    bool                    emphasised   = false;
    bool                    ghost        = false;
    bool                    showsLabel   = false;
};
