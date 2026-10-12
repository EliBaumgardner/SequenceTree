#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <map>

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
    bool                               isHovered             = false;
    bool                               isSelected            = false;
    bool                               isOutlined            = false;
    bool                               isEncapsulationRinged = false;
    bool                               hasInnerRim           = false;
    juce::Colour                       encapsulationRingColour;
};
