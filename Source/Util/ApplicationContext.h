#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class SequenceTreeAudioProcessor;
class NodeCanvas;
class CustomLookAndFeel;
class NodeController;
class GraphState;
class TraversalRuleState;
class RTGraphBuilder;

struct ApplicationContext
{
    SequenceTreeAudioProcessor* processor          = nullptr;
    NodeCanvas*                 canvas             = nullptr;
    CustomLookAndFeel*          lookAndFeel        = nullptr;
    juce::UndoManager*          undoManager        = nullptr;
    GraphState*                 graphState         = nullptr;
    TraversalRuleState*         traversalRuleState = nullptr;
    NodeController*             nodeController     = nullptr;
    RTGraphBuilder*             rtGraphBuilder     = nullptr;
};
