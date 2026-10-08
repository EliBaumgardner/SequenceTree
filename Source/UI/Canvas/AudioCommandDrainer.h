#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class NodeCanvas;
class AudioUIBridge;
class GraphState;

class AudioCommandDrainer
{
public:
    AudioCommandDrainer(NodeCanvas& nodeCanvas, AudioUIBridge& bridge, GraphState& graphState);

    void drainAll();

private:

    void drainHighlights();
    juce::Colour getTraversalColour(int traversalId) const;
    void drainArrows();
    void drainCounts();

    NodeCanvas&    nodeCanvas;
    AudioUIBridge& bridge;
    GraphState&    graphState;
};
