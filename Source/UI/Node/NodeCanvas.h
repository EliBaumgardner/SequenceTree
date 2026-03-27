/*
  ==============================================================================

    NodeCanvas.h
    Created: 6 May 2025 8:34:41pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once

#include "../Util/PluginModules.h"
#include "Node.h"
#include "../Util/RTData.h"
#include "NodeBox.h"
#include "NodeArrow.h"
#include "RelayNode.h"
#include "../Logic/DynamicPort.h"



class NodeCanvas : public juce::Component, public juce::ValueTree::Listener {
    
    public:
    
        NodeCanvas();
    
        void paint(juce::Graphics& g) override;
    
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;

        void addLinePoints(Node* startNode, Node* endNode);
        void updateLinePoints(Node* movedNode);
        void removeLinePoints(Node* node);
    
        void setSelectionMode(NodeBox::DisplayMode mode) const;

        void removeNode(Node* node);
        void addRootNode(int nodeId);

        void makeRTGraph(Node* root);
        void destroyRTGraph(Node* root);
    
        void setProcessorPlayblack(bool isPlaying);

        void setValueTreeState(const juce::ValueTree& stateTree);

        void clearCanvas();

        void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
        void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex) override;
        void valueTreePropertyChanged(juce::ValueTree &tree, const juce::Identifier &property) override;

        juce::OwnedArray<Node> canvasNodes;
        juce::OwnedArray<NodeArrow> nodeArrows;

        using nodeMap  = std::unordered_map<int, Node*>;
        std::unordered_map<int,nodeMap> nodeMaps;
        std::unordered_map<int, std::shared_ptr<RTGraph>> rtGraphs;
        std::shared_ptr<RTGraph> lastGraph;

        juce::Colour canvasColour = juce::Colours::white;
        juce::String infoText;

        bool start = false;

        juce::ValueTree canvasTree {"CanvasTree"};
};
