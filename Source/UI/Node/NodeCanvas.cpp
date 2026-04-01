// NodeCanvas.cpp
#include "../Util/PluginContext.h"
#include "NodeCanvas.h"

#include <stack>

#include "../../Util/ValueTreeState.h"
#include "../Core/PluginProcessor.h"

// Canvas Related Functions //
NodeCanvas::NodeCanvas()
{
    setLookAndFeel(ComponentContext::lookAndFeel);


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
    std::unique_ptr<Node> childNode = nullptr;

    if (nodeValueTree.getType() == ValueTreeState::RootNodeData) {
        childNode = std::make_unique<Node>();
    }
    if (nodeValueTree.getType() == ValueTreeState::NodeData) {
        childNode = std::make_unique<Node>();
    }
    else if (nodeValueTree.getType() == (ValueTreeState::ConnectorData)) {
        childNode = std::make_unique<Connector>();
    }

    childNode->setComponentID(std::to_string(nodeId));

    juce::ValueTree nodeParent = ValueTreeState::getNodeParent(nodeId);

    if (nodeParent.isValid()) {
        int parentNodeId = nodeParent.getProperty(ValueTreeState::Id);
        auto parentNodePair = nodeMap.find(parentNodeId);
        if (parentNodePair != nodeMap.end()) {
            Node* parentNode = parentNodePair->second;
            addLinePoints(parentNode, childNode.get());
        }
    }


    addAndMakeVisible(childNode.get());
    nodeMap[nodeId] = childNode.release();

    makeRTGraph(nodeValueTree);

    int rootNodeId = nodeValueTree.getProperty(ValueTreeState::RootNodeId);
    juce::ValueTree connectorParent = ValueTreeState::getNodeParent(rootNodeId);

    if (connectorParent.isValid()) {

        int connectorRootId = connectorParent.getProperty(ValueTreeState::RootNodeId);

        juce::ValueTree connectorRootTree = ValueTreeState::getNode(connectorRootId);

        if (connectorRootTree.isValid()) {
            makeRTGraph(connectorRootTree);
        }
    }
}

