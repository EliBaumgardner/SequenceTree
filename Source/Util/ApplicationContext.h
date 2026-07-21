/*
  ==============================================================================

    ApplicationContext.h
    Created: 24 Apr 2026
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include "NodeInfo.h"

class SequenceTreeAudioProcessor;
class NodeCanvas;
class CustomLookAndFeel;
class NodeController;
class NodeFactory;
class ValueTreeState;
class RTGraphBuilder;
class Node;

struct ApplicationContext
{
    SequenceTreeAudioProcessor* processor      = nullptr;
    NodeCanvas*                 canvas         = nullptr;
    CustomLookAndFeel*          lookAndFeel    = nullptr;
    juce::UndoManager*          undoManager    = nullptr;
    ValueTreeState*             valueTreeState = nullptr;
    NodeController*             nodeController = nullptr;
    RTGraphBuilder*             rtGraphBuilder = nullptr;

    std::function<void(NodeDisplayMode)> onDisplayModeChanged;

    NodeDisplayMode currentDisplayMode = NodeDisplayMode::Pitch;

    void addNodeSelectedListener(std::function<void(Node*, bool)> listener)
    {
        onNodeSelectedListeners.push_back(std::move(listener));
    }

    void notifyNodeSelected(Node* node, bool selected)
    {
        for (auto& listener : onNodeSelectedListeners)
            listener(node, selected);
    }

private:

    std::vector<std::function<void(Node*, bool)>> onNodeSelectedListeners;
};
