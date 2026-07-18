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
#include "ValueField.h"

class Node;
class RootNode;
class NodeArrow;
class DanglingArrow;

class NodeCanvas : public juce::Component, public juce::AsyncUpdater {

    public:

        enum class AsyncUpdateType {NodeAdded,NodeRemoved,NodeMoved,DurationOnly,ValueChanged,DanglingArrowsChanged};

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
        void removeArrow(NodeArrow* arrow);

        void setArrowMode(bool enabled);
        void updateDanglingPreview(Node* node, juce::Point<int> tipOffset, bool dashed = false);
        void commitDanglingArrow();
        void cancelDanglingPreview();
        bool hasDanglingPreview() const { return danglingPreview != nullptr; }
        void addDanglingArrow(Node* node, juce::Point<int> tipOffset);
        DanglingArrow* hitTestDanglingArrowHead(juce::Point<int> canvasPos, float radius) const;
        void setDanglingArrowTip(DanglingArrow* arrow, juce::Point<int> tipOffset);
        void commitDanglingArrowTip(DanglingArrow* arrow);
        void removeDanglingArrow(DanglingArrow* arrow);
        void rebuildDanglingArrowsForNode(int nodeId);
        void removeDanglingArrowsForNode(Node* node);
        void removeDanglingArrowsForNodeId(int nodeId);

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
        void resetGraphArrowProgress(int graphId, int traversalId);

        void setPaintMode(bool enabled);
        void setBrushColour(juce::Colour colour);
        void setBrushRadius(float radius);
        void setActivePaintLayer(int index);
        void setViewZoom(float z);
        void refreshValueField();
        void paintStroke(juce::Point<float> canvasPos, bool isStart, bool erase = false);
        void endStroke();

        juce::OwnedArray<NodeArrow> nodeArrows;
        NodeArrow* snapGhostArrow = nullptr;

        juce::OwnedArray<DanglingArrow> danglingArrows;
        std::unique_ptr<DanglingArrow> danglingPreview;
        bool arrowMode = false;

        std::unordered_map<int, Node*> nodeMap;

        juce::Colour canvasColour = juce::Colours::white;
        juce::String infoText;

        bool start     = false;
        bool paintMode = false;

        bool showGrid = false;
        bool gridOriginSet = false;
        juce::Point<float> gridOrigin { 0.0f, 0.0f };
        float gridSpacing = 50.0f;

        juce::ValueTree canvasTree {"CanvasTree"};

        std::vector<AsyncUpdate> asyncUpdates;

        NodeCanvasTreeListener treeListener { *this };

        ValueField valueField { *this };

    private:
        ApplicationContext& applicationContext;
};
