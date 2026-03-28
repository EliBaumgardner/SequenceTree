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



// Node Related Functions //

void NodeCanvas::removeNode(Node* node)
{
    Node* temp = node->root;
    
    if(!node->nodeData.children.isEmpty())
    {
        for (Node* child : node->nodeData.children) { child->parent = nullptr; }
    }

    if (node->parent != nullptr)
    {
        node->parent->nodeData.removeChild(node);
    }

    nodeMaps[node->root->nodeID].erase(node->nodeID);
    removeLinePoints(node);
    canvasNodes.removeObject(node);

    makeRTGraph(temp);
    repaint();
}

void NodeCanvas::setSelectionMode(NodeBox::DisplayMode mode) const {
    std::cout<<"selecting display mode"<<std::endl;

    for(int i = 0; i < canvasNodes.size(); i++)
    {
        canvasNodes[i]->setDisplayMode(mode);
        canvasNodes[i]->editor.get()->formatDisplay(mode);
    }
}



// MouseEvent Related Functions //

void NodeCanvas::mouseDown(const juce::MouseEvent& e)
{

    // jassert(ComponentContext::nodeController != nullptr);
    // jassert(ComponentContext::valueTreeState != nullptr);
    //
    // ValueTreeState& valueTreeState = *ComponentContext::valueTreeState;
    // NodeController& nodeController = *ComponentContext::nodeController;
    // juce::UndoManager& undoManager = *ComponentContext::undoManager;
    //
    // if (!e.mods.isLeftButtonDown() || nodeController.nodeControllerMode != NodeController::NodeControllerMode::Node || !e.mods.isShiftDown()) {
    //     return;
    // }
    //
    // juce::ValueTree valueTree = valueTreeState.addRootNode(&undoManager);
    // int rootId = valueTree.getProperty(ValueTreeState::Id);
    //
    // int xPosition = valueTree.getProperty(ValueTreeState::XPosition);
    // int yPosition = valueTree.getProperty(ValueTreeState::YPosition);
    // int radius = valueTree.getProperty(ValueTreeState::Radius);
    //
    // valueTreeState.setNodePosition(rootId, xPosition, yPosition,radius,&undoManager);
}

void NodeCanvas::addRootNode(int nodeId) {

    // std::unique_ptr<Node> rootNode = std::make_unique<Node>();
    //
    // ValueTreeState::NodePosition nodePosition = ValueTreeState::getNodePosition(nodeId);
    //
    // int radius = nodePosition.radius;
    //
    // int xPosition = nodePosition.xPosition - radius;
    // int yPosition = nodePosition.yPosition - radius;
    // int width = radius * 2;
    // int height = radius * 2;
    //
    //
    // rootNode->setBounds(xPosition, yPosition, width, height);
    //
    // addAndMakeVisible(rootNode.get());
    // makeRTGraph(rootNode.get());
    //
    // canvasNodes.add(rootNode.release());
}

void NodeCanvas::mouseDrag(const juce::MouseEvent& e)
{
    // if (auto* parent = dynamic_cast<DynamicPort*>(getParentComponent())){
    //     auto parentEvent = e.getEventRelativeTo(parent);
    //     parent->mouseDrag(parentEvent);
    // }
}



//linePoint Functions//

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

        if (arrow->startNode != movedNode && arrow->endNode != movedNode) { continue; }

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

