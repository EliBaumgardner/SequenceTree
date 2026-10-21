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

    juce::Value quaverCount { minimumCountLimit };

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
