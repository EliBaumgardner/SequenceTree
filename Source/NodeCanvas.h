/*
  ==============================================================================

    NodeCanvas.h
    Created: 6 May 2025 8:34:41pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include "ProjectModules.h"
#include "NodeMenu.h"
#include "DynamicPort.h"
#include "Node.h"
#include "RTData.h"
#include "DynamicEditor.h"
#include "NodeBox.h"

class NodeCanvas : public juce::Component {
    
    public:
    
        NodeCanvas();
        ~NodeCanvas() override;
    
        void paint(juce::Graphics& g) override;
        void resized() override;
    
        void updateInfoText();
    
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
    
        juce::OwnedArray<Node>& getCanvasNodes();
    
        void addLinePoints(Node* lineStartNode, Node* lineEndNode);
        void removeLinePoints(Node* linePointNode);
    
        void setNodeMenu(NodeMenu* nodeMenu);
        NodeMenu* getNodeMenu();
    
        void setSelectionMode(NodeBox::DisplayMode mode);
        void removeNode(Node* node);
        
        void makeRTGraph(Node* root);
        void destroyRTGraph(Node* root);
    
        void setProcessorPlayblack(bool isPlaying);
    
        enum class ControllerMode { Inspect, Node };
    
        ControllerMode controllerMode;
    
        Node* root = nullptr;

        using nodeMap  = std::unordered_map<int, Node*>;
        std::unordered_map<int,nodeMap> nodeMaps;
    
        bool start = false;
    
        
    private:
    
        float zoomLevel = 1.0f;
        float offsetX = 0.0f; float offsetY = 0.0f;
        bool isPanning = false;
        juce::Point<int> lastMouse;
        int originalHeight; int originalWidth;
    
        juce::Point<int> lastMouseScreen;
        juce::Colour canvasColour = juce::Colours::white;
        juce::String infoText;
        
        NodeMenu* nodeMenu = nullptr;
        juce::OwnedArray<Node> canvasNodes;
        std::unordered_map<int, std::shared_ptr<RTGraph>> rtGraphs;
    
        juce::Array<std::pair<Node*,Node*>> linePoints;
    
        std::shared_ptr<RTGraph> lastGraph;
};
