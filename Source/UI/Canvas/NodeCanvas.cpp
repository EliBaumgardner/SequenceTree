#include <juce_gui_basics/juce_gui_basics.h>

#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/GraphState.h"
#include "../../Plugin/PluginProcessor.h"
#include "../Node/Arrow.h"
#include "../../Graph/ValueTreeIdentifiers.h"

#include "../Node/Node.h"
#include "../Node/RootNode.h"
#include "NodeCanvas.h"
#include "../../Audio/EventManager.h"
#include "../../Graph/RTGraphBuilder.h"

#include "../Node/Modulator.h"
#include "../Node/TraversalFlagNode.h"

#include <algorithm>
#include <cmath>

namespace {

void rememberOnce(std::vector<int>& ids, int id)
{
    if (std::find(ids.begin(), ids.end(), id) == ids.end()) {
        ids.push_back(id);
    }
}

}

NodeCanvas::NodeCanvas(ApplicationContext& context) : applicationContext(context)
{
    setPaintingIsUnclipped(true);
    setLookAndFeel(applicationContext.lookAndFeel);
}

NodeCanvas::~NodeCanvas()
{
    clearCanvas();
}


void NodeCanvas::paint(juce::Graphics& g)
{
    CustomLookAndFeel::get(*this).drawCanvas(g, *this);


    if (paintMode && valueField.image.isValid()) {
        g.setImageResamplingQuality(juce::Graphics::mediumResamplingQuality);
        g.drawImage(valueField.image, getLocalBounds().toFloat());
    }

    if (!selectionBounds.isEmpty()) {
        const Theme& theme = CustomLookAndFeel::get(*this);

        g.setColour(theme.selectionBoxColour.withAlpha(0.15f));
        g.fillRect(selectionBounds);

        g.setColour(theme.selectionBoxColour);
        g.drawRect(selectionBounds, 1);
    }
}

void NodeCanvas::enqueueAsyncUpdate(const AsyncUpdate& update)
{
    asyncUpdates.push_back(update);
    triggerAsyncUpdate();
}

void NodeCanvas::cancelPendingUpdatesFor(int nodeId)
{
    asyncUpdates.erase(
        std::remove_if(asyncUpdates.begin(), asyncUpdates.end(),
            [nodeId](const AsyncUpdate& update) {
                return update.nodeId == nodeId
                    && update.type != AsyncUpdateType::NodeRemoved;
            }),
        asyncUpdates.end()
    );
}

