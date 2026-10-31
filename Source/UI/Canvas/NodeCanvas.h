#pragma once

#include <unordered_set>

#include "../../Graph/RTData.h"
#include "../../Util/NodeInfo.h"
#include "EditCanvas.h"
#include "ValueField.h"
#include "AudioCommandDrainer.h"
#include "EncapsulationView.h"
#include "OrnamentMarks.h"
#include "../../Util/ArrowInfo.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

/*
    The EditCanvas for the graph: it shows GraphDocument's nodeMap and adds what only the graph
    has. That is the active tool (none, paint, span, note preview or ornament), which only
    setActiveTool changes: it sets the cursor, stops a note preview that was running and tells
    the bar through onActiveToolChanged; the selection rectangle; and the parts AudioCommandDrainer
    for the audio thread's commands, EncapsulationView, OrnamentMarks and ValueField.

    Its hooks build the Node class each node type needs (RootNode, Modulator, TraversalFlagNode or
    Encapsulator), bind each node to its MIDI note, colour and the display mode, collapse and
    refresh groups as nodes come and go, draw a member of a collapsed group at its group, and
    refresh ornament marks and group colours. handleAsyncUpdate drains the audio commands before
    it applies the queued updates; the editor's per-frame poll and SelectionGestures call it.
    rebuildFromNodeMap also collapses every group and puts the grid origin on a root.
    setProcessorPlayback stores the play state for the processor and starts or pauses the
    arrows' progress animation.

    NodeController routes mouse events on it; the editor and the bars hold it by reference.
*/

class Node;
class RootNode;
class Arrow;
class SequenceTreeAudioProcessor;
class GraphState;
class GraphDocument;
class RTGraphBuilder;

class NodeCanvas : public EditCanvas
{
public:

    enum class CanvasTool;

    NodeCanvas(SequenceTreeAudioProcessor& processor, GraphState& graphState, GraphDocument& graphDocument, RTGraphBuilder& rtGraphBuilder,
               ArsNovaUI::CustomLookAndFeel& lookAndFeel);

    ~NodeCanvas() override;

    void paint(juce::Graphics& graphics) override;

    void rebuildFromNodeMap(const juce::ValueTree& stateTree) override;
    void handleAsyncUpdate () override;

    void nodeCreated  (Node& node) override;
    void nodeAdded    (Node& node) override;
    void nodeRemoving (Node& node) override;
    void prepareArrow (Arrow& arrow) override;
    void updateApplied(const AsyncUpdate& update) override;

    void setActiveTool(CanvasTool tool);

    void applyViewTransform  (const juce::AffineTransform& newViewTransform, float zoom);
    void setProcessorPlayback(bool isPlaying);

    juce::Point<int> drawnPosition(const juce::ValueTree& nodeTree) const override;

    std::unique_ptr<Node> makeNode(const juce::ValueTree& nodeTree) override;

    enum class CanvasTool { None, Paint, Span, NotePreview, Ornament };

    SequenceTreeAudioProcessor& processor;
    GraphState&                 graphState;
    RTGraphBuilder&             rtGraphBuilder;

    juce::Colour canvasColour = juce::Colours::white;

    CanvasTool activeTool = CanvasTool::None;

    std::function<void(CanvasTool)> onActiveToolChanged;

    bool start        = false;
    bool quaverRepeat = false;

    int quaverTraversalId = 1;
    int spanAnchorNodeId  = -1;

    juce::Value quaverCount { NodeInfo::minimumCountLimit };

    static constexpr int spanCursorSize   = 16;
    static constexpr int quaverCursorSize = 24;

    juce::Rectangle<int> selectionBounds;

    ValueField valueField { *this };

    AudioCommandDrainer drainer;
    EncapsulationView   encapsulationView;
    OrnamentMarks       ornamentMarks;

private:

    juce::MouseCursor spanCursor  () const;
    juce::MouseCursor quaverCursor() const;
};
