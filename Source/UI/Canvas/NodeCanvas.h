/*
  ==============================================================================

    NodeCanvas.h
    Created: 6 May 2025 8:34:41pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once



#include "../../Graph/RTData.h"
#include "DynamicPort.h"
#include "../../Util/NodeInfo.h"
#include "../../Util/ApplicationContext.h"
#include "NodeCanvasTreeListener.h"

class Node;
class RootNode;
class NodeArrow;

class NodeCanvas : public juce::Component, public juce::AsyncUpdater {

    public:

        enum class AsyncUpdateType {NodeAdded,NodeRemoved,NodeMoved,DurationOnly};

        struct AsyncUpdate {
            AsyncUpdateType type;
            int nodeId;
            int rootNodeId;
        };

        NodeCanvas(ApplicationContext& context);
        ~NodeCanvas();

        void enqueueAsyncUpdate(const AsyncUpdate& update);

        void paint(juce::Graphics& g) override;

        void addLinePoints(Node* startNode, Node* endNode);
        void updateLinePoints(Node* movedNode);

        void removeLinePoints(Node* node);

        void setSelectionMode(NodeDisplayMode mode) const;

        void addNodeToCanvas(int nodeId);
        void removeNodeFromCanvas(int nodeId);

        Node* instantiateNodeFromTree(const juce::ValueTree& nodeValueTree);

        void moveDescendants(juce::ValueTree nodeValueTree, int deltaX, int deltaY);

        void setNodePosition(int nodeId);

        void setProcessorPlayblack(bool isPlaying);

        void setValueTreeState(const juce::ValueTree& stateTree);

        void clearCanvas();
        void triggerArrowSnapForNode(int nodeId);
        void showSnapGhostArrow(Node* from, Node* to);
        void hideSnapGhostArrow();

        void handleAsyncUpdate() override;

        void drainHighlightFifo();
        void drainProgressFifo();
        void drainCountFifo();
        void drainArrowResetFifo();

        void resetAllArrowProgress();
        void resetGraphArrowProgress(int graphId);

        juce::OwnedArray<NodeArrow> nodeArrows;
        NodeArrow* snapGhostArrow = nullptr;

        std::unordered_map<int, Node*> nodeMap;

        juce::Colour canvasColour = juce::Colours::white;
        juce::String infoText;

        bool start = false;

        bool showGrid = true;
        bool gridOriginSet = false;
        juce::Point<float> gridOrigin { 0.0f, 0.0f };
        float gridSpacing = 50.0f;

        juce::ValueTree canvasTree {"CanvasTree"};

        std::vector<AsyncUpdate> asyncUpdates;

        NodeCanvasTreeListener treeListener { *this };

    private:
        ApplicationContext& applicationContext;
};