void NodeCanvas::handleAsyncUpdate() {
    drainer.drainAll();

    std::vector<AsyncUpdate> pendingUpdates;
    pendingUpdates.swap(asyncUpdates);

    std::vector<int> pitchSyncNodeIds;
    std::vector<int> durationRefreshNodeIds;
    std::vector<int> movedNodeIds;

    for (auto& asyncUpdate  : pendingUpdates) {
        int nodeId = asyncUpdate.nodeId;

        AsyncUpdateType updateType = asyncUpdate.type;

        if (updateType == AsyncUpdateType::NodeAdded) {
            nodeManager.add(nodeId);
        }
        else if (updateType == AsyncUpdateType::NodeRemoved) {
            nodeManager.remove(nodeId);

            int rootNodeId = asyncUpdate.rootNodeId;
            if (rootNodeId != nodeId) {

                applicationContext.rtGraphBuilder->makeRTGraph(applicationContext.graphState->getNode(rootNodeId));
            } else {

                applicationContext.rtGraphBuilder->discardGraph(rootNodeId);
            }
        }
        else if (updateType == AsyncUpdateType::NodeMoved) {
            nodeManager.setPosition(nodeId);
            rememberOnce(pitchSyncNodeIds, nodeId);
            rememberOnce(durationRefreshNodeIds, nodeId);
            rememberOnce(movedNodeIds, nodeId);
        }
        else if (updateType == AsyncUpdateType::ValueChanged) {
            if (Node* const changedNode = nodeManager.find(nodeId)) {
                arrowManager.refreshFor(changedNode);
            }
        }
        else if (updateType == AsyncUpdateType::DurationOnly) {
            rememberOnce(durationRefreshNodeIds, nodeId);
        }
        else if (updateType == AsyncUpdateType::DanglingArrowsChanged) {
            arrowManager.rebuildDanglingForNode(nodeId);
            rememberOnce(pitchSyncNodeIds, nodeId);
            rememberOnce(durationRefreshNodeIds, nodeId);
        }
        else if (updateType == AsyncUpdateType::ArrowAdded) {
            arrowManager.handleArrowAdded(nodeId, asyncUpdate.rootNodeId);
        }
        else if (updateType == AsyncUpdateType::ArrowRemoved) {
            arrowManager.handleArrowRemoved(nodeId, asyncUpdate.rootNodeId);
        }
        else if (updateType == AsyncUpdateType::ArrowInfoChanged) {
            arrowManager.handleArrowInfoChanged(nodeId, asyncUpdate.rootNodeId);
        }
        else if (updateType == AsyncUpdateType::ArrowDurationChanged) {
            if (Node* const owningNode = nodeManager.find(nodeId)) {
                arrowManager.refreshFor(owningNode);
            }

            rememberOnce(durationRefreshNodeIds, nodeId);
        }
    }

    GraphState& state = *applicationContext.graphState;

    std::vector<int> repitchedRootIds;

    for (int nodeId : pitchSyncNodeIds) {
        for (int repitchedNodeId : state.arrows.syncPitchBindings(nodeId, applicationContext.undoManager)) {
            rememberOnce(repitchedRootIds,
                         (int) state.getNode(repitchedNodeId).getProperty(ValueTreeIdentifiers::RootNodeId));
        }
    }

    for (int rootNodeId : repitchedRootIds) {
        applicationContext.rtGraphBuilder->makeRTGraph(state.getNode(rootNodeId));
    }

    for (int nodeId : movedNodeIds) {
        state.arrows.clearArrowDurations(nodeId, applicationContext.undoManager);
    }

    applicationContext.rtGraphBuilder->updateDurationMaps(durationRefreshNodeIds);

    const bool fieldNeedsRefresh = ! pendingUpdates.empty();

    if (paintMode && fieldNeedsRefresh) {
        valueField.refresh();
    }
}

void NodeCanvas::setProcessorPlayblack(bool isPlaying)
{
    start = isPlaying;
    applicationContext.processor->isPlaying.store(start);

    if (isPlaying) {
        nodeManager.equipRootTraversals();
    }

    applicationContext.rtGraphBuilder->rebuildAllGraphs();

    if (! isPlaying) {
        arrowManager.resetAllProgress();
    }
}

void NodeCanvas::clearCanvas()
{
    arrowManager.clear();
    nodeManager.clear();

    gridOriginSet = false;
    gridVisible = false;
}

