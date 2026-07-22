#include <juce_gui_basics/juce_gui_basics.h>

#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/ValueTreeState.h"
#include "../../Plugin/PluginProcessor.h"
#include "../Node/NodeTextEditor.h"
#include "../Node/Arrow.h"
#include "../../Graph/ValueTreeIdentifiers.h"

#include "../Node/Node.h"
#include "../Node/RootNode.h"
#include "NodeCanvas.h"
#include "../../Audio/EventManager.h"
#include "../../Graph/RTGraphBuilder.h"

#include "../Node/Modulator.h"
#include "../Node/TraversalFlagNode.h"


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
}

void NodeCanvas::enqueueAsyncUpdate(const AsyncUpdate& update)
{
    asyncUpdates.push_back(update);
    triggerAsyncUpdate();
}

void NodeCanvas::handleAsyncUpdate() {
    drainer.drainAll();

    for (auto& asyncUpdate  : asyncUpdates) {
        int nodeId = asyncUpdate.nodeId;

        AsyncUpdateType updateType = asyncUpdate.type;

        if (updateType == AsyncUpdateType::NodeAdded) {
            nodeManager.add(nodeId);
        }
        else if (updateType == AsyncUpdateType::NodeRemoved) {
            nodeManager.remove(nodeId);

            int rootNodeId = asyncUpdate.rootNodeId;
            if (rootNodeId != nodeId) {

                juce::ValueTree rootTree = applicationContext.valueTreeState->getNode(rootNodeId);
                if (rootTree.isValid()) {
                    applicationContext.rtGraphBuilder->makeRTGraph(rootTree);
                }
            } else {

                auto emptyGraph = std::make_shared<RTGraph>();
                emptyGraph->graphID = rootNodeId;
                applicationContext.processor->setNewGraph(emptyGraph);
            }
        }
        else if (updateType == AsyncUpdateType::NodeMoved) {
            nodeManager.setPosition(nodeId);
            applicationContext.rtGraphBuilder->updateDurationMap(nodeId);
        }
        else if (updateType == AsyncUpdateType::DurationOnly) {
            applicationContext.rtGraphBuilder->updateDurationMap(nodeId);
        }
        else if (updateType == AsyncUpdateType::DanglingArrowsChanged) {
            danglingArrowLayer.rebuildForNode(nodeId);
            applicationContext.rtGraphBuilder->updateDurationMap(nodeId);
        }
        else if (updateType == AsyncUpdateType::ArrowAdded) {
            arrowManager.handleArrowAdded(nodeId, asyncUpdate.rootNodeId);
        }
        else if (updateType == AsyncUpdateType::ArrowRemoved) {
            arrowManager.handleArrowRemoved(nodeId, asyncUpdate.rootNodeId);
        }
    }

    const bool fieldNeedsRefresh = ! asyncUpdates.empty();
    asyncUpdates.clear();

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
        graph.get()->traversalRequested = start;
        applicationContext.processor->setNewGraph(graph);
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
