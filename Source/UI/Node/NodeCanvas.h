/*
  ==============================================================================

    NodeCanvas.h
    Created: 6 May 2025 8:34:41pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Node.h"
#include "../Util/RTData.h"
#include "NodeTextEditor.h"
#include "NodeArrow.h"
#include "Connector.h"
#include "../Logic/DynamicPort.h"
#include "../Util/NodeInfo.h"




class NodeCanvas : public juce::Component, public juce::ValueTree::Listener, public juce::AsyncUpdater {
    
    public:

        enum class AsyncUpdateType {NodeAdded,NodeRemoved,NodeMoved};

        struct AsyncUpdate {
            AsyncUpdateType type;
            int nodeId;
            int rootNodeId;
        };

        NodeCanvas();

        void paint(juce::Graphics& g) override;

        void addLinePoints(Node* startNode, Node* endNode);
        void updateLinePoints(Node* movedNode);
        void removeLinePoints(Node* node);
    
        void setSelectionMode(NodeDisplayMode mode) const;

        void addNodeToCanvas(int nodeId);
        void removeNodeFromCanvas(int nodeId);
        void setNodePosition(int nodeId);

        void makeRTGraph(const juce::ValueTree& nodeValueTree);
        void destroyRTGraph(Node* root);
    
        void setProcessorPlayblack(bool isPlaying);

        void setValueTreeState(const juce::ValueTree& stateTree);

        void clearCanvas();

        void handleAsyncUpdate() override;

        void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
        void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex) override;
        void valueTreePropertyChanged(juce::ValueTree &tree, const juce::Identifier &propertyIdentifier) override;

        juce::OwnedArray<NodeArrow> nodeArrows;


        std::unordered_map<int, Node*> nodeMap;
        std::unordered_map<int,std::unordered_map<int, juce::ValueTree>> nodeMaps;
        std::unordered_map<int, std::shared_ptr<RTGraph>> rtGraphs;
        std::shared_ptr<RTGraph> lastGraph;

        juce::Colour canvasColour = juce::Colours::white;
        juce::String infoText;

        bool start = false;

        juce::ValueTree canvasTree {"CanvasTree"};

        std::vector<AsyncUpdate> asyncUpdates;
};
