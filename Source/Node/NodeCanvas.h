/*
  ==============================================================================

    NodeCanvas.h
    Created: 6 May 2025 8:34:41pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once

#include "../Util/ProjectModules.h"
#include "Node.h"
#include "../Util/RTData.h"
#include "NodeBox.h"
#include "NodeArrow.h"
#include "Traverser.h"
#include "../Logic/DynamicPort.h"

class NodeCanvas : public juce::Component {
    
    public:
    
        NodeCanvas();
        ~NodeCanvas() override;
    
        void paint(juce::Graphics& g) override;
        void resized() override;
    
        void updateInfoText();
    
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;

        void addLinePoints(Node* startNode, Node* endNode);
        void updateLinePoints(Node* movedNode);
        void removeLinePoints(Node* node);
    
        void setSelectionMode(NodeBox::DisplayMode mode);
        void removeNode(Node* node);
        
        void makeRTGraph(Node* root);
        void destroyRTGraph(Node* root);
    
        void setProcessorPlayblack(bool isPlaying);
    
        enum class ControllerMode { Inspect, Node, Counter,Traverser };


    // Object Variables //

        ControllerMode controllerMode;
        std::unique_ptr<ObjectController> controller;
    
        Node* root = nullptr;
        juce::OwnedArray<Node> canvasNodes;
        juce::OwnedArray<NodeArrow> nodeArrows;

        using nodeMap  = std::unordered_map<int, Node*>;
        std::unordered_map<int,nodeMap> nodeMaps;
        std::unordered_map<int, std::shared_ptr<RTGraph>> rtGraphs;
        std::shared_ptr<RTGraph> lastGraph;

        juce::Colour canvasColour = juce::Colours::white;
        juce::String infoText;

        bool controllerMade = false;

    // Primative Variables //
    bool start = false;
};
