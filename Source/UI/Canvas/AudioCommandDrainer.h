#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/*
    The reader on the message thread for the three queues AudioUIBridge fills: highlights, arrows
    and counts. drainAll empties each queue and applies every command to the canvas: a highlight
    command lights or clears a node in the colour of its traversal (or clears every node), an
    arrow command starts the progress trail on the arrows between two nodes or resets the trails,
    and a count command sets the visit count a node shows.

    If a queue overflowed, the commands that did arrive no longer describe the state, so drainAll
    throws them away and resets the highlights and the arrows' progress instead (a dropped count
    command only discards the counts). The constructor does the same for the commands that piled
    up before the canvas existed. It runs only on the message thread and never writes to the
    queues. NodeCanvas::handleAsyncUpdate calls drainAll.
*/

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
