// NodeCanvas.cpp

#include <juce_gui_basics/juce_gui_basics.h>

#include "CustomLookAndFeel.h"
#include "../Util/ValueTreeState.h"
#include "../Core/PluginProcessor.h"
#include "NodeTextEditor.h"
#include "NodeArrow.h"
#include "../Util/ValueTreeIdentifiers.h""

#include "Node.h"
#include "Connector.h"
#include "NodeCanvas.h"


// Canvas Related Functions //
NodeCanvas::NodeCanvas()
{
    setLookAndFeel(ComponentContext::lookAndFeel);
}

NodeCanvas::~NodeCanvas() {
    nodeArrows.clear();
}


void NodeCanvas::paint(juce::Graphics& g)
{
    if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel()))
    {
        customLookAndFeel->drawCanvas(g,*this);
    }
}

void NodeCanvas::handleAsyncUpdate() {
    for (auto& asyncUpdate  : asyncUpdates) {
        int nodeId = asyncUpdate.nodeId;

        AsyncUpdateType updateType = asyncUpdate.type;

        if (updateType == AsyncUpdateType::NodeAdded) {
            addNodeToCanvas(nodeId);
        }
        else if (updateType == AsyncUpdateType::NodeRemoved) {
            removeNodeFromCanvas(nodeId);
        }
        else if (updateType == AsyncUpdateType::NodeMoved) {
            setNodePosition(nodeId);
        }
    }
    asyncUpdates.clear();
}

void NodeCanvas::addNodeToCanvas(int nodeId)
{
    juce::ValueTree nodeValueTree = ValueTreeState::getNode(nodeId);
    juce::ValueTree nodeParent = ValueTreeState::getNodeParent(nodeId);
    juce::ValueTree midiNotes  = ValueTreeState::getMidiNotes(nodeId);
    juce::ValueTree midiNote = midiNotes.getChildWithName(ValueTreeIdentifiers::MidiNoteData);

    std::unique_ptr<Node> childNode = nullptr;

    if (nodeValueTree.getType() == ValueTreeIdentifiers::RootNodeData
        || nodeValueTree.getType() == ValueTreeIdentifiers::NodeData) {
        childNode = std::make_unique<Node>();
    }
    else if (nodeValueTree.getType() == (ValueTreeIdentifiers::ConnectorData)) {
        childNode = std::make_unique<Connector>();
    }

    jassert(childNode);

    childNode->setComponentID(std::to_string(nodeId));
    childNode->nodeValueTree = nodeValueTree;
    childNode->midiNoteData = midiNote;


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

    makeRTGraph(nodeValueTree);

    int rootNodeId = nodeValueTree.getProperty(ValueTreeIdentifiers::RootNodeId);
    juce::ValueTree connectorParent = ValueTreeState::getNodeParent(rootNodeId);

    if (connectorParent.isValid()) {

        int connectorRootId = connectorParent.getProperty(ValueTreeIdentifiers::RootNodeId);

        juce::ValueTree connectorRootTree = ValueTreeState::getNode(connectorRootId);

        if (connectorRootTree.isValid()) {
            makeRTGraph(connectorRootTree);
        }
    }
}

void NodeCanvas::removeNodeFromCanvas(int nodeId)
{
    // auto nodePair = nodeMap.find(nodeId);
    // juce::ValueTree nodeValueTree = ValueTreeState::getNode(nodeId);
    //
    // jassert(nodePair != nodeMap.end());
    //
    // auto& node = nodePair->second;
    // Node* temp = node->root;
    //
    // nodeMaps[node->root->nodeId].erase(node->nodeId);
    // removeLinePoints(node);
    //
    // makeRTGraph(nodeValueTree);
    // repaint();
}

