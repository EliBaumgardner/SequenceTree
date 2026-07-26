#pragma once

#include "../../Util/PluginModules.h"

class NodeCanvas;
class Arrow;
class Node;

class CanvasHitTester
{
public:

    explicit CanvasHitTester(NodeCanvas& canvas) : canvas(canvas) {}

    Arrow* arrowNear        (juce::Point<float> point, float radius) const;
    Arrow* arrowHeadNear    (juce::Point<float> point, float radius) const;
    Arrow* danglingHeadNear (juce::Point<float> point, float radius) const;

    Node*  nodeNear (juce::Point<float> point, float radius, int excludeId) const;
    Node*  rootNear (juce::Point<float> point, float radius, int excludeId) const;

    static float distanceToSegment(juce::Point<float> p, juce::Point<float> a, juce::Point<float> b);

private:

    NodeCanvas& canvas;
};
