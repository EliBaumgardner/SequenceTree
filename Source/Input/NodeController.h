#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include "SelectionGestures.h"
#include "OrnamentApplier.h"
#include "CanvasController.h"
#include "EncapsulationGestures.h"
#include "ArrowMenu.h"

/*
    The router for every mouse event on the graph canvas and its nodes: a CanvasController that
    handles the graph's own presses before it passes the rest on. On mouseDown that is the active
    canvas tool first (paint, span, note preview, ornament), then an ornament mark, then a plain
    right-click (the arrow menu near an arrow, the selection menu elsewhere on the canvas, and
    expanding or collapsing a group on a node), then a Shift drag in arrow mode on the empty
    canvas, which draws a selection box. mouseDrag and mouseUp follow the same choice, and while
    the paint tool is active there is no hover.

    Its own features are the members it owns: selectionGestures, ornamentApplier,
    encapsulationGestures and arrowMenu; the features every canvas shares come from
    CanvasController. A new graph feature is another such class and another branch here; a
    feature for every canvas goes into CanvasController. It forwards paint-tool strokes to
    NodeCanvas::valueField and pushes note preview requests to TraversalSession::previewRequests.

    PluginEditor builds it after the canvas. Titlebar sets arrowGestures.arrowMode, and
    PluginEditor's key handler calls selectionGestures.
*/

class NodeCanvas;

class Node;

class GraphState;

class GraphDocument;

class TraversalSession;

class RTGraphBuilder;

class OrnamentLibraries;



class NodeController : public CanvasController
{
public:

    NodeController(NodeCanvas& nodeCanvas, GraphState& graphState, GraphDocument& graphDocument, juce::UndoManager& undoManager,
                   TraversalSession& traversalSession, RTGraphBuilder& rtGraphBuilder, OrnamentLibraries& ornamentLibraries);
    ~NodeController() override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

    SelectionGestures     selectionGestures;
    OrnamentApplier       ornamentApplier;
    EncapsulationGestures encapsulationGestures;
    ArrowMenu             arrowMenu;

private:

    enum class GraphGesture { None, BoxSelecting };

    TraversalSession& traversalSession;
    RTGraphBuilder&   rtGraphBuilder;
    NodeCanvas&       nodeCanvas;

    GraphGesture graphGesture = GraphGesture::None;
};