void NodeCanvas::setNodePosition(int nodeId)
{
    NodePosition nodePosition = ValueTreeState::getNodePosition(nodeId);

    int xPosition = nodePosition.xPosition;
    int yPosition = nodePosition.yPosition;
    int radius = nodePosition.radius;

    auto nodePair = nodeMap.find(nodeId);
    jassert(nodePair != nodeMap.end());

    auto node = nodePair->second;
    node->setCentrePosition(xPosition,yPosition);
    node->setSize(radius*2,radius*2);

    updateLinePoints(node);
}

void NodeCanvas::addLinePoints(Node* parentNode, Node* childNode)
{
    auto arrow = std::make_unique<NodeArrow>(parentNode, childNode);


    addAndMakeVisible(arrow.get());

    arrow->toBack();
    arrow->setInterceptsMouseClicks(false,false);


    //nodeArrowMap[{parentNode,childNode}] = arrow.get();
    //nodeArrows.add(arrow.get());
}

void NodeCanvas::removeLinePoints(Node* node)
{
    for (int i = nodeArrows.size() - 1; i >= 0; i--)
    {
        NodeArrow* nodeArrow = nodeArrows[i];
        if (nodeArrow->parentNode != node && nodeArrow->childNode != node) { continue; }

        nodeArrows.remove(i);
    }
}

void NodeCanvas::updateLinePoints(Node* movedNode)
{
    for (NodeArrow* arrow : nodeArrows)
    {
        if (arrow->parentNode != movedNode && arrow->childNode != movedNode) {
            continue;
        }

        arrow->setArrowBounds(movedNode);

        int nodeId = arrow->childNode->getComponentID().getIntValue();
        juce::ValueTree nodeValueTree = ValueTreeState::getNode(nodeId);

        NodeNote note;
        note.pitch = 60;
        note.duration = 1000;
        note.velocity = 60;

        ValueTreeState::setMidiValue(nodeId,note, ComponentContext::undoManager);
    }
}

// processor-related Functions //

void NodeCanvas::makeRTGraph(const juce::ValueTree& nodeValueTree)
{
    jassert(nodeValueTree.isValid());

    int rootNodeId = nodeValueTree.getProperty(ValueTreeIdentifiers::RootNodeId);
    juce::ValueTree rootNodeValueTree = ValueTreeState::getNode(rootNodeId);

    auto rtGraph = std::make_shared<RTGraph>();

    rtGraph->graphID = rootNodeId;

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
            rtNode.countLimit = countLimit;;

            if (nodeType == ValueTreeIdentifiers::NodeData || nodeType == ValueTreeIdentifiers::RootNodeData) {rtNode.nodeType = RTNode::NodeType::Node;}
            if (nodeType == ValueTreeIdentifiers::ConnectorData ) { rtNode.nodeType = RTNode::NodeType::RelayNode; }
            if (nodeParentType == ValueTreeIdentifiers::ConnectorData) { rtNode.graphID = rtNode.nodeID; }

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
        bool isConnector = nodeVT.getType() == ValueTreeIdentifiers::ConnectorData;

        for (int i = 0; i < nodeChildrenIds.getNumChildren(); i++) {
            juce::ValueTree childIdTree = nodeChildrenIds.getChild(i);
            int childId = childIdTree.getProperty(ValueTreeIdentifiers::Id);

            juce::ValueTree childDataTree = ValueTreeState::getNode(childId);
            if (!childDataTree.isValid()) { continue; }

            if (childDataTree.getType() == ValueTreeIdentifiers::RootNodeData && isConnector) {
                rtGraph->nodeMap[id].connectors.push_back(childId);
            }
            else if (childDataTree.getType() == ValueTreeIdentifiers::NodeData
                  || childDataTree.getType() == ValueTreeIdentifiers::RootNodeData) {
                rtGraph->nodeMap[id].children.push_back(childId);
                  }
            else if (childDataTree.getType() == ValueTreeIdentifiers::ConnectorData) {
                rtGraph->nodeMap[id].connectors.push_back(childId);
            }
        }
    }

    rtGraph->traversalRequested = start;

    nodeMaps[rtGraph->graphID] = tempNodeMap;
    rtGraphs[rtGraph->graphID] = rtGraph;
    lastGraph = rtGraph;

    ComponentContext::processor->setNewGraph(rtGraph);
}