void NodeCanvas::rebuildFromNodeMap(const juce::ValueTree& stateTree)
{
    asyncUpdates.clear();
    cancelPendingUpdate();

    clearCanvas();

    std::unordered_map<int,juce::ValueTree> rootNodeMap;

    struct NodePair {
        int parentNodeId;
        int childNodeId;
    };

    std::vector<NodePair> nodePairs;

    if (stateTree.getNumChildren() == 0) { DBG("stateTree is empty"); }

    for (int i = 0 ; i < stateTree.getNumChildren(); i++) {

        juce::ValueTree nodeValueTree = stateTree.getChild(i);
        int nodeId = nodeValueTree.getProperty(ValueTreeIdentifiers::Id);

        if (nodeValueTree.getType() == ValueTreeIdentifiers::RootNodeData) {
            rootNodeMap[nodeId] = nodeValueTree;
        }

        juce::ValueTree nodeValueTreeChildren = nodeValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);
        for (int j = 0; j < nodeValueTreeChildren.getNumChildren(); j++) {
            int childId = nodeValueTreeChildren.getChild(j).getProperty(ValueTreeIdentifiers::Id);
            nodePairs.push_back({ nodeId, childId });
        }

        nodeManager.instantiateFromTree(nodeValueTree);
    }

    for (auto [parentNodeId,childNodeId] : nodePairs) {

        Node* parentNode = nodeManager.find(parentNodeId);
        Node* childNode  = nodeManager.find(childNodeId);

        if (parentNode == nullptr || childNode == nullptr) {
            continue;
        }

        Node* startNode = parentNode;
        Node* endNode   = childNode;

        if (childNode->nodeValueTree.getType() == ValueTreeIdentifiers::AlternativeNodeData
            || childNode->nodeValueTree.getType() == ValueTreeIdentifiers::AlternativeModulatorData) {
            startNode = childNode;
            endNode   = parentNode;
        }

        if (startNode->nodeArrows.count(endNode->getComponentID().getIntValue()) > 0) {
            continue;
        }

        endNode->nodeColour = startNode->nodeColour;
        arrowManager.connect(startNode, endNode);
        arrowManager.refreshFor(endNode);
    }

    for (auto& [nodeId, node] : nodeManager.all()) {
        arrowManager.rebuildDanglingForNode(nodeId);
    }

    encapsulationView.collapseAll();

    if (!gridOriginSet && !rootNodeMap.empty()) {
        auto it = rootNodeMap.begin();
        int firstRootId = it->first;
        NodePosition pos = applicationContext.graphState->getNodePosition(firstRootId);
        gridOrigin    = { (float)pos.xPosition,
                          (float)pos.yPosition };
        gridSpacing   = ArrowInfo::pixelsPerGridSpace;
        gridOriginSet = true;
    }

    for (auto& [id, rootNodeValueTree] : rootNodeMap) {
        if (rootNodeValueTree.isValid()) {
            applicationContext.rtGraphBuilder->makeRTGraph(rootNodeValueTree);
        }
    }

}

void NodeCanvas::setPaintMode(bool enabled)
{
    paintMode = enabled;

    nodeManager.setInterceptsClicks(!enabled, !enabled && !spanMode);

    if (enabled) {
        valueField.updateCursor();
        valueField.refresh();
    }
    else {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void NodeCanvas::setSpanMode(bool enabled)
{
    spanMode         = enabled;
    spanAnchorNodeId = -1;

    nodeManager.setInterceptsClicks(!paintMode, !paintMode && !enabled);
    nodeManager.clearOutlines();

    if (! enabled) {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    juce::Image cursorImage(juce::Image::ARGB, spanCursorSize, spanCursorSize, true);
    juce::Graphics cursorGraphics(cursorImage);

    cursorGraphics.setColour(juce::Colours::black);
    cursorGraphics.drawRect(cursorImage.getBounds().reduced(1), 1);

    setMouseCursor(juce::MouseCursor(cursorImage, spanCursorSize / 2, spanCursorSize / 2));
}

void NodeCanvas::showGrid()
{
    if (gridOriginSet) {
        gridVisible = true;
        repaint();
    }
}

void NodeCanvas::hideGrid()
{
    if (gridVisible) {
        gridVisible = false;
        repaint();
    }
}

juce::Point<int> NodeCanvas::snapPointToGrid(juce::Point<int> point) const
{
    if (!gridOriginSet) {
        return point;
    }

    const float originX = gridOrigin.x;
    const float originY = gridOrigin.y;
    const float snapThreshold = 12.0f;

    const float snappedX = originX + std::round((float(point.x) - originX) / gridSpacing) * gridSpacing;
    const float snappedY = originY + std::round((float(point.y) - originY) / gridSpacing) * gridSpacing;

    juce::Point<int> result = point;

    if (std::abs(float(point.x) - snappedX) < snapThreshold) {
        result.x = int(snappedX);
    }
    if (std::abs(float(point.y) - snappedY) < snapThreshold) {
        result.y = int(snappedY);
    }

    return result;
}
