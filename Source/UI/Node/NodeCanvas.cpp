// NodeCanvas.cpp
#include "../Util/PluginContext.h"
#include "NodeCanvas.h"
#include "../Core/PluginProcessor.h"

// Canvas Related Functions //
NodeCanvas::NodeCanvas() { setLookAndFeel(ComponentContext::lookAndFeel); }

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
    
    if(!node->nodeData.children.isEmpty()){ for (Node* child : node->nodeData.children){ child->parent = nullptr; } }
    if (node->parent != nullptr) { node->parent->nodeData.removeChild(node); }

    nodeMaps[node->root->nodeID].erase(node->nodeID);
    removeLinePoints(node);
    canvasNodes.removeObject(node);

    makeRTGraph(temp);
    repaint();
}

void NodeCanvas::setSelectionMode(NodeBox::DisplayMode mode)
{
    for(int i = 0; i < canvasNodes.size(); i++)
    {
        canvasNodes[i]->setDisplayMode(mode);
        canvasNodes[i]->editor.get()->formatDisplay(mode);
    }
}



// MouseEvent Related Functions //

void NodeCanvas::mouseDown(const juce::MouseEvent& e)
{
    if (!e.mods.isLeftButtonDown() || controllerMode != ControllerMode::Node || !e.mods.isShiftDown()) { return; }

    Node* root = new Node();
    canvasNodes.add(root);

    auto pos = e.getPosition().toFloat();
    root->setBounds(int(pos.x) - 20, int(pos.y) - 20, 40, 40);
    addAndMakeVisible(root);

    root->root = root;
    makeRTGraph(root);

    lastPosition = e.getPosition();
    if (controllerMade == false) { controllerMade = true; controller = std::make_unique<ObjectController>(root); }
}

void NodeCanvas::addRootNode(Node* root) { }

void NodeCanvas::mouseDrag(const juce::MouseEvent& e)
{

    if (auto* parent = dynamic_cast<DynamicPort*>(getParentComponent()))
    {
        auto parentEvent = e.getEventRelativeTo(parent);
        parent->mouseDrag(parentEvent);
    }
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
    std::cout<<"making graph"<<std::endl;
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

            if (auto traverser = dynamic_cast<RelayNode*>(current))      { rtNode.nodeType = RTNode::NodeType::RelayNode; }
            if (auto parent = dynamic_cast<RelayNode*>(current->parent)) { rtNode.graphID = rtNode.nodeID; }

            for(auto note : current->nodeData.midiNotes){

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

void NodeCanvas::setValueTreeState(juce::ValueTree stateTree)
{
    clearCanvas();

    for (int i = 0 ; i < stateTree.getNumChildren(); i++) {

        juce::ValueTree treeChild = stateTree.getChild(i);

        int xPos = treeChild.getProperty("x");
        int yPos = treeChild.getProperty("y");
        int radius = treeChild.getProperty("radius");
        int count = treeChild.getProperty("count");
        int countLimit = treeChild.getProperty("countLimit");
        int nodeID = treeChild.getProperty("nodeID");


        if (treeChild.getType() == juce::Identifier("NodeData")) {

            auto canvasNode = new Node();

            canvasNodes.add(canvasNode);
            canvasNode->setCentrePosition(xPos, yPos);

            std::cout<<"X Position: "<<xPos<<std::endl;
            std::cout<<"Y Position: "<<yPos<<std::endl;
            std::cout<<"Radius: "<<radius<<std::endl;

            canvasNode->setSize(radius*2, radius*2);

            canvasNode->nodeData.nodeData.setProperty("x",xPos,nullptr);
            canvasNode->nodeData.nodeData.setProperty("y",yPos,nullptr);
            canvasNode->nodeData.nodeData.setProperty("radius",canvasNode->getWidth(),nullptr);

            canvasNode->nodeData.nodeData.setProperty("count",count,nullptr);
            canvasNode->nodeData.nodeData.setProperty("countLimit",countLimit,nullptr);
            canvasNode->nodeData.nodeData.setProperty("nodeID",nodeID,nullptr);
            addAndMakeVisible(canvasNode);
        }
    }
    canvasTree.removeAllChildren(nullptr);
}

void NodeCanvas::clearCanvas()
{
    if (canvasNodes.isEmpty()) { DBG("CLEAR CANVAS CALLED ON EMPTY CANVAS"); return; }

    nodeArrows.clear();
    canvasNodes.clear();
}