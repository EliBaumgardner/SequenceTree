#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <map>
#include <memory>

#include "OrnamentMarkView.h"

/*
    The record of the ornament marks on the canvas: for each node that carries an ornament, an
    OrnamentMarkView placed on the top edge of the node, added to the NodeCanvas as a child. A
    node counts as ornamented when its tree has an OrnamentUid. Its mark shows the ornament's
    symbol, or a missing marker when the library has no such ornament or it is still a draft
    that has not been published.

    refresh creates, updates or removes the mark for one node. It listens to the ornament
    libraries tree and refreshes every node when an ornament is added, removed or has its symbol
    changed, and it listens to each marked node's component so the mark follows it when the node
    moves, resizes, hides or is deleted.

    NodeCanvas owns it, and calls refresh from its nodeCreated hook for a new node and from its
    updateApplied hook for a NodeMarkChanged update.
*/

class NodeCanvas;
class Node;
class OrnamentLibraries;

class OrnamentMarks : private juce::ValueTree::Listener, private juce::ComponentListener
{
public:

    OrnamentMarks(NodeCanvas& nodeCanvas, OrnamentLibraries& ornamentLibraries);
    ~OrnamentMarks() override;

    void refresh(Node& node);

    static constexpr float markHeightRatio = 0.6f;

private:

    void valueTreeChildAdded     (juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved   (juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;

    void componentMovedOrResized   (juce::Component& component, bool wasMoved, bool wasResized) override;
    void componentVisibilityChanged(juce::Component& component) override;
    void componentBeingDeleted     (juce::Component& component) override;

    void place     (Node& node);
    void refreshAll();

    NodeCanvas&        nodeCanvas;
    OrnamentLibraries& ornamentLibraries;

    std::map<juce::Component*, std::unique_ptr<OrnamentMarkView>> views;
};