void NodeCanvas::removeNodeFromCanvas(int nodeId)
{
    auto nodePair = nodeMap.find(nodeId);
    juce::ValueTree nodeValueTree = ValueTreeState::getNode(nodeId);

    jassert(nodePair != nodeMap.end());

    auto& node = nodePair->second;
    Node* temp = node->root;

    nodeMaps[node->root->nodeID].erase(node->nodeID);
    removeLinePoints(node);

    makeRTGraph(nodeValueTree);
    repaint();
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

void NodeCanvas::addLinePoints(Node* startNode, Node* endNode) {

    auto* arrow = new NodeArrow(startNode, endNode);
    addAndMakeVisible(arrow);
    arrow->toBack();
    arrow->setInterceptsMouseClicks(false,false);
    nodeArrows.add(arrow);
}

void NodeCanvas::removeLinePoints(Node* node)
{
    for (int i = nodeArrows.size() - 1; i >= 0; i--)
    {
        NodeArrow* nodeArrow = nodeArrows[i];
        if (nodeArrow->startNode != node && nodeArrow->endNode != node) { continue; }

        nodeArrows.remove(i);
    }
}

void NodeCanvas::updateLinePoints(Node* movedNode)
{
    for (NodeArrow* arrow : nodeArrows)
    {

        if (arrow->startNode != movedNode && arrow->endNode != movedNode) {
            DBG("NODE ARROW NOT LOCATED");
            continue;
        }

        juce::Point start = movedNode->getBounds().getCentre();
        juce::Point end = arrow->endNode->getBounds().getCentre();

        if (arrow->startNode != movedNode)
        {
            start = arrow->endNode->getBounds().getCentre();
            end = arrow->startNode->getBounds().getCentre();
        }

        juce::Rectangle arrowBounds = juce::Rectangle<int>::leftTopRightBottom(
            std::min(start.x,end.x),
            std::min(start.y,end.y),
            std::max(start.x,end.x),
            std::max(start.y,end.y)
            ).expanded(2);

        // arrow->startNode.nodeData.midiNoteData.setProperty()
        // arrowBounds.getWidth();

        arrow->setBounds(arrowBounds);
        arrow->repaint();
    }
}



// processor-related Functions //

void NodeCanvas::makeRTGraph(const juce::ValueTree& nodeValueTree)
{
    //CONVERT GRAPH TO FORM THAT TAKES VALUE TREE INSTEAD
    DBG("MAKING RT GRAPH");

    jassert(nodeValueTree.isValid());

    int rootNodeId = nodeValueTree.getProperty(ValueTreeState::RootNodeId);
    juce::ValueTree rootNodeValueTree = ValueTreeState::getNode(rootNodeId);

    auto rtGraph = std::make_shared<RTGraph>();

    rtGraph->graphID = rootNodeId;


    std::unordered_map<int,juce::ValueTree> tempNodeMap;

    std::vector<juce::ValueTree> stack = {rootNodeValueTree};

    while(!stack.empty()){

        juce::ValueTree currentValueTree = stack.back();
        juce::ValueTree nodeValueTreeChildren = currentValueTree.getChildWithName(ValueTreeState::NodeChildrenIds);
        juce::ValueTree nodeMidiNotes = currentValueTree.getChildWithName(ValueTreeState::MidiNotesData);
        juce::ValueTree nodeParentValueTree = ValueTreeState::getNodeParent(currentValueTree.getProperty(ValueTreeState::Id));

        juce::Identifier nodeType = currentValueTree.getType();
        juce::Identifier nodeParentType = nodeParentValueTree.getType();

        stack.pop_back();

        int nodeId = currentValueTree.getProperty(ValueTreeState::Id);
        int graphId = currentValueTree.getProperty(ValueTreeState::RootNodeId);
        int countLimit = currentValueTree.getProperty(ValueTreeState::CountLimit);

        if(tempNodeMap.count(nodeId) == false){

            tempNodeMap[nodeId] = currentValueTree;

            RTNode rtNode;
            rtNode.graphID = graphId;
            rtNode.nodeID = nodeId;
            rtNode.countLimit = countLimit;;

            if (nodeType == ValueTreeState::NodeData || nodeType == ValueTreeState::RootNodeData) {rtNode.nodeType = RTNode::NodeType::Node;}
            if (nodeType == ValueTreeState::ConnectorData ) { rtNode.nodeType = RTNode::NodeType::RelayNode; }
            if (nodeParentType == ValueTreeState::ConnectorData) { rtNode.graphID = rtNode.nodeID; }

            for (int i = 0; i < nodeMidiNotes.getNumChildren(); i++) {
                juce::ValueTree note = nodeMidiNotes.getChild(i);
                RTNote rtNote;

                int pitch   = note.getProperty(ValueTreeState::MidiPitch);
                int velocity= note.getProperty(ValueTreeState::MidiVelocity);
                int duration= note.getProperty(ValueTreeState::MidiDuration);

                rtNote.pitch    = pitch;
                rtNote.velocity = velocity;
                rtNote.duration = duration;
                rtNode.notes.push_back(std::move(rtNote));
            }

            rtGraph->nodeMap[nodeId] = std::move(rtNode);

            for (int i = 0; i < nodeValueTreeChildren.getNumChildren(); i++) {
                juce::ValueTree childIdTree = nodeValueTreeChildren.getChild(i);
                int childId = childIdTree.getProperty(ValueTreeState::Id);
                juce::ValueTree childDataTree = ValueTreeState::getNode(childId);
                if (childDataTree.isValid()) {
                    stack.push_back(childDataTree);
                }
            }
        }
    }


    for (auto& [id, nodeVT] : tempNodeMap) {
        juce::ValueTree nodeChildrenIds = nodeVT.getChildWithName(ValueTreeState::NodeChildrenIds);
        bool isConnector = nodeVT.getType() == ValueTreeState::ConnectorData;

        for (int i = 0; i < nodeChildrenIds.getNumChildren(); i++) {
            juce::ValueTree childIdTree = nodeChildrenIds.getChild(i);
            int childId = childIdTree.getProperty(ValueTreeState::Id);

            juce::ValueTree childDataTree = ValueTreeState::getNode(childId);
            if (!childDataTree.isValid()) { continue; }

            if (childDataTree.getType() == ValueTreeState::RootNodeData && isConnector) {
                rtGraph->nodeMap[id].connectors.push_back(childId);
            }
            else if (childDataTree.getType() == ValueTreeState::NodeData
                  || childDataTree.getType() == ValueTreeState::RootNodeData) {
                rtGraph->nodeMap[id].children.push_back(childId);
                  }
            else if (childDataTree.getType() == ValueTreeState::ConnectorData) {
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
    if (parent.getType() == ValueTreeState::NodeMap)
    {
            jassert(child.getType() == ValueTreeState::NodeData
                || child.getType() == ValueTreeState::ConnectorData
                || child.getType() == ValueTreeState::RootNodeData);

            AsyncUpdate asyncUpdate;
            asyncUpdate.type         = AsyncUpdateType::NodeAdded;
            asyncUpdate.nodeId    = child.getProperty(ValueTreeState::Id);
            asyncUpdate.rootNodeId= child.getProperty(ValueTreeState::RootNodeId);

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
   if (propertyIdentifier == ValueTreeState::XPosition
       || propertyIdentifier == ValueTreeState::YPosition
       || propertyIdentifier == ValueTreeState::Radius)
   {
       jassert(tree.getType() == ValueTreeState::NodeData
                || tree.getType() == ValueTreeState::ConnectorData
                || tree.getType() == ValueTreeState::RootNodeData);

       AsyncUpdate asyncUpdate;
       asyncUpdate.type = AsyncUpdateType::NodeMoved;
       asyncUpdate.nodeId = tree.getProperty(ValueTreeState::Id);

       asyncUpdates.push_back(asyncUpdate);
       triggerAsyncUpdate();
   }
}

void NodeCanvas::setSelectionMode(NodeBox::DisplayMode mode) const {

    for(int i = 0; i < nodeMap.size(); i++)
    {
        auto nodePair = nodeMap.find(i);
        jassert(nodePair == nodeMap.end());
        Node* node = nodePair->second;

        node->setDisplayMode(mode);
        node->editor.get()->formatDisplay(mode);
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
        juce::ValueTree nodeValueTreeChildren = nodeValueTree.getChildWithName(ValueTreeState::NodeChildrenIds);
        juce::Identifier treeType = nodeValueTree.getType();

        auto canvasNode = std::make_unique<Node>();

        int xPosition = nodeValueTree.getProperty      (ValueTreeState::XPosition);
        int yPosition = nodeValueTree.getProperty      (ValueTreeState::YPosition);
        int radius    = nodeValueTree.getProperty      (ValueTreeState::Radius);
        int nodeId    = nodeValueTree.getProperty      (ValueTreeState::Id);

        if (treeType == ValueTreeState::RootNodeData) {
            rootNodeMap[nodeId] = nodeValueTree;
        }

        for (int i = 0; i < nodeValueTreeChildren.getNumChildren(); i++) {
            juce::ValueTree childIdTree = nodeValueTreeChildren.getChild(i);
            int childId = childIdTree.getProperty(ValueTreeState::Id);

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