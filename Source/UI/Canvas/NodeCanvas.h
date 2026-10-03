#pragma once

#include <unordered_set>

#include "../../Graph/RTData.h"
#include "DynamicPort.h"
#include "../../Util/NodeInfo.h"
#include "../../Util/ApplicationContext.h"
#include "NodeCanvasTreeListener.h"
#include "ValueField.h"
#include "AudioCommandDrainer.h"
#include "NodeManager.h"
#include "ArrowManager.h"
#include "CanvasHitTester.h"
#include "EncapsulationView.h"
#include "../../Util/ArrowInfo.h"

class Node;
class RootNode;
class Arrow;

class NodeCanvas : public juce::Component, public juce::AsyncUpdater
{
public:

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
        NodeColourChanged
    };

    enum class QuaverMode { Off, Preview };

    struct AsyncUpdate
    {
        AsyncUpdateType type       = AsyncUpdateType::None;
        int             nodeId     = -1;
        int             rootNodeId = -1;
    };

    struct NodePair
    {
        int parentNodeId;
        int childNodeId;
    };

    explicit NodeCanvas(const ApplicationContext& context);
    ~NodeCanvas() override;

    void paint(juce::Graphics& graphics) override;

    void clearCanvas();
    void enqueueAsyncUpdate(const AsyncUpdate& update);
    void setProcessorPlayback(bool isPlaying);
    void rebuildFromNodeMap(const juce::ValueTree& stateTree);
    void handleAsyncUpdate() override;
    void setPaintMode(bool enabled);
    void setSpanMode(bool enabled);
    void setQuaverMode(QuaverMode mode);
    void showGrid();
    void hideGrid();
    juce::Point<int> snapPointToGrid(juce::Point<int> point) const;
    void cancelPendingUpdatesFor(int nodeId);

    const ApplicationContext& applicationContext;

    juce::Colour canvasColour = juce::Colours::white;

    bool       start      = false;
    bool       paintMode  = false;
    bool       spanMode   = false;
    QuaverMode quaverMode = QuaverMode::Off;

    int         quaverTraversalId = 1;
    juce::Value quaverCount { minimumCountLimit };
    bool        quaverRepeat = false;

    int spanAnchorNodeId = -1;

    static constexpr int spanCursorSize   = 16;
    static constexpr int quaverCursorSize = 24;

    bool               gridVisible   = false;
    bool               gridOriginSet = false;
    juce::Point<float> gridOrigin    { 0.0f, 0.0f };
    float              gridSpacing   = ArrowInfo::pixelsPerGridSpace;

    juce::Rectangle<int> selectionBounds;

    std::vector<AsyncUpdate> asyncUpdates;

    NodeCanvasTreeListener treeListener { *this };

    ValueField valueField { *this };

    NodeManager         nodeManager       { *this, applicationContext };
    ArrowManager        arrowManager      { *this, applicationContext };
    AudioCommandDrainer drainer           { *this, applicationContext };
    CanvasHitTester     hitTester         { *this };
    EncapsulationView   encapsulationView { *this, applicationContext };
};
