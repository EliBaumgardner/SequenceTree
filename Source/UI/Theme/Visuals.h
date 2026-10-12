#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

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
