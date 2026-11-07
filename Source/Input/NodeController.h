#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include "Gestures/SelectionGestures.h"
#include "OrnamentApplier.h"
#include <ArsNovaUI/Canvas/CanvasController.h>
#include "Gestures/EncapsulationGestures.h"
#include "Gestures/NotePreviewGestures.h"
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
    encapsulationGestures and arrowMenu. It also routes to notePreviewGestures, which PluginEditor
    owns because BottomBar and TraversalMenu configure it. The features every canvas shares come
    from CanvasController. A new graph feature is another such class and another branch here; a
    feature for every canvas goes into CanvasController. It forwards paint-tool strokes to
    NodeCanvas::valueField, and sets NodeCanvas::onToolLeft to clear the span anchor and stop a
    note preview when the canvas leaves those tools.

    PluginEditor builds it after the canvas. Titlebar sets arrowGestures.arrowMode, and
    PluginEditor's key handler calls selectionGestures.
*/

class NodeCanvas;

class Node;


class GraphDocument;


class OrnamentLibraries;



class NodeController : public ArsNovaUI::CanvasController
{
public:

    NodeController(NodeCanvas& nodeCanvas, GraphDocument& graphDocument, NotePreviewGestures& notePreviewGestures, OrnamentLibraries& ornamentLibraries);
    ~NodeController() override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

    SelectionGestures     selectionGestures;
    EncapsulationGestures encapsulationGestures;

    OrnamentApplier ornamentApplier;
    ArrowMenu       arrowMenu;

    NotePreviewGestures& notePreviewGestures;

private:

    enum class GraphGesture { None, BoxSelecting };

    NodeCanvas& nodeCanvas;

    GraphGesture graphGesture = GraphGesture::None;
};
