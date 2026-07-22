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
#include "DanglingArrowLayer.h"
#include "NodeManager.h"
#include "ArrowManager.h"

class Node;
class RootNode;
class Arrow;

class NodeCanvas : public juce::Component, public juce::AsyncUpdater {

    private:
        ApplicationContext& applicationContext;

    public:

        enum class AsyncUpdateType {NodeAdded,NodeRemoved,NodeMoved,DurationOnly,ValueChanged,DanglingArrowsChanged,ArrowAdded,ArrowRemoved};

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

        void setValueTreeState(const juce::ValueTree& stateTree);

        void clearCanvas();

        void handleAsyncUpdate() override;

        void setPaintMode(bool enabled);

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

        NodeManager         nodeManager        { *this, applicationContext };
        ArrowManager        arrowManager       { *this, applicationContext };
        AudioCommandDrainer drainer            { *this, applicationContext };
        DanglingArrowLayer  danglingArrowLayer { *this, applicationContext };

        ApplicationContext& getApplicationContext() { return applicationContext; }
};
