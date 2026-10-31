#pragma once

#include "CanvasDocument.h"

/*
    The CanvasDocument for an ornament's figure: each edit the figure canvas makes, carried out on
    the OrnamentFigure tree of ornament through OrnamentLibraries, which holds the figure's rules.
    A figure is one chain of nodes, each pointing at the next, and only its last node may grow a
    child (growFrom answers -1 for the others) or carry the one dangling arrow, whose tip sets
    the last note's length. Moving a node carries every node after it and shifts its pitch offset
    by the rows it moved, adding a root is refused once the figure has a node, and no arrow joins
    two nodes the user picks: canConnect always refuses, so connect and disconnect are never
    reached and change nothing.

    OrnamentBuildPanel owns it and sets ornament and nodes when the active ornament changes;
    OrnamentCanvas and OrnamentFigureController hold it by reference.
*/

class OrnamentLibraries;

class FigureDocument : public CanvasDocument
{
public:

    FigureDocument(OrnamentLibraries& ornamentLibraries, juce::UndoManager& undoManager);

    juce::ValueTree findNode    (int nodeId) const override;
    juce::ValueTree addChildNode(int parentNodeId, juce::Point<int> position, const juce::ModifierKeys& mods) override;

    int growFrom(int nodeId) const override;

    void addRootNode(juce::Point<int> position) override;
    void moveNode   (juce::ValueTree nodeTree, juce::Point<int> position) override;
    void removeNode (int nodeId) override;

    void addDanglingArrow   (juce::ValueTree nodeTree, juce::Point<int> tipOffset) override;
    void setDanglingArrowTip(juce::ValueTree arrowTree, juce::Point<int> tipOffset) override;
    void removeDanglingArrow(juce::ValueTree arrowTree) override;

    void connect   (int parentNodeId, int childNodeId, ConnectionGesture gesture) override;
    void disconnect(int startNodeId, int endNodeId) override;

    bool canConnect(const Node& startNode, const Node& endNode) const override;

    juce::ValueTree ornament;

private:

    OrnamentLibraries& ornamentLibraries;
};
