#pragma once

#include <juce_graphics/juce_graphics.h>

class NodeCanvas;
class Arrow;
class Node;

class CanvasHitTester
{
public:

    explicit CanvasHitTester(NodeCanvas& canvas) : canvas(canvas)
    {
    }

    Arrow* arrowNear        (juce::Point<float> point, float radius) const;
    static float distanceToSegment(juce::Point<float> point, juce::Point<float> segmentStart, juce::Point<float> segmentEnd);
    Arrow* arrowHeadNear    (juce::Point<float> point, float radius) const;
    Arrow* danglingHeadNear (juce::Point<float> point, float radius) const;
    Arrow* arrowLabelNear   (juce::Point<float> point, float radius) const;
    Node*  nodeNear (juce::Point<float> point, float radius, int excludeId) const;
    Node*  rootNear (juce::Point<float> point, float radius, int excludeId) const;
    Node*  nodeContaining (juce::Point<float> point, int excludeId) const;

private:

    NodeCanvas& canvas;
};
