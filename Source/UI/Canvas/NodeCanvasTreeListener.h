#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/*
    The listener that turns each change to a canvas's document tree into an entry in EditCanvas's queue of
    AsyncUpdates, which says what the canvas must redraw and for which node. A node added to or
    removed from the node map becomes NodeAdded or NodeRemoved; an id added to or removed from a
    node's list of children becomes ArrowAdded or ArrowRemoved; a changed position or radius
    becomes NodeMoved; a changed pitch or velocity becomes ValueChanged; a changed arrow duration
    becomes ArrowDurationChanged; a changed arrow type, sync, binding or multiplier becomes
    ArrowInfoChanged; and so on for colour, ornament and dangling arrows.

    It only appends to the queue and touches no component, so the changes are applied later in
    one place, EditCanvas::handleAsyncUpdate, whatever caused them. That includes undo, redo and
    paste. The canvas's owner attaches it to the document's tree: PluginEditor to the graph's
    nodeMap while the editor is open. It never rebuilds the audio graph; RTGraphBuilder listens
    on its own.
*/

class EditCanvas;

class NodeCanvasTreeListener : public juce::ValueTree::Listener
{
public:

    explicit NodeCanvasTreeListener(EditCanvas& canvas);

    void valueTreeChildAdded     (juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved   (juce::ValueTree& parent, juce::ValueTree& child, int childIndex) override;
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& propertyIdentifier) override;

private:

    void enqueueDanglingArrowsChanged(const juce::ValueTree& nodeTree) const;

    EditCanvas& canvas;
};
