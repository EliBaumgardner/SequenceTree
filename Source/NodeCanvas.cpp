// NodeCanvas.cpp
#include "ComponentContext.h"

#include "NodeCanvas.h"
#include <algorithm>
#include "PluginProcessor.h"


NodeCanvas::NodeCanvas()
{
    updateInfoText();
    
}

NodeCanvas::~NodeCanvas() {
    
}

void NodeCanvas::paint(juce::Graphics& g)
{
    g.fillAll(canvasColour);
    g.setFont(12.0f);
    g.drawText(infoText, getLocalBounds(), juce::Justification::topRight, true);

    for (auto& linePoint : linePoints)
    {
        auto* a = linePoint.first;
        auto* b = linePoint.second;
        
        // Arrow size
        float arrowLength = 10.0f;
        float arrowWidth = 5.0f;
        int radius = b->getBounds().getWidth()/2;
        
        g.setColour(juce::Colours::black);
        int x1 = a->getBounds().getCentreX();
        int y1 = a->getBounds().getCentreY();
        int x2 = b->getBounds().getCentreX();
        int y2 = b->getBounds().getCentreY();
        
        // Calculate direction vector
        float dx = float(x2 - x1);
        float dy = float(y2 - y1);
        float length = std::sqrt(dx*dx + dy*dy);
        if (length == 0) continue; // avoid division by zero
        
        // Normalize direction
        float nx = dx / length;
        float ny = dy / length;
        
        x2 = x2 - nx*radius;
        y2 = y2 - ny*radius;
        
        // Draw main line
        g.drawLine(x1, y1, x2, y2, 2.0f);
        g.setColour(juce::Colours::white);
        g.drawLine(x1, y1, x2, y2, 1.0f);
        
        // Calculate the two points for the arrowhead lines
        float leftX = x2 - arrowLength * nx + arrowWidth * ny;
        float leftY = y2 - arrowLength * ny - arrowWidth * nx;
        
        float rightX = x2 - arrowLength * nx - arrowWidth * ny;
        float rightY = y2 - arrowLength * ny + arrowWidth * nx;
        
        // Draw arrowhead lines (black thick)
        g.setColour(juce::Colours::black);
        g.drawLine(x2, y2, leftX, leftY, 2.0f);
        g.drawLine(x2, y2, rightX, rightY, 2.0f);
        
        // Draw arrowhead lines (white thin) for contrast
        g.setColour(juce::Colours::white);
        g.drawLine(x2, y2, leftX, leftY, 1.0f);
        g.drawLine(x2, y2, rightX, rightY, 1.0f);
    }
}

void NodeCanvas::resized()
{
  
}

void NodeCanvas::updateInfoText()
{
    infoText = canvasNodes.isEmpty() ? "click anywhere to add root" : "";
    repaint();
}

void NodeCanvas::mouseDown(const juce::MouseEvent& e)
{

    if (e.mods.isLeftButtonDown() && controllerMode == ControllerMode::Node)
    {
        
            auto* node = new Node(this);
            
            canvasNodes.add(node);
            auto pos = e.getPosition().toFloat();
            node->setBounds(int(pos.x) - 20, int(pos.y) - 20, 40, 40);
            addAndMakeVisible(node);
            updateInfoText();
            
            if(root == nullptr){
                root = node;
            }
        
            makeRTGraph(root);
    }
}

void NodeCanvas::mouseDrag(const juce::MouseEvent& e)
{
 
    if (auto* parent = dynamic_cast<DynamicPort*>(getParentComponent()))
        {
            auto parentEvent = e.getEventRelativeTo(parent);
            parent->mouseDrag(parentEvent);
        }
}

void NodeCanvas::mouseUp(const juce::MouseEvent& e)
{
    isPanning = false;
}

juce::OwnedArray<Node>& NodeCanvas::getCanvasNodes() { return canvasNodes; }

void NodeCanvas::addLinePoints(Node* start, Node* end) { linePoints.add({ start, end }); }

void NodeCanvas::removeLinePoints(Node* target)
{
    for (int i = linePoints.size(); --i >= 0; )
        if (linePoints.getReference(i).first == target ||
            linePoints.getReference(i).second == target)
            linePoints.remove(i);
    repaint();
}

NodeMenu* NodeCanvas::getNodeMenu() { return nodeMenu; }
void NodeCanvas::setNodeMenu(NodeMenu* nm) { nodeMenu = nm; }

void NodeCanvas::removeNode(Node* node) {
    std::cout<<"removed Node"<<std::endl;

    if(!node->getNodeData()->children.isEmpty()){
        for (Node* child : node->getNodeData()->children){
            child->parent = nullptr;
        }
    }
    
    root->nodeLogic.hardStart = false;

    if (node == root->nodeLogic.target)
        root->nodeLogic.target = nullptr;

    if (node == root->nodeLogic.referenceNode)
        root->nodeLogic.referenceNode = nullptr;
    
    if (node->parent != nullptr) {
        node->parent->getNodeData()->removeChild(node);
    }

    node->removeMouseListener(node->nodeController.get());
    removeLinePoints(node);
    canvasNodes.removeObject(node);
  
    repaint();
    
    updateProcessorGraph(root);
}

std::shared_ptr<RTGraph> NodeCanvas::makeRTGraph(Node* root){
    
    std::cout<<"graph made"<<std::endl;
    nodeMap.clear();

    auto rtGraph = std::make_shared<RTGraph>();
    std::vector<Node*> stack = {root};
    
    while(!stack.empty()){
        
        Node* current = stack.back();
        stack.pop_back();
        
        
        if(nodeMap.count(current) == false){
            int id = current->nodeID;
            std::cout<<"nodeID: "<<id<<std::endl;
            nodeMap[current] = id;
            
            RTNode rtNode;
            rtNode.nodeID = id;
            rtNode.countLimit = static_cast<int>(current->nodeData.nodeData.getProperty("countLimit"));
            std::cout<<"countLimit: "<<rtNode.countLimit<<std::endl;
            
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
            //rtGraph->nodes.push_back(std::move(rtNode));
            for(auto child : current->nodeData.children){
                stack.push_back(child);
            }
        }
    }
    
    for(auto& [node, id] : nodeMap){
        
        for(auto child : node->nodeData.children){
            rtGraph->nodeMap[id].children.push_back(nodeMap[child]);
        }
    }
    
    rtGraph->traversalRequested = start;
    return rtGraph;
}

void NodeCanvas::updateProcessorGraph(Node* node){
    
    ComponentContext::processor->setNewGraph(makeRTGraph(node));
    //context->processor.setNewGraph(makeRTGraph(node));
}

void NodeCanvas::setSelectionMode(NodeBox::DisplayMode mode) {
    
    for(int i = 0; i < canvasNodes.size(); i++){
        canvasNodes[i]->setDisplayMode(mode);
        canvasNodes[i]->editor.get()->formatDisplay(mode);
    }
}
