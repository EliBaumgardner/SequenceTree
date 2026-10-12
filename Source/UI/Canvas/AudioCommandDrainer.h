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
    void drainArrows    ();
    void drainCounts    ();

    juce::Colour getTraversalColour(int traversalId) const;

    NodeCanvas&    nodeCanvas;
    AudioUIBridge& bridge;
    GraphState&    graphState;
};
