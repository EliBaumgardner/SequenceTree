/*
  ==============================================================================

    NodeCanvas.h
    Created: 6 May 2025 8:34:41pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once

#include <unordered_set>

#include "../../Graph/RTData.h"
#include "DynamicPort.h"
#include "../../Util/NodeInfo.h"
#include "../../Util/ApplicationContext.h"
#include "NodeCanvasTreeListener.h"
#include "ValueField.h"
#include "AudioCommandDrainer.h"
#include "NodeManager.h"
#include "ArrowManager.h"
#include "CanvasHitTester.h"
#include "../../Util/ArrowInfo.h"

class Node;
class RootNode;
class Arrow;

class NodeCanvas : public juce::Component, public juce::AsyncUpdater {

    private:
        ApplicationContext& applicationContext;

    public:

        enum class AsyncUpdateType {NodeAdded,NodeRemoved,NodeMoved,DurationOnly,ValueChanged,DanglingArrowsChanged,ArrowAdded,ArrowRemoved,ArrowTypeChanged};

        struct AsyncUpdate {
            AsyncUpdateType type;
            int nodeId;
            int rootNodeId;
        };

        NodeCanvas(ApplicationContext& context);
        ~NodeCanvas();

        void enqueueAsyncUpdate(const AsyncUpdate& update);
        void paint(juce::Graphics& g) override;
        void setProcessorPlayblack(bool isPlaying);
        void rebuildFromNodeMap(const juce::ValueTree& stateTree);
        void clearCanvas();
        void handleAsyncUpdate() override;

        void setPaintMode(bool enabled);

        void showGrid();
        void hideGrid();
        juce::Point<int> snapPointToGrid(juce::Point<int> point) const;

        void cancelPendingUpdatesFor(int nodeId);

        juce::Colour canvasColour = juce::Colours::white;
        juce::String infoText;

        bool start     = false;
        bool paintMode = false;

        bool gridVisible = false;
        bool gridOriginSet = false;
        juce::Point<float> gridOrigin { 0.0f, 0.0f };
        float gridSpacing = ArrowInfo::pixelsPerGridSpace;

        juce::Rectangle<int> selectionBounds;

        juce::ValueTree canvasTree {"CanvasTree"};

        std::vector<AsyncUpdate> asyncUpdates;

        NodeCanvasTreeListener treeListener { *this };

        ValueField valueField { *this };

        NodeManager         nodeManager        { *this, applicationContext };
        ArrowManager        arrowManager       { *this, applicationContext };
        AudioCommandDrainer drainer            { *this, applicationContext };
        CanvasHitTester     hitTester          { *this };

        ApplicationContext& getApplicationContext() { return applicationContext; }
};
