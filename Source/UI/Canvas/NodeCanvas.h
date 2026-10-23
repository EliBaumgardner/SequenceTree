#pragma once

#include <unordered_set>

#include "../../Graph/RTData.h"
#include "../../Util/NodeInfo.h"
#include "NodeCanvasTreeListener.h"
#include "ValueField.h"
#include "AudioCommandDrainer.h"
#include "NodeManager.h"
#include "ArrowManager.h"
#include "CanvasHitTester.h"
#include "EncapsulationView.h"
#include "OrnamentMarks.h"
#include "../../Util/ArrowInfo.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

/*
    The component that draws the graph: every Node and Arrow is a child of it, and it holds what
    those children share: the view and model transforms of the pan and zoom, the snap grid
    (gridOrigin, gridSpacing and whether it is visible), the selection rectangle, the active
    tool and the queue of pending updates. The parts that do its work are members: NodeManager
    and ArrowManager for the components, NodeCanvasTreeListener for the queue, AudioCommandDrainer
    for the audio thread's commands, CanvasHitTester for picking, EncapsulationView, OrnamentMarks
    and ValueField.

    NodeCanvasTreeListener adds an AsyncUpdate for each change to the graph's tree, and
    handleAsyncUpdate applies them on the message thread and drains the audio commands; the
    editor's per-frame poll and SelectionGestures call it. The canvas never changes the graph
    itself. Only setActiveTool changes the CanvasTool (none, paint, span, note preview or
    ornament): it sets the cursor, stops a note preview that was running and tells the bar
    through onActiveToolChanged. rebuildFromNodeMap clears the canvas and builds every node and
    arrow from a restored graph. setProcessorPlayback stores the play state for the processor and
    starts or pauses the arrows' progress animation.

    NodeController routes mouse events on it; the editor and the bars hold it by reference.
*/

class Node;
class RootNode;
class Arrow;
class SequenceTreeAudioProcessor;
class GraphState;
class RTGraphBuilder;

class NodeCanvas : public juce::Component
{
public:

    enum class CanvasTool;

    NodeCanvas(SequenceTreeAudioProcessor& processor, GraphState& graphState, RTGraphBuilder& rtGraphBuilder, juce::UndoManager& undoManager,
               ArsNovaUI::CustomLookAndFeel& lookAndFeel);

    ~NodeCanvas() override;

    void paint(juce::Graphics& graphics) override;

    void clearCanvas ();
    void rebuildFromNodeMap(const juce::ValueTree& stateTree);
    void childrenChanged () override;

    void handleAsyncUpdate ();
    void cancelPendingUpdatesFor(int nodeId);

    void setActiveTool(CanvasTool tool);

    void showGrid();
    void hideGrid();

    void applyViewTransform  (const juce::AffineTransform& newViewTransform, float zoom);
    void setProcessorPlayback(bool isPlaying);

    juce::Point<int> snapPointToGrid(juce::Point<int> point) const;

    enum class AsyncUpdateType
    {
        None,
        NodeAdded,
        NodeRemoved,
        NodeMoved,
        ValueChanged,
        DanglingArrowsChanged,
        ArrowAdded,
        ArrowRemoved,
        ArrowInfoChanged,
        ArrowDurationChanged,
        NodeColourChanged,
        OrnamentChanged
    };

    enum class CanvasTool { None, Paint, Span, NotePreview, Ornament };

    struct AsyncUpdate
    {
        AsyncUpdateType type = AsyncUpdateType::None;

        int nodeId     = -1;
        int rootNodeId = -1;
    };

    struct NodePair
    {
        int parentNodeId;
        int childNodeId;
    };

    SequenceTreeAudioProcessor& processor;
    GraphState&                 graphState;
    RTGraphBuilder&             rtGraphBuilder;

    juce::Colour canvasColour = juce::Colours::white;

    juce::AffineTransform viewTransform;
    juce::AffineTransform modelTransform;

    CanvasTool activeTool = CanvasTool::None;

    std::function<void(CanvasTool)> onActiveToolChanged;

    bool start         = false;
    bool quaverRepeat  = false;
    bool gridVisible   = false;
    bool gridOriginSet = false;

    int quaverTraversalId = 1;
    int spanAnchorNodeId  = -1;

    juce::Value quaverCount { NodeInfo::minimumCountLimit };

    static constexpr int spanCursorSize   = 16;
    static constexpr int quaverCursorSize = 24;

    juce::Point<float> gridOrigin { 0.0f, 0.0f };

    float gridSpacing = ArrowInfo::pixelsPerGridSpace;

    juce::Rectangle<int> selectionBounds;

    std::vector<AsyncUpdate> asyncUpdates;

    NodeCanvasTreeListener treeListener { *this };

    ValueField valueField { *this };

    NodeManager  nodeManager;
    ArrowManager arrowManager;

    AudioCommandDrainer drainer;
    CanvasHitTester     hitTester { *this };
    EncapsulationView   encapsulationView;
    OrnamentMarks       ornamentMarks;

private:

    juce::MouseCursor spanCursor  () const;
    juce::MouseCursor quaverCursor() const;
};
