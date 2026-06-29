/*
  ==============================================================================

    NodeCanvas.h
    Created: 6 May 2025 8:34:41pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once

#include <array>

#include "../../Graph/RTData.h"
#include "DynamicPort.h"
#include "../../Util/NodeInfo.h"
#include "../../Util/ApplicationContext.h"
#include "NodeCanvasTreeListener.h"

class Node;
class RootNode;
class NodeArrow;

class NodeCanvas : public juce::Component, public juce::AsyncUpdater, public juce::Timer {

    public:

        enum class AsyncUpdateType {NodeAdded,NodeRemoved,NodeMoved,DurationOnly,ValueChanged};

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

        void setPaintMode(bool paintMode);
        void setBrushColour(juce::Colour colour);
        void setBrushRadius(float radius);
        void updateBrushCursor();
        void setActivePaintLayer(int index);
        void renderValueField();
        void refreshValueField();
        void paintStroke(juce::Point<float> canvasPos, bool isStart, bool erase = false);
        void endStroke();
        void ensurePaintBuffers();
        void seedStrokeDensityFromNodes();
        void accumulateStroke(juce::Point<float> from, juce::Point<float> to, bool rearm = false);
        void applyPaintToNodes(juce::Point<float> from, juce::Point<float> to);
        juce::Identifier paintLayerValueId() const;
        void timerCallback() override;

        juce::OwnedArray<NodeArrow> nodeArrows;
        NodeArrow* snapGhostArrow = nullptr;

        std::unordered_map<int, Node*> nodeMap;

        juce::Colour canvasColour = juce::Colours::white;
        juce::String infoText;

        bool start     = false;
        bool paintMode = false;

        static constexpr int numPaintLayers = 3;

        int activePaintLayer = 0;
        juce::Image  valueField;

        std::vector<float> fieldWeightedSum;
        std::vector<float> fieldTotalWeight;
        std::vector<float> fieldCoverageProd;

        juce::Colour brushColour = juce::Colours::white;
        float        brushRadius = 12.0f;
        float        viewZoom    = 1.0f;

        void setViewZoom(float z);

        std::array<std::vector<float>, numPaintLayers> paintDensity;
        std::vector<float>   strokeMask;
        juce::Point<float>   strokePrevPoint;
        juce::Point<float>   brushCurrentPoint;
        float  brushFlow         = 0.22f;
        bool   brushStrokeActive = false;
        bool   brushErase        = false;

        static constexpr int   dwellTimerHz = 30;
        static constexpr float dwellRearm   = 0.15f;

        bool showGrid = false;
        bool gridOriginSet = false;
        juce::Point<float> gridOrigin { 0.0f, 0.0f };
        float gridSpacing = 50.0f;

        juce::ValueTree canvasTree {"CanvasTree"};

        std::vector<AsyncUpdate> asyncUpdates;

        NodeCanvasTreeListener treeListener { *this };

    private:
        juce::Colour mapFieldColour(float factor) const;

        ApplicationContext& applicationContext;
};
