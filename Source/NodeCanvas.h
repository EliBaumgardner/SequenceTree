/*
  ==============================================================================

    NodeCanvas.h
    Created: 6 May 2025 8:34:41pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "NodeMenu.h"
#include "DynamicPort.h"
#include "Node.h"
#include "RTData.h"

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
    
        void removeNode(Node* node);
        
        std::shared_ptr<RTGraph> makeRTGraph(Node* root);
    
        void updateProcessorGraph(Node* node);
    
        Node* root = nullptr;

        std::unordered_map<Node*, int> nodeMap;
    
        bool start = false;
        
    private:
    
        juce::Point<int> lastMouseScreen;
    
        float zoomLevel = 1.0f;
        
        float offsetX = 0.0f; float offsetY = 0.0f;
        bool isPanning = false;
        juce::Point<int> lastMouse;
    
        int originalHeight; int originalWidth;
    
        NodeMenu* nodeMenu = nullptr;
    
        juce::Colour canvasColour = juce::Colours::white;
        
        juce::OwnedArray<Node> canvasNodes;

        juce::String infoText;
    
        juce::Array<std::pair<Node*,Node*>> linePoints;
};
