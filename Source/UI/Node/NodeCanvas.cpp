#include <juce_gui_basics/juce_gui_basics.h>

#include "CustomLookAndFeel.h"
#include "../Util/ValueTreeState.h"
#include "../Core/PluginProcessor.h"
#include "NodeTextEditor.h"
#include "NodeArrow.h"
#include "../Util/ValueTreeIdentifiers.h"

#include "Node.h"
#include "RootNode.h"
#include "Connector.h"
#include "NodeCanvas.h"
#include "../Core/EventManager.h"

#include "Modulator.h"


// Canvas Related Functions //
NodeCanvas::NodeCanvas(ApplicationContext& context) : applicationContext(context)
{
    setPaintingIsUnclipped(true);
    setLookAndFeel(applicationContext.lookAndFeel);
}

NodeCanvas::~NodeCanvas() {
    clearCanvas();
}


void NodeCanvas::paint(juce::Graphics& g)
{
    CustomLookAndFeel::get(*this).drawCanvas(g, *this);
}

void NodeCanvas::handleAsyncUpdate() {
    drainHighlightFifo();
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
                    makeRTGraph(rootTree);
            } else {

                auto emptyGraph = std::make_shared<RTGraph>();
                emptyGraph->graphID = rootNodeId;
                applicationContext.processor->setNewGraph(emptyGraph);
            }
        }
        else if (updateType == AsyncUpdateType::NodeMoved) {
            setNodePosition(nodeId);
            updateDurationMap(nodeId);
        }
        else if (updateType == AsyncUpdateType::DurationOnly) {
            updateDurationMap(nodeId);
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
        if (parentIt == nodeMap.end()) return;

        // A new cycle through this graph — wipe the filled path from the previous cycle
        // so only the current traversal's path stays visible.
        if (cmd.parentNodeId == cmd.graphId)
            resetGraphArrowProgress(cmd.graphId);

        auto arrowIt = parentIt->second->nodeArrows.find(cmd.childNodeId);
        if (arrowIt == parentIt->second->nodeArrows.end() || arrowIt->second == nullptr)
            return;

        arrowIt->second->startProgress(cmd.durationMs);
    };

    for (int i = 0; i < scope.blockSize1; ++i)
        apply(bridge.progressBuffer[static_cast<size_t>(scope.startIndex1 + i)]);
    for (int i = 0; i < scope.blockSize2; ++i)
        apply(bridge.progressBuffer[static_cast<size_t>(scope.startIndex2 + i)]);
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

