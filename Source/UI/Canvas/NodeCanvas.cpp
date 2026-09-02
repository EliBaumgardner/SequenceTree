#include <juce_gui_basics/juce_gui_basics.h>

#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/ValueTreeState.h"
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

namespace {

void rememberOnce(std::vector<int>& ids, int id)
{
    if (std::find(ids.begin(), ids.end(), id) == ids.end()) {
        ids.push_back(id);
    }
}

}

// Canvas Related Functions //
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

void NodeCanvas::handleAsyncUpdate() {
    drainer.drainAll();

    std::vector<AsyncUpdate> pendingUpdates;
    pendingUpdates.swap(asyncUpdates);

    std::vector<int> pitchSyncNodeIds;
    std::vector<int> durationRefreshNodeIds;

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

                applicationContext.rtGraphBuilder->makeRTGraph(applicationContext.valueTreeState->getNode(rootNodeId));
            } else {

                auto emptyGraph = std::make_shared<RTGraph>();
                emptyGraph->graphID = rootNodeId;
                applicationContext.processor->snapshots.publishGraph(emptyGraph);
            }
        }
        else if (updateType == AsyncUpdateType::NodeMoved) {
            nodeManager.setPosition(nodeId);
            rememberOnce(pitchSyncNodeIds, nodeId);
            rememberOnce(durationRefreshNodeIds, nodeId);
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
            danglingArrowLayer.rebuildForNode(nodeId);
            rememberOnce(pitchSyncNodeIds, nodeId);
            rememberOnce(durationRefreshNodeIds, nodeId);
        }
        else if (updateType == AsyncUpdateType::ArrowAdded) {
            arrowManager.handleArrowAdded(nodeId, asyncUpdate.rootNodeId);
        }
        else if (updateType == AsyncUpdateType::ArrowRemoved) {
            arrowManager.handleArrowRemoved(nodeId, asyncUpdate.rootNodeId);
        }
        else if (updateType == AsyncUpdateType::ArrowTypeChanged) {
            arrowManager.handleArrowTypeChanged(nodeId, asyncUpdate.rootNodeId);
        }
    }

    ValueTreeState& state = *applicationContext.valueTreeState;

    std::vector<int> repitchedRootIds;

    for (int nodeId : pitchSyncNodeIds) {
        for (int repitchedNodeId : state.syncPitchBindings(nodeId, applicationContext.undoManager)) {
            rememberOnce(repitchedRootIds,
                         (int) state.getNode(repitchedNodeId).getProperty(ValueTreeIdentifiers::RootNodeId));
        }
    }

    for (int rootNodeId : repitchedRootIds) {
        applicationContext.rtGraphBuilder->makeRTGraph(state.getNode(rootNodeId));
    }

    applicationContext.rtGraphBuilder->updateDurationMaps(durationRefreshNodeIds);

    const bool fieldNeedsRefresh = ! pendingUpdates.empty();

    if (paintMode && fieldNeedsRefresh) {
        valueField.refresh();
    }
}

// processor-related Functions //

void NodeCanvas::setProcessorPlayblack(bool isPlaying)
{
    start = isPlaying;
    applicationContext.processor->isPlaying.store(start);

    if (isPlaying) {
        nodeManager.equipRootTraversals();
    }

    for(auto& [graphID,graph] : applicationContext.rtGraphBuilder->rtGraphs) {
        applicationContext.processor->snapshots.publishGraph(graph);
    }

    if (! isPlaying) {
        arrowManager.resetAllProgress();
    }
}

void NodeCanvas::clearCanvas()
{
    arrowManager.clear();
    danglingArrowLayer.clear();
    nodeManager.clear();

    gridOriginSet = false;
    showGrid = false;
}

void NodeCanvas::setValueTreeState(const juce::ValueTree& stateTree)
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

        if (childNode->nodeValueTree.getType() == ValueTreeIdentifiers::AlternativeNodeData) {
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
        danglingArrowLayer.rebuildForNode(nodeId);
    }

    if (!gridOriginSet && !rootNodeMap.empty()) {
        auto it = rootNodeMap.begin();
        int firstRootId = it->first;
        NodePosition pos = applicationContext.valueTreeState->getNodePosition(firstRootId);
        gridOrigin    = { (float)pos.xPosition,
                          (float)pos.yPosition };
        gridSpacing   = 50.0f;
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

    nodeManager.setInterceptsClicks(!enabled);

    if (enabled) {
        valueField.updateCursor();
        valueField.refresh();
    }
    else {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
    }
}
