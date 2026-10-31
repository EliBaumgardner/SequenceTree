#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ArrowGestures.h"
#include "NodeGestures.h"

/*
    The router for the mouse events every node canvas handles the same way, over an EditCanvas
    and its CanvasDocument. On mouseDown it works out which feature a press belongs to and
    records it in gesture, so mouseDrag and mouseUp go to the same place. On the empty canvas
    that is, in order: a Shift right-click removing the arrow under it, grabbing a dangling
    arrow's tip, editing an arrow's duration label, grabbing an arrow's head to move the node it
    points at, selecting an arrow's shaft, and otherwise clearing the selection, panning the
    DynamicPort the canvas sits in and, with Shift, adding a root. On a node it is a Ctrl drag
    that scrubs the node's value (dragValue), a Shift drag that starts an arrow, a Shift
    right-click that removes the node, and otherwise selecting it; a drag then moves the node or
    grows a child (NodeGestures). A plain click opens the node's editor. It also keeps node and
    arrow hover.

    The features are the members it owns, nodeGestures and arrowGestures. NodeController and
    OrnamentFigureController derive from it: each handles its own presses first and passes the
    rest to these functions.
*/

class EditCanvas;
class CanvasDocument;
class Node;

class CanvasController : public juce::MouseListener
{
public:

    CanvasController(EditCanvas& canvas, CanvasDocument& document, juce::UndoManager& undoManager);

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

    enum class Gesture { Idle, EditingValue, MovingArrowHead, ArrowSelected, MovingDanglingTip, DrawingArrow };

    ArrowGestures arrowGestures;
    NodeGestures  nodeGestures;

    Gesture gesture = Gesture::Idle;

    juce::Component::SafePointer<Node> draggingValueNode;

private:

    void dragValue(const juce::MouseEvent& e);

    void handleCanvasMouseDown(const juce::MouseEvent& e);
    void handleCanvasMouseDrag(const juce::MouseEvent& e);
    void handleNodeMouseDown  (const juce::MouseEvent& e, Node& node);
    void handleNodeMouseDrag  (const juce::MouseEvent& e, Node& node);

    static constexpr int dragThreshold = 5;

    EditCanvas&        canvas;
    CanvasDocument&    document;
    juce::UndoManager& undoManager;

    double dragStartValue = 0.0;
};