void NodeCanvas::addNodeToCanvas(int nodeId)
{
    juce::ValueTree nodeValueTree = ValueTreeState::getNode(nodeId);

    jassert(nodeValueTree.isValid());

    juce::ValueTree nodeParent = ValueTreeState::getNodeParent(nodeId);
    juce::ValueTree midiNotes  = ValueTreeState::getMidiNotes(nodeId);
    juce::ValueTree midiNote = midiNotes.getChildWithName(ValueTreeIdentifiers::MidiNoteData);

    std::unique_ptr<Node> childNode = nullptr;

    if (nodeValueTree.getType() == ValueTreeIdentifiers::RootNodeData) {
        childNode = std::make_unique<RootNode>(applicationContext);
    }
    else if (nodeValueTree.getType() == ValueTreeIdentifiers::NodeData) {
        childNode = std::make_unique<Node>(applicationContext);
    }
    else if (nodeValueTree.getType() == ValueTreeIdentifiers::ConnectorData) {
        childNode = std::make_unique<Connector>(applicationContext);
    }
    else if (nodeValueTree.getType() == ValueTreeIdentifiers::ModulatorData) {
        childNode = std::make_unique<Modulator>(applicationContext);
    }
    else if (nodeValueTree.getType() == ValueTreeIdentifiers::ModulatorRootData) {
        childNode = std::make_unique<Modulator>(applicationContext);
    }

    jassert(childNode);

    childNode->setComponentID(std::to_string(nodeId));
    childNode->nodeValueTree = nodeValueTree;
    childNode->midiNoteData = midiNote;
    childNode->setDisplayMode(NodeDisplayMode::Pitch);

    if (nodeParent.isValid()) {
        int parentNodeId = nodeParent.getProperty(ValueTreeIdentifiers::Id);
        auto parentNodePair = nodeMap.find(parentNodeId);
        if (parentNodePair != nodeMap.end()) {
            Node* parentNode = parentNodePair->second;
            addLinePoints(parentNode, childNode.get());
        }
    }


    addAndMakeVisible(childNode.get());
    nodeMap[nodeId] = childNode.release();

    setNodePosition(nodeId);

    if (!gridOriginSet && nodeValueTree.getType() == ValueTreeIdentifiers::RootNodeData) {
        NodePosition pos = ValueTreeState::getNodePosition(nodeId);
        gridOrigin    = { (float)pos.xPosition, (float)pos.yPosition };
        gridSpacing   = 50.0f;
        gridOriginSet = true;
    }

    makeRTGraph(nodeValueTree);

    int rootNodeId = nodeValueTree.getProperty(ValueTreeIdentifiers::RootNodeId);
    juce::ValueTree connectorParent = ValueTreeState::getNodeParent(rootNodeId);

    if (connectorParent.isValid()) {

        int connectorRootId = connectorParent.getProperty(ValueTreeIdentifiers::RootNodeId);

        if (connectorRootId != 0) {
            juce::ValueTree connectorRootTree = ValueTreeState::getNode(connectorRootId);
            if (connectorRootTree.isValid()) {
                makeRTGraph(connectorRootTree);
            }
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



void NodeCanvas::addLinePoints(Node* parentNode, Node* childNode)
{
    int parentNodeId = parentNode->getComponentID().getIntValue();
    int childNodeId  = childNode->getComponentID().getIntValue();

    juce::ValueTree parentMidiNotesData = ValueTreeState::getMidiNotes(parentNodeId);
    juce::ValueTree parentMidiNoteData  = parentMidiNotesData.getChildWithName(ValueTreeIdentifiers::MidiNoteData);

    auto arrow = std::make_unique<NodeArrow>(parentNode, childNode, applicationContext);

    parentNode->nodeArrows[childNodeId] = arrow.get();
    addAndMakeVisible(arrow.get());

    arrow->toBack();
    arrow->setInterceptsMouseClicks(false,false);

    if (parentMidiNoteData.isValid() && childNode->nodeType != NodeType::Connector && childNode->nodeType != NodeType::Root)
        arrow->bindToProperty(parentMidiNoteData, ValueTreeIdentifiers::MidiDuration);
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

void NodeCanvas::makeRTGraph(const juce::ValueTree& nodeValueTree)
{

    jassert(nodeValueTree.isValid());

    int rootNodeId = nodeValueTree.getProperty(ValueTreeIdentifiers::RootNodeId);
    if (rootNodeId == 0) return;
    juce::ValueTree rootNodeValueTree = ValueTreeState::getNode(rootNodeId);
    if (!rootNodeValueTree.isValid()) return;

    auto rtGraph = std::make_shared<RTGraph>();

    rtGraph->graphID   = rootNodeId;
    rtGraph->loopLimit = rootNodeValueTree.getProperty(ValueTreeIdentifiers::LoopLimit, 0);

    std::unordered_map<int,juce::ValueTree> tempNodeMap;

    std::vector<juce::ValueTree> stack = {rootNodeValueTree};

    while(!stack.empty()){

        juce::ValueTree currentValueTree = stack.back();
        juce::ValueTree nodeValueTreeChildren = currentValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);
        juce::ValueTree nodeMidiNotes = currentValueTree.getChildWithName(ValueTreeIdentifiers::MidiNotesData);
        juce::ValueTree nodeParentValueTree = ValueTreeState::getNodeParent(currentValueTree.getProperty(ValueTreeIdentifiers::Id));

        juce::Identifier nodeType = currentValueTree.getType();
        juce::Identifier nodeParentType = nodeParentValueTree.getType();

        stack.pop_back();

        int nodeId = currentValueTree.getProperty(ValueTreeIdentifiers::Id);
        int graphId = currentValueTree.getProperty(ValueTreeIdentifiers::RootNodeId);
        int countLimit = currentValueTree.getProperty(ValueTreeIdentifiers::CountLimit);

        if(tempNodeMap.count(nodeId) == false){

            tempNodeMap[nodeId] = currentValueTree;

            RTNode rtNode;
            rtNode.graphID = graphId;
            rtNode.nodeID = nodeId;
            rtNode.countLimit = countLimit;

            auto nodeIt = nodeMap.find(nodeId);
            if (nodeIt != nodeMap.end()) {
                Node* nodeFromTree = nodeIt->second;
                for (auto& [childId, nodeArrow] : nodeFromTree->nodeArrows)
                    rtNode.durationMap[childId] = nodeArrow->duration;
            }

            if (nodeType == ValueTreeIdentifiers::NodeData) {
                rtNode.nodeType = RTNode::NodeType::Node;
            }
            if (nodeType == ValueTreeIdentifiers::RootNodeData) {
                rtNode.nodeType = RTNode::NodeType::RootNode;
            }
            if (nodeType == ValueTreeIdentifiers::ConnectorData) {
                rtNode.nodeType = RTNode::NodeType::Connector;
            }
            if (nodeType == ValueTreeIdentifiers::ModulatorRootData) {
                rtNode.nodeType = RTNode::NodeType::ModulatorRoot;
            }
            if (nodeType == ValueTreeIdentifiers::ModulatorData) {
                rtNode.nodeType = RTNode::NodeType::Modulator;
            }
            if (nodeParentType == ValueTreeIdentifiers::ConnectorData) {
                rtNode.graphID = rtNode.nodeID;
            }

            for (int i = 0; i < nodeMidiNotes.getNumChildren(); i++) {
                juce::ValueTree note = nodeMidiNotes.getChild(i);
                RTNote rtNote;

                int pitch   = note.getProperty(ValueTreeIdentifiers::MidiPitch);
                int velocity= note.getProperty(ValueTreeIdentifiers::MidiVelocity);
                int duration= note.getProperty(ValueTreeIdentifiers::MidiDuration);

                rtNote.pitch    = pitch;
                rtNote.velocity = velocity;
                rtNote.duration = duration;
                rtNode.notes.push_back(std::move(rtNote));
            }

            rtGraph->nodeMap[nodeId] = std::move(rtNode);

            for (int i = 0; i < nodeValueTreeChildren.getNumChildren(); i++) {
                juce::ValueTree childIdTree = nodeValueTreeChildren.getChild(i);
                int childId = childIdTree.getProperty(ValueTreeIdentifiers::Id);
                juce::ValueTree childDataTree = ValueTreeState::getNode(childId);
                if (childDataTree.isValid()) {
                    stack.push_back(childDataTree);
                }
            }
        }
    }


    for (auto& [id, nodeVT] : tempNodeMap) {
        juce::ValueTree nodeChildrenIds = nodeVT.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

        for (int i = 0; i < nodeChildrenIds.getNumChildren(); i++) {
            juce::ValueTree childIdTree = nodeChildrenIds.getChild(i);
            int childId = childIdTree.getProperty(ValueTreeIdentifiers::Id);

            juce::ValueTree childDataTree = ValueTreeState::getNode(childId);
            if (!childDataTree.isValid()) DBG("INVALID CHILD FOUND");

            rtGraph->nodeMap[id].children.push_back(childId);
        }
    }

    rtGraph->traversalRequested = start;

    nodeMaps[rtGraph->graphID] = tempNodeMap;
    rtGraphs[rtGraph->graphID] = rtGraph;
    lastGraph = rtGraph;

    applicationContext.processor->setNewGraph(rtGraph);
}

void NodeCanvas::updateDurationMap(int nodeId)
{
    auto* processor = applicationContext.processor;
    auto snap = std::atomic_load(&processor->audioSnapshot);
    if (!snap || !snap->globalNodes) return;

    // 1. Find the node and its parent
    auto nodeIt = nodeMap.find(nodeId);
    if (nodeIt == nodeMap.end()) return;
    Node* node = nodeIt->second;

    juce::ValueTree nodeVT = ValueTreeState::getNode(nodeId);
    if (!nodeVT.isValid()) return;

    auto newSnap = std::make_shared<SequenceTreeAudioProcessor::AudioSnapshot>();
    newSnap->globalNodes = std::make_shared<NodeMap>(*snap->globalNodes);
    newSnap->rtGraphs    = snap->rtGraphs;

    // 2. Update this node's outgoing durations
    auto globalNodeIt = newSnap->globalNodes->find(nodeId);
    if (globalNodeIt != newSnap->globalNodes->end())
    {
        globalNodeIt->second.durationMap.clear();
        for (auto& [childId, arrow] : node->nodeArrows)
            globalNodeIt->second.durationMap[childId] = arrow->duration;
    }

    // 3. Update parent's duration pointing to this node (incoming arrow)
    juce::ValueTree parentVT = ValueTreeState::getNodeParent(nodeId);
    if (parentVT.isValid())
    {
        int parentId = parentVT.getProperty(ValueTreeIdentifiers::Id);
        auto parentIt = nodeMap.find(parentId);
        if (parentIt != nodeMap.end())
        {
            Node* parentNode = parentIt->second;
            auto globalParentIt = newSnap->globalNodes->find(parentId);
            if (globalParentIt != newSnap->globalNodes->end())
            {
                globalParentIt->second.durationMap.clear();
                for (auto& [childId, arrow] : parentNode->nodeArrows) {
                    globalParentIt->second.durationMap[childId] = arrow->duration;
                }
            }
        }
    }

    std::atomic_store(&processor->audioSnapshot, newSnap);
}

void NodeCanvas::destroyRTGraph(Node* root) { }

void NodeCanvas::setProcessorPlayblack(bool isPlaying)
{
    start = isPlaying;
    applicationContext.processor->isPlaying.store(start);

    for(auto& [graphID,graph] : rtGraphs) {
        graph.get()->traversalRequested = start;
        applicationContext.processor->setNewGraph(graph);
    }

    if (! isPlaying)
        resetAllArrowProgress();
}

void NodeCanvas::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    if (parent.getType() == ValueTreeIdentifiers::NodeMap)
    {
            jassert(child.getType() == ValueTreeIdentifiers::NodeData
                || child.getType() == ValueTreeIdentifiers::ConnectorData
                || child.getType() == ValueTreeIdentifiers::RootNodeData
                || child.getType() == ValueTreeIdentifiers::ModulatorRootData
                || child.getType() == ValueTreeIdentifiers::ModulatorData);

            AsyncUpdate asyncUpdate;
            asyncUpdate.type         = AsyncUpdateType::NodeAdded;
            asyncUpdate.nodeId    = child.getProperty(ValueTreeIdentifiers::Id);
            asyncUpdate.rootNodeId= child.getProperty(ValueTreeIdentifiers::RootNodeId);

            asyncUpdates.push_back(asyncUpdate);
            triggerAsyncUpdate();
    }
}

void NodeCanvas::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex)
{
    if (parent.getType() == ValueTreeIdentifiers::NodeMap)
    {
        AsyncUpdate asyncUpdate;
        asyncUpdate.type       = AsyncUpdateType::NodeRemoved;
        asyncUpdate.nodeId     = child.getProperty(ValueTreeIdentifiers::Id);
        asyncUpdate.rootNodeId = child.getProperty(ValueTreeIdentifiers::RootNodeId);
        asyncUpdates.push_back(asyncUpdate);
        triggerAsyncUpdate();
    }
}

void NodeCanvas::valueTreePropertyChanged(juce::ValueTree &tree, const juce::Identifier &propertyIdentifier)
{
    juce::Identifier nodeType = tree.getType();

   if (propertyIdentifier == ValueTreeIdentifiers::XPosition
       || propertyIdentifier == ValueTreeIdentifiers::YPosition
       || propertyIdentifier == ValueTreeIdentifiers::Radius
       )
   {
       jassert(nodeType == ValueTreeIdentifiers::NodeData
       || nodeType == ValueTreeIdentifiers::ConnectorData
       || nodeType == ValueTreeIdentifiers::RootNodeData
       || nodeType == ValueTreeIdentifiers::ModulatorRootData
       || nodeType == ValueTreeIdentifiers::ModulatorData);

       AsyncUpdate asyncUpdate;
       asyncUpdate.type = AsyncUpdateType::NodeMoved;
       asyncUpdate.nodeId = tree.getProperty(ValueTreeIdentifiers::Id);

       asyncUpdates.push_back(asyncUpdate);
       triggerAsyncUpdate();
   }
    else if (propertyIdentifier == ValueTreeIdentifiers::MidiDuration) {
        juce::ValueTree noteNode = tree.getParent().getParent();
        AsyncUpdate asyncUpdate;
        asyncUpdate.type = AsyncUpdateType::DurationOnly;
        asyncUpdate.nodeId = noteNode.getProperty(ValueTreeIdentifiers::Id);

        asyncUpdates.push_back(asyncUpdate);
        triggerAsyncUpdate();
    }
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
        juce::ValueTree nodeValueTreeChildren = nodeValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);
        juce::Identifier treeType = nodeValueTree.getType();

        std::unique_ptr<Node> canvasNode;
        if (treeType == ValueTreeIdentifiers::RootNodeData)
            canvasNode = std::make_unique<RootNode>(applicationContext);
        else if (treeType == ValueTreeIdentifiers::ConnectorData)
            canvasNode = std::make_unique<Connector>(applicationContext);
        else if (treeType == ValueTreeIdentifiers::ModulatorData || treeType == ValueTreeIdentifiers::ModulatorRootData)
            canvasNode = std::make_unique<Modulator>(applicationContext);
        else
            canvasNode = std::make_unique<Node>(applicationContext);

        int nodeId = nodeValueTree.getProperty(ValueTreeIdentifiers::Id);

        if (treeType == ValueTreeIdentifiers::RootNodeData) {
            rootNodeMap[nodeId] = nodeValueTree;
        }

        for (int i = 0; i < nodeValueTreeChildren.getNumChildren(); i++) {
            juce::ValueTree childIdTree = nodeValueTreeChildren.getChild(i);
            int childId = childIdTree.getProperty(ValueTreeIdentifiers::Id);

            NodePair nodePair;
            nodePair.parentNodeId = nodeId;
            nodePair.childNodeId = childId;

            nodePairs.push_back(nodePair);
        }

        juce::ValueTree midiNotes = nodeValueTree.getChildWithName(ValueTreeIdentifiers::MidiNotesData);
        canvasNode->nodeValueTree = nodeValueTree;
        canvasNode->midiNoteData  = midiNotes.getChildWithName(ValueTreeIdentifiers::MidiNoteData);
        canvasNode->setComponentID(std::to_string(nodeId));
        canvasNode->setDisplayMode(NodeDisplayMode::Pitch);

        addAndMakeVisible(canvasNode.get());
        nodeMap[nodeId] = canvasNode.release();

        setNodePosition(nodeId);
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
            makeRTGraph(rootNodeValueTree);
    }

}