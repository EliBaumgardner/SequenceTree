/*
  ==============================================================================

    ObjectObjectController
    Created: 6 May 2025 8:38:35pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include "SelectionGestures.h"
#include "OrnamentApplier.h"
#include "ArrowGestures.h"
#include "NodeGestures.h"
#include "EncapsulationGestures.h"


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
