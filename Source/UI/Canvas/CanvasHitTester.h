#pragma once

#include <juce_graphics/juce_graphics.h>

/*
    The picking queries of the canvas: given a point in canvas coordinates, each function returns
    the closest visible arrow or node of one kind within a radius, or nullptr. arrowNear measures
    to the line from an arrow's start node to its tip, arrowHeadNear to the head of an arrow
    between two nodes, danglingHeadNear to the tip of a dangling arrow, and arrowLabelNear to the
    centre of a duration label. nodeNear and rootNear measure to node centres, rootNear only for
    root nodes and nodeContaining only for nodes the point is inside (plain or root nodes, which
    are the ones an arrow may connect to). A node's id in excludeId is skipped.

    It reads the nodes and arrows through EditCanvas::nodeManager and arrowManager and changes
    nothing. EditCanvas owns it; the gesture classes, NodeController and OrnamentCanvas call it.
*/

class EditCanvas;
class Arrow;
class Node;

class CanvasHitTester
{
public:

    explicit CanvasHitTester(EditCanvas& canvas) : canvas(canvas)
    {
    }

    Arrow* arrowNear       (juce::Point<float> point, float radius) const;
    Arrow* arrowHeadNear   (juce::Point<float> point, float radius) const;
    Arrow* danglingHeadNear(juce::Point<float> point, float radius) const;
    Arrow* arrowLabelNear  (juce::Point<float> point, float radius) const;

    Node* nodeNear      (juce::Point<float> point, float radius, int excludeId) const;
    Node* rootNear      (juce::Point<float> point, float radius, int excludeId) const;
    Node* nodeContaining(juce::Point<float> point, int excludeId) const;

    static float distanceToSegment(juce::Point<float> point, juce::Point<float> segmentStart, juce::Point<float> segmentEnd);

private:

    EditCanvas& canvas;
};
