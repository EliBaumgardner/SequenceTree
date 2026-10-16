#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Graph/Clipboard.h"

#include <vector>

class Node;
class NodeCanvas;
class GraphState;

class SelectionGestures
{
public:

    SelectionGestures(GraphState& graphState, juce::UndoManager& undoManager, NodeCanvas& nodeCanvas)
        : graphState(graphState), undoManager(undoManager), nodeCanvas(nodeCanvas), clipboard(graphState, undoManager)
    {}

    void copySelection  ();
    void deleteSelection();
    void pasteAt        (juce::Point<int> canvasPoint);

    void beginBoxSelection (const juce::Point<int>& clickPoint);
    void updateBoxSelection(const juce::MouseEvent& e);
    void finishBoxSelection();
    void showSelectionMenu (juce::Point<int> canvasPoint);

    void selectAll        () const;
    void clearAll         () const;
    void deselectAllExcept(const Node& keptNode) const;

private:

    std::vector<int> selectedNodeIds() const;

    GraphState&        graphState;
    juce::UndoManager& undoManager;
    NodeCanvas&        nodeCanvas;

    Clipboard clipboard;

    juce::Point<int> selectionAnchor;
};
