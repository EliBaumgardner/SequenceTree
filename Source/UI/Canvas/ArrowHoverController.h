#pragma once

#include "../../Util/PluginModules.h"

class NodeCanvas;

class ArrowHoverController
{
public:

    explicit ArrowHoverController(NodeCanvas& canvas) : canvas(canvas) {}

    void update(juce::Point<float> cursor) const;

private:

    NodeCanvas& canvas;

    static constexpr float flagProximityRadius = 28.0f;
    static constexpr float boldHoverRadius      = 8.0f;
};