void NodeCanvas::makeRTGraph(Node* root)
{
    //CONVERT GRAPH TO FORM THAT TAKES VALUE TREE INSTEAD
    DBG("MAKING RT GRAPH");

    if (root == nullptr) { DBG("ROOT NODE IS NULL"); }

    auto rtGraph = std::make_shared<RTGraph>();

    rtGraph->graphID = root->nodeID;

    std::unordered_map<int,Node*> nodeMap;

    std::vector<Node*> stack = {root};

    while(!stack.empty()){

        Node* current = stack.back();
        stack.pop_back();
        int id = current->nodeID;

        if(nodeMap.count(id) == false){

            nodeMap[id] = current;

            RTNode rtNode;
            rtNode.graphID = rtGraph->graphID;
            rtNode.nodeID = id;
            rtNode.countLimit = static_cast<int>(current->nodeData.nodeData.getProperty("countLimit"));

            if (auto traverser = dynamic_cast<Connector*>(current))      { rtNode.nodeType = RTNode::NodeType::RelayNode; }
            if (auto parent = dynamic_cast<Connector*>(current->parent)) { rtNode.graphID = rtNode.nodeID; }

            for(const auto& note : current->nodeData.midiNotes){

                RTNote rtNote;

                float pitch = static_cast<float>(note.getProperty("pitch"));
                float velocity = static_cast<float>(note.getProperty("velocity"));
                float duration = static_cast<float>(note.getProperty("duration"));

                rtNote.pitch = pitch;
                rtNote.velocity = velocity;
                rtNote.duration = duration;
                rtNode.notes.push_back(std::move(rtNote));
            }

            rtGraph->nodeMap[id] = std::move(rtNode);

            for(auto child : current->nodeData.children){ stack.push_back(child); }
        }
    }

    for(auto& [id, node] : nodeMap){
        for (auto& child : node->nodeData.children)       { rtGraph->nodeMap[id].children.push_back(child->nodeID); }
        for (auto& connector : node->nodeData.connectors) { rtGraph->nodeMap[id].connectors.push_back(connector->nodeID); }
    }

    rtGraph->traversalRequested = start;

    nodeMaps[rtGraph->graphID] = nodeMap;
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

void NodeCanvas::setValueTreeState(const juce::ValueTree& stateTree)
{
    // clearCanvas();
    //
    // std::unordered_map<int,Node*> nodeMap;
    //
    // if (stateTree.getNumChildren() == 0) { DBG("stateTree is empty"); }
    //
    // for (int i = 0 ; i < stateTree.getNumChildren(); i++) {
    //
    //     juce::ValueTree treeChild = stateTree.getChild(i);
    //
    //     if (treeChild.getType() == juce::Identifier("NodeData")) {
    //
    //         int xPos = treeChild.getProperty("x");
    //         int yPos = treeChild.getProperty("y");
    //         int radius = treeChild.getProperty("radius");
    //         int count = treeChild.getProperty("count");
    //         int countLimit = treeChild.getProperty("countLimit");
    //         int nodeID = treeChild.getProperty("nodeID");
    //
    //         Node* canvasNode;
    //
    //         if (treeChild.getProperty("nodeType").toString() ==  "Connector") {
    //             DBG("created relay node");
    //             canvasNode = new Connector();
    //         }
    //         else if (treeChild.getProperty("nodeType").toString() == "Node"){
    //             DBG("created node");
    //             canvasNode = new Node();
    //         }
    //         else { DBG("INVALID TYPE");}
    //
    //         canvasNodes.add(canvasNode);
    //         canvasNode->setCentrePosition(xPos, yPos);
    //         canvasNode->nodeID = nodeID;
    //
    //         canvasNode->setSize(radius*2, radius*2);
    //
    //         canvasNode->nodeData.nodeData.setProperty("x",xPos,nullptr);
    //         canvasNode->nodeData.nodeData.setProperty("y",yPos,nullptr);
    //         canvasNode->nodeData.nodeData.setProperty("radius",radius,nullptr);
    //
    //         canvasNode->nodeData.nodeData.setProperty("count",count,nullptr);
    //         canvasNode->nodeData.nodeData.setProperty("countLimit",countLimit,nullptr);
    //         canvasNode->nodeData.nodeData.setProperty("nodeID",nodeID,nullptr);
    //         addAndMakeVisible(canvasNode);
    //
    //         nodeMap[nodeID] = canvasNode;
    //     }
    // }
    //
    // std::unordered_map<int,Node*> childMap;
    //
    //
    // for (int i =0; i < stateTree.getNumChildren(); i++) {
    //     juce::String index = std::to_string(i);
    //     DBG("iterating child no:" + index);
    //     juce::ValueTree treeChild = stateTree.getChild(i);
    //
    //     if (treeChild.getType() == juce::Identifier("NodeData")) {
    //
    //         for (int i = 0; i < treeChild.getNumChildren(); i++) {
    //
    //             juce::ValueTree subTreeChild = treeChild.getChild(i);
    //             if (subTreeChild.getType() != juce::Identifier("NodeData")) {
    //                 continue;
    //             }
    //             int childNodeID = subTreeChild.getProperty("nodeID");
    //             int parentNodeID= treeChild.getProperty("nodeID");
    //
    //             if (!childNodeID){ DBG("child nodeID is empty"); }
    //
    //             Node* childNode  = nodeMap[childNodeID];
    //             Node* parentNode = nodeMap[parentNodeID];
    //
    //             bool isRoot = false;
    //
    //             if (treeChild.getProperty("nodeType").toString() == "Connector") {
    //                 parentNode->nodeData.addConnector(childNode);
    //                 isRoot = true;
    //             }
    //             else {
    //                 parentNode->nodeData.addChild(childNode);
    //             }
    //
    //
    //
    //             if (parentNode == nullptr) { DBG("PARENT NODE IS NULL"); }
    //             if (childNode == nullptr)  { DBG("CHILD NODE IS NULL");  }
    //
    //             DBG("line point made");
    //             addLinePoints(parentNode,childNode);
    //             DBG("updating line points");
    //             updateLinePoints(childNode);
    //             DBG("adding node to childMap");
    //             if (!isRoot) {
    //                 childMap[childNodeID] = childNode;
    //             }
    //             DBG("done adding node to childmap");
    //         }
    //     }
    //     DBG("done configuring node data");
    // }
    // DBG("done configuring all data of node");
    //
    // DBG("root map made");
    // std::unordered_map<int,Node*> rootMap;
    //
    // DBG("adding roots to map");
    // for (auto& [index,node]: nodeMap) {
    //     if (childMap.count(index) == 0) { rootMap[index] = node; }
    // }
    //
    // for (auto& [index,rootNode] : rootMap) {
    //     std::vector<Node*> stack;
    //     stack.push_back(rootNode);
    //
    //     while (!stack.empty()) {
    //         Node* current = stack.back();
    //         stack.pop_back();
    //
    //         current->root = rootNode;
    //
    //         for (Node* child : current->nodeData.children) {
    //             stack.push_back(child);
    //             child->root = current->root;
    //             child->parent = current;
    //         }
    //     }
    //
    //     makeRTGraph(rootNode);
    // }
}

void NodeCanvas::clearCanvas()
{
    if (canvasNodes.isEmpty()) { DBG("CLEAR CANVAS CALLED ON EMPTY CANVAS"); return; }

    nodeArrows.clear();
    canvasNodes.clear();
}

void NodeCanvas::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    int parentId = parent.getProperty(ValueTreeState::Id);
    int childId = child.getProperty(ValueTreeState::Id);
    DBG("VALUETREE CHILD ADDED: " + std::to_string(parentId) + " " + std::to_string(childId) + "");
    if (parent.getType() == ValueTreeState::NodeMap)
    {
        DBG("NODE TREE CHILD ADDED");
        if (child.getType() == ValueTreeState::RootNodeData) {

            std::unique_ptr<Node> childNode = std::make_unique<Node>();
            childNode->setComponentID(std::to_string(childId));

            NodePosition nodePosition = ValueTreeState::getNodePosition(childId);
            int xPosition = nodePosition.xPosition;
            int yPosition = nodePosition.yPosition;
            int radius = nodePosition.radius;

            addAndMakeVisible(childNode.get());
            childNode.get()->setBounds(100, 100, 40, 40);
            childNode.get()->repaint();
            canvasNodes.add(childNode.release());

        }
    }
    else if (parent.getType() == ValueTreeState::NodeData)
    {
        DBG("NODE DATA CHILD ADDED");

    }
    else if (parent.getType() == ValueTreeState::RootNodeData)
    {
        DBG("ROOT NODE DATA CHILD ADDED");

    }
    else if (parent.getType() == ValueTreeState::ConnectorData)
    {
        DBG("CONNECTOR DATA CHILD ADDED");
    }
    else {
        DBG(parent.getType());
        DBG(child.getType());
    }

}

void NodeCanvas::valueTreeChildRemoved(juce::ValueTree& parent,juce::ValueTree& child, int childIndex)
{
    DBG("valueTreeChildRemoved");
}

void NodeCanvas::valueTreePropertyChanged(juce::ValueTree &tree, const juce::Identifier &property)
{
    DBG("valueTreePropertyChanged");
}