void NodeCanvas::destroyRTGraph(Node* root) { }

void NodeCanvas::setProcessorPlayblack(bool isPlaying)
{
    start = isPlaying;
    ComponentContext::processor->isPlaying.store(start);

    for(auto& [graphID,graph] : rtGraphs) {
        graph.get()->traversalRequested = start;
        ComponentContext::processor->setNewGraph(graph);
    }
}

void NodeCanvas::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    if (parent.getType() == ValueTreeIdentifiers::NodeMap)
    {
            jassert(child.getType() == ValueTreeIdentifiers::NodeData
                || child.getType() == ValueTreeIdentifiers::ConnectorData
                || child.getType() == ValueTreeIdentifiers::RootNodeData);

            AsyncUpdate asyncUpdate;
            asyncUpdate.type         = AsyncUpdateType::NodeAdded;
            asyncUpdate.nodeId    = child.getProperty(ValueTreeIdentifiers::Id);
            asyncUpdate.rootNodeId= child.getProperty(ValueTreeIdentifiers::RootNodeId);

            asyncUpdates.push_back(asyncUpdate);
            triggerAsyncUpdate();
    }
}

void NodeCanvas::valueTreeChildRemoved(juce::ValueTree& parent,juce::ValueTree& child, int childIndex)
{
    DBG("valueTreeChildRemoved");
}

void NodeCanvas::valueTreePropertyChanged(juce::ValueTree &tree, const juce::Identifier &propertyIdentifier)
{
   if (propertyIdentifier == ValueTreeIdentifiers::XPosition
       || propertyIdentifier == ValueTreeIdentifiers::YPosition
       || propertyIdentifier == ValueTreeIdentifiers::Radius)
   {
       jassert(tree.getType() == ValueTreeIdentifiers::NodeData
                || tree.getType() == ValueTreeIdentifiers::ConnectorData
                || tree.getType() == ValueTreeIdentifiers::RootNodeData);

       AsyncUpdate asyncUpdate;
       asyncUpdate.type = AsyncUpdateType::NodeMoved;
       asyncUpdate.nodeId = tree.getProperty(ValueTreeIdentifiers::Id);

       asyncUpdates.push_back(asyncUpdate);
       triggerAsyncUpdate();
   }
}

void NodeCanvas::setSelectionMode(NodeDisplayMode mode) const {

    for (auto& [nodeId, node] : nodeMap)
    {
        node->setDisplayMode(mode);
        node->nodeTextEditor->formatDisplay(mode);
    }
}

void NodeCanvas::clearCanvas()
{
    if (nodeMap.empty()) { DBG("CLEAR CANVAS CALLED ON EMPTY CANVAS"); return; }

    nodeArrows.clear();
    nodeMap.clear();
}

void NodeCanvas::setValueTreeState(const juce::ValueTree& stateTree)
{
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

        auto canvasNode = std::make_unique<Node>();

        int xPosition = nodeValueTree.getProperty      (ValueTreeIdentifiers::XPosition);
        int yPosition = nodeValueTree.getProperty      (ValueTreeIdentifiers::YPosition);
        int radius    = nodeValueTree.getProperty      (ValueTreeIdentifiers::Radius);
        int nodeId    = nodeValueTree.getProperty      (ValueTreeIdentifiers::Id);

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

        canvasNode->setCentrePosition(xPosition, yPosition);
        canvasNode->setSize(radius*2, radius*2);
        canvasNode->setComponentID(std::to_string(nodeId));

        addAndMakeVisible(canvasNode.get());

        nodeMap[nodeId] = canvasNode.release();
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

    for (int i = 0; i < rootNodeMap.size(); i++) {

        juce::ValueTree rootNodeValueTree = rootNodeMap[i];
        makeRTGraph(rootNodeValueTree);
    }

}