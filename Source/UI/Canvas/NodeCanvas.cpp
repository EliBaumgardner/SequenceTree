#include <juce_gui_basics/juce_gui_basics.h>

#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/ValueTreeState.h"
#include "../../Plugin/PluginProcessor.h"
#include "../Node/NodeTextEditor.h"
#include "../Node/NodeArrow.h"
#include "../../Graph/ValueTreeIdentifiers.h"

#include "../Node/Node.h"
#include "../Node/RootNode.h"
#include "../Node/Connector.h"
#include "NodeCanvas.h"
#include "../../Audio/EventManager.h"
#include "../../Graph/RTGraphBuilder.h"

#include "../Node/Modulator.h"


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
}

void NodeCanvas::enqueueAsyncUpdate(const AsyncUpdate& update)
{
    asyncUpdates.push_back(update);
    triggerAsyncUpdate();
}

void NodeCanvas::handleAsyncUpdate() {
    drainHighlightFifo();
    drainArrowResetFifo();
    drainProgressFifo();
    drainCountFifo();

    for (auto& asyncUpdate  : asyncUpdates) {
        int nodeId = asyncUpdate.nodeId;

        AsyncUpdateType updateType = asyncUpdate.type;

        if (updateType == AsyncUpdateType::NodeAdded) {
            addNodeToCanvas(nodeId);
        }
        else if (updateType == AsyncUpdateType::NodeRemoved) {
            removeNodeFromCanvas(nodeId);

            int rootNodeId = asyncUpdate.rootNodeId;
            if (rootNodeId != nodeId) {

                juce::ValueTree rootTree = ValueTreeState::getNode(rootNodeId);
                if (rootTree.isValid())
                    applicationContext.rtGraphBuilder->makeRTGraph(rootTree);
            } else {

                auto emptyGraph = std::make_shared<RTGraph>();
                emptyGraph->graphID = rootNodeId;
                applicationContext.processor->setNewGraph(emptyGraph);
            }
        }
        else if (updateType == AsyncUpdateType::NodeMoved) {
            setNodePosition(nodeId);
            applicationContext.rtGraphBuilder->updateDurationMap(nodeId);
        }
        else if (updateType == AsyncUpdateType::DurationOnly) {
            applicationContext.rtGraphBuilder->updateDurationMap(nodeId);
        }
    }
    asyncUpdates.clear();
}

void NodeCanvas::drainProgressFifo()
{
    auto& bridge = applicationContext.processor->eventManager.bridge;
    const auto scope = bridge.progressFifo.read(bridge.progressFifo.getNumReady());

    auto apply = [this](const AudioUIBridge::ProgressCommand& cmd)
    {
        auto parentIt = nodeMap.find(cmd.parentNodeId);
        if (parentIt == nodeMap.end()) {
            return;
        }

        auto arrowIt = parentIt->second->nodeArrows.find(cmd.childNodeId);
        if (arrowIt == parentIt->second->nodeArrows.end() || arrowIt->second == nullptr) {
            return;
        }

        arrowIt->second->startProgress(cmd.durationMs);
    };

    for (int i = 0; i < scope.blockSize1; ++i)
        apply(bridge.progressBuffer[static_cast<size_t>(scope.startIndex1 + i)]);
    for (int i = 0; i < scope.blockSize2; ++i)
        apply(bridge.progressBuffer[static_cast<size_t>(scope.startIndex2 + i)]);
}

void NodeCanvas::drainArrowResetFifo() {
    auto& bridge = applicationContext.processor->eventManager.bridge;
    const auto scope = bridge.arrowResetFifo.read(bridge.arrowResetFifo.getNumReady());

    auto apply = [this](const AudioUIBridge::ResetCommand cmd) {
            resetGraphArrowProgress(cmd.rootId);
    };

    for (int i = 0; i < scope.blockSize1; ++i)
        apply(bridge.arrowResetBuffer[static_cast<size_t>(scope.startIndex1 + i)]);
    for (int i = 0; i < scope.blockSize2; ++i)
        apply(bridge.arrowResetBuffer[static_cast<size_t>(scope.startIndex2 + i)]);
}

void NodeCanvas::resetAllArrowProgress()
{
    for (NodeArrow* arrow : nodeArrows)
        if (arrow != nullptr)
            arrow->resetProgress();
}

void NodeCanvas::resetGraphArrowProgress(int graphId)
{
    for (NodeArrow* arrow : nodeArrows)
    {
        if (arrow == nullptr || arrow->parentNode == nullptr) continue;

        const int parentId = arrow->parentNode->getComponentID().getIntValue();
        const juce::ValueTree arrowRoot = ValueTreeState::getRootNode(parentId);
        if (! arrowRoot.isValid()) continue;

        if (static_cast<int>(arrowRoot.getProperty(ValueTreeIdentifiers::Id)) == graphId)
            arrow->resetProgress();
    }
}

void NodeCanvas::drainCountFifo()
{
    auto& bridge = applicationContext.processor->eventManager.bridge;
    const auto scope = bridge.countFifo.read(bridge.countFifo.getNumReady());

    auto apply = [this](const AudioUIBridge::CountCommand& cmd)
    {
        auto it = nodeMap.find(cmd.nodeId);
        if (it == nodeMap.end()) return;

        Node* node = it->second;
        node->displayCurrentCount = cmd.currentCount;
        node->displayCountLimit   = juce::jmax(1, cmd.countLimit);
        node->repaint();
    };

    for (int i = 0; i < scope.blockSize1; ++i)
        apply(bridge.countBuffer[static_cast<size_t>(scope.startIndex1 + i)]);
    for (int i = 0; i < scope.blockSize2; ++i)
        apply(bridge.countBuffer[static_cast<size_t>(scope.startIndex2 + i)]);
}

void NodeCanvas::drainHighlightFifo()
{
    auto& bridge = applicationContext.processor->eventManager.bridge;
    const auto scope = bridge.highlightFifo.read(bridge.highlightFifo.getNumReady());

    for (int i = 0; i < scope.blockSize1; ++i)
    {
        auto& cmd = bridge.highlightBuffer[static_cast<size_t>(scope.startIndex1 + i)];
        auto it = nodeMap.find(cmd.nodeId);
        if (it != nodeMap.end())
            it->second->setHighlightVisual(cmd.shouldHighlight);
    }
    for (int i = 0; i < scope.blockSize2; ++i)
    {
        auto& cmd = bridge.highlightBuffer[static_cast<size_t>(scope.startIndex2 + i)];
        auto it = nodeMap.find(cmd.nodeId);
        if (it != nodeMap.end())
            it->second->setHighlightVisual(cmd.shouldHighlight);
    }
}

Node* NodeCanvas::instantiateNodeFromTree(const juce::ValueTree& nodeValueTree)
{
    jassert(nodeValueTree.isValid());

    const juce::Identifier treeType = nodeValueTree.getType();
    const int nodeId = nodeValueTree.getProperty(ValueTreeIdentifiers::Id);

    std::unique_ptr<Node> node;
    if (treeType == ValueTreeIdentifiers::RootNodeData) {
        node = std::make_unique<RootNode>(applicationContext);
    }
    else if (treeType == ValueTreeIdentifiers::NodeData) {
        node = std::make_unique<Node>(applicationContext);
    }
    else if (treeType == ValueTreeIdentifiers::ConnectorData) {
        node = std::make_unique<Connector>(applicationContext);
    }
    else if (treeType == ValueTreeIdentifiers::ModulatorData
          || treeType == ValueTreeIdentifiers::ModulatorRootData) {
        node = std::make_unique<Modulator>(applicationContext);
    }

    jassert(node);

    juce::ValueTree midiNotes = nodeValueTree.getChildWithName(ValueTreeIdentifiers::MidiNotesData);

    node->setComponentID(std::to_string(nodeId));
    node->nodeValueTree = nodeValueTree;
    node->midiNoteData  = midiNotes.getChildWithName(ValueTreeIdentifiers::MidiNoteData);
    node->setDisplayMode(NodeDisplayMode::Pitch);

    node->onSelected = [this](Node* n, bool sel) {
        if (applicationContext.onNodeSelected)
            applicationContext.onNodeSelected(n, sel);
    };

    addAndMakeVisible(node.get());
    Node* raw = node.release();
    nodeMap[nodeId] = raw;

    setNodePosition(nodeId);

    return raw;
}

void NodeCanvas::addNodeToCanvas(int nodeId)
{
    juce::ValueTree nodeValueTree = ValueTreeState::getNode(nodeId);
    jassert(nodeValueTree.isValid());

    Node* childNode = instantiateNodeFromTree(nodeValueTree);

    juce::ValueTree nodeParent = ValueTreeState::getNodeParent(nodeId);

    if (nodeParent.isValid()) {
        int parentNodeId = nodeParent.getProperty(ValueTreeIdentifiers::Id);
        auto parentNodePair = nodeMap.find(parentNodeId);
        if (parentNodePair != nodeMap.end()) {
            childNode->nodeColour = parentNodePair->second->nodeColour;
            addLinePoints(parentNodePair->second, childNode);
            updateLinePoints(childNode);
        }
    }

    if (!gridOriginSet && nodeValueTree.getType() == ValueTreeIdentifiers::RootNodeData) {
        NodePosition pos = ValueTreeState::getNodePosition(nodeId);
        gridOrigin    = { (float)pos.xPosition, (float)pos.yPosition };
        gridSpacing   = 50.0f;
        gridOriginSet = true;
    }

    applicationContext.rtGraphBuilder->makeRTGraph(nodeValueTree);

    int rootNodeId = nodeValueTree.getProperty(ValueTreeIdentifiers::RootNodeId);
    juce::ValueTree connectorParent = ValueTreeState::getNodeParent(rootNodeId);

    if (connectorParent.isValid()) {
        int connectorRootId = connectorParent.getProperty(ValueTreeIdentifiers::RootNodeId);
        if (connectorRootId != 0) {
            juce::ValueTree connectorRootTree = ValueTreeState::getNode(connectorRootId);
            if (connectorRootTree.isValid())
                applicationContext.rtGraphBuilder->makeRTGraph(connectorRootTree);
        }
    }
}

void NodeCanvas::removeNodeFromCanvas(int nodeId)
{
    auto nodePair = nodeMap.find(nodeId);
    if (nodePair == nodeMap.end()) return;

    Node* node = nodePair->second;
    removeLinePoints(node);
    removeChildComponent(node);
    delete node;
    nodeMap.erase(nodeId);
}

void NodeCanvas::setNodePosition(int nodeId)
{
    auto nodePair = nodeMap.find(nodeId);
    if (nodePair == nodeMap.end()) return;

    juce::ValueTree nodeValueTree = ValueTreeState::getNode(nodeId);
    if (!nodeValueTree.isValid()) return;

    NodePosition nodePosition = ValueTreeState::getNodePosition(nodeId);

    int xPosition = nodePosition.xPosition;
    int yPosition = nodePosition.yPosition;
    int radius    = nodePosition.radius;

    auto node = nodePair->second;

    if (node->nodeType == NodeType::Root)
    {
        const int rw = RootNode::loopLimitRectangleWidth;
        node->setSize(radius * 2 + rw, radius * 2);
        node->setTopLeftPosition(xPosition - radius - rw, yPosition - radius);
    }
    else
    {
        node->setSize(radius * 2, radius * 2);
        node->setCentrePosition(xPosition, yPosition);
    }

    updateLinePoints(node);

}

void NodeCanvas::moveDescendants(juce::ValueTree nodeValueTree, int deltaX, int deltaY)
{
    juce::ValueTree nodeValueTreeChildren = nodeValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    for (int i = 0; i < nodeValueTreeChildren.getNumChildren(); i++) {
        juce::ValueTree childIdTree = nodeValueTreeChildren.getChild(i);
        int childId = childIdTree.getProperty(ValueTreeIdentifiers::Id);
        juce::ValueTree childNodeTree = ValueTreeState::getNode(childId);

        NodePosition childPosition = ValueTreeState::getNodePosition(childId);
        childPosition.xPosition += deltaX;
        childPosition.yPosition += deltaY;

        ValueTreeState::setNodePosition(childNodeTree, childPosition, applicationContext.undoManager);
        moveDescendants(childNodeTree, deltaX, deltaY);
    }
}

void NodeCanvas::addLinePoints(Node* parentNode, Node* childNode)
{
    DBG("creating line point");

    int parentNodeId = parentNode->getComponentID().getIntValue();
    int childNodeId  = childNode->getComponentID().getIntValue();

    juce::ValueTree parentMidiNotesData = ValueTreeState::getMidiNotes(parentNodeId);
    juce::ValueTree parentMidiNoteData  = parentMidiNotesData.getChildWithName(ValueTreeIdentifiers::MidiNoteData);

    auto arrow = std::make_unique<NodeArrow>(parentNode, childNode, applicationContext);

    parentNode->nodeArrows[childNodeId] = arrow.get();
    addAndMakeVisible(arrow.get());

    arrow->toBack();
    arrow->setInterceptsMouseClicks(false,false);

    if (parentMidiNoteData.isValid() && childNode->nodeType != NodeType::Connector && childNode->nodeType != NodeType::Root) {
        arrow->bindToProperty(parentMidiNoteData, ValueTreeIdentifiers::MidiDuration);
    }

    nodeArrows.add(arrow.release());
}

void NodeCanvas::removeLinePoints(Node* node)
{
    for (int i = nodeArrows.size() - 1; i >= 0; i--)
    {
        NodeArrow* nodeArrow = nodeArrows[i];
        if (nodeArrow->parentNode != node && nodeArrow->childNode != node) continue;

        const int childNodeId = nodeArrow->childNode->getComponentID().getIntValue();
        nodeArrow->parentNode->nodeArrows.erase(childNodeId);

        nodeArrows.remove(i);
    }
}

void NodeCanvas::updateLinePoints(Node* movedNode)
{
    for (NodeArrow* arrow : nodeArrows)
    {
        Node* parentNode = arrow->parentNode;
        Node* childNode  = arrow->childNode;
        NodeDisplayMode mode = movedNode->mode;

        if (parentNode != movedNode && childNode != movedNode) {
            continue;
        }

        parentNode->nodeTextEditor->formatDisplay(mode);
        childNode->nodeTextEditor->formatDisplay(mode);
        arrow->setArrowBounds(movedNode);
    }
}

// processor-related Functions //

void NodeCanvas::setProcessorPlayblack(bool isPlaying)
{
    start = isPlaying;
    applicationContext.processor->isPlaying.store(start);

    for(auto& [graphID,graph] : applicationContext.rtGraphBuilder->rtGraphs) {
        graph.get()->traversalRequested = start;
        applicationContext.processor->setNewGraph(graph);
    }

    if (! isPlaying)
        resetAllArrowProgress();
}

void NodeCanvas::setSelectionMode(NodeDisplayMode mode) const {

    for (auto& [nodeId, node] : nodeMap)
    {
        node->setDisplayMode(mode);
    }
}

void NodeCanvas::triggerArrowSnapForNode(int nodeId)
{
    auto nodePair = nodeMap.find(nodeId);
    if (nodePair == nodeMap.end()) return;
    Node* node = nodePair->second;

    for (NodeArrow* arrow : nodeArrows)
    {
        if (arrow->childNode == node)
        {
            arrow->triggerSnapAnimation();
            return;
        }
    }
}

void NodeCanvas::showSnapGhostArrow(Node* from, Node* to)
{
    if (snapGhostArrow != nullptr)
    {
        if (snapGhostArrow->parentNode == from && snapGhostArrow->childNode == to)
            return;
        removeChildComponent(snapGhostArrow);
        delete snapGhostArrow;
        snapGhostArrow = nullptr;
    }

    snapGhostArrow = new NodeArrow(from, to, applicationContext);
    snapGhostArrow->isGhost = true;
    snapGhostArrow->setInterceptsMouseClicks(false, false);
    addAndMakeVisible(snapGhostArrow);
    snapGhostArrow->toBack();
    snapGhostArrow->setArrowBounds(from);
    snapGhostArrow->triggerSnapAnimation();
}

void NodeCanvas::hideSnapGhostArrow()
{
    if (snapGhostArrow != nullptr)
    {
        removeChildComponent(snapGhostArrow);
        delete snapGhostArrow;
        snapGhostArrow = nullptr;
    }
}

void NodeCanvas::clearCanvas()
{
    hideSnapGhostArrow();
    nodeArrows.clear();

    for (auto& [nodeId, node] : nodeMap) {
        removeChildComponent(node);
        delete node;
    }
    nodeMap.clear();
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

        if (nodeValueTree.getType() == ValueTreeIdentifiers::RootNodeData)
            rootNodeMap[nodeId] = nodeValueTree;

        juce::ValueTree nodeValueTreeChildren = nodeValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);
        for (int j = 0; j < nodeValueTreeChildren.getNumChildren(); j++) {
            int childId = nodeValueTreeChildren.getChild(j).getProperty(ValueTreeIdentifiers::Id);
            nodePairs.push_back({ nodeId, childId });
        }

        instantiateNodeFromTree(nodeValueTree);
    }

    for (auto [parentNodeId,childNodeId] : nodePairs) {

        auto parentNodePair = nodeMap.find(parentNodeId);
        auto childNodePair = nodeMap.find(childNodeId);

        jassert(parentNodePair != nodeMap.end());
        jassert(childNodePair != nodeMap.end());

        Node* parentNode = parentNodePair->second;
        Node* childNode = childNodePair->second;

        addLinePoints(parentNode,childNode);
        updateLinePoints(parentNode);
    }

    if (!gridOriginSet && !rootNodeMap.empty()) {
        auto it = rootNodeMap.begin();
        int firstRootId = it->first;
        NodePosition pos = ValueTreeState::getNodePosition(firstRootId);
        gridOrigin    = { (float)pos.xPosition, (float)pos.yPosition };
        gridSpacing   = 50.0f;
        gridOriginSet = true;
    }

    for (auto& [id, rootNodeValueTree] : rootNodeMap) {
        if (rootNodeValueTree.isValid())
            applicationContext.rtGraphBuilder->makeRTGraph(rootNodeValueTree);
    }

}