#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Graph/Clipboard.h"

#include <vector>

/*
    The selection features of the canvas: choosing nodes and acting on the chosen ones. Whether a
    node is selected is the isSelected flag on its Node component; this class changes those flags
    (selectAll, clearAll, deselectAllExcept) and draws and applies a selection rectangle: begin,
    update and finishBoxSelection select every visible node whose centre is inside it.

    copySelection, deleteSelection and pasteAt act on the selected nodes. It owns the Clipboard
    and hands it the selected ids; after a paste it selects the ids the clipboard returns. Deleting
    removes every selected node in one undo transaction. showSelectionMenu offers the same three
    actions at a point on the canvas.

    NodeController routes the mouse to it, and PluginEditor's key handler calls it for delete,
    copy, paste and select all.
*/

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
