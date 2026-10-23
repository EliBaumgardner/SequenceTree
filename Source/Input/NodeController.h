#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include "SelectionGestures.h"
#include "OrnamentApplier.h"
#include "ArrowGestures.h"
#include "NodeGestures.h"
#include "EncapsulationGestures.h"

/*
    The router for every mouse event on the canvas and its nodes. On mouseDown it works out which
    feature the press belongs to: the active canvas tool first (paint, span, note preview,
    ornament), then an ornament mark, then the arrow hit tests, then the node or the empty canvas.
    It hands the press to the class that implements that feature and records the choice in
    gesture, so mouseDrag and mouseUp go to the same class.

    The features are the members it owns: selectionGestures, ornamentApplier, arrowGestures,
    nodeGestures and encapsulationGestures. A new canvas feature is another such class and another
    branch here. What it does itself is small: forwarding paint-tool strokes to
    NodeCanvas::valueField, pushing note preview requests to TraversalSession::previewRequests,
    the Ctrl-drag that changes a node's value (dragValue), and node hover.

    PluginEditor builds it after the canvas. Titlebar sets arrowGestures.arrowMode and
    nodeGestures.creationMode, and PluginEditor's key handler calls selectionGestures.
*/

class NodeCanvas;

class Node;

class GraphState;

class TraversalSession;

class RTGraphBuilder;

class OrnamentLibraries;



class NodeController : public juce::MouseListener {

public:

    NodeController(NodeCanvas& nodeCanvas, GraphState& graphState, juce::UndoManager& undoManager, TraversalSession& traversalSession,
                   RTGraphBuilder& rtGraphBuilder, OrnamentLibraries& ornamentLibraries);
    ~NodeController() override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

    SelectionGestures     selectionGestures;
    OrnamentApplier       ornamentApplier;
    ArrowGestures         arrowGestures;
    NodeGestures          nodeGestures;
    EncapsulationGestures encapsulationGestures;

private:

    void dragValue(const juce::MouseEvent& e);

    void handleCanvasMouseDown(const juce::MouseEvent& e);
    void handleCanvasMouseDrag(const juce::MouseEvent& e);
    void handleNodeMouseDown  (const juce::MouseEvent& e, Node& node);
    void handleNodeMouseDrag  (const juce::MouseEvent& e, Node& node);

    enum class Gesture {
        Idle,
        EditingValue,
        MovingArrowHead,
        ArrowSelected,
        MovingDanglingTip,
        DrawingArrow,
        BoxSelecting
    };

    static constexpr int dragThreshold = 5;

    GraphState&        graphState;
    juce::UndoManager& undoManager;
    TraversalSession&  traversalSession;
    RTGraphBuilder&    rtGraphBuilder;
    NodeCanvas&        nodeCanvas;


    Gesture gesture = Gesture::Idle;

    juce::Component::SafePointer<Node> draggingValueNode;

    double dragStartValue = 0.0;
};
