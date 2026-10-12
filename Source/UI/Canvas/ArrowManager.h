#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>

#include "../../Util/ArrowInfo.h"

class NodeCanvas;
class Node;
class Arrow;
class GraphState;

class ArrowManager
{
public:

    ArrowManager(NodeCanvas& nodeCanvas, GraphState& graphState, juce::UndoManager& undoManager);
    ~ArrowManager();

    const juce::OwnedArray<Arrow>& all() const
    {
        return arrows;
    }

    Arrow* find                (int parentNodeId, int childNodeId) const;
    Arrow* connect             (Node* startNode, Node* endNode);
    Arrow* connectParentToChild(Node* parentNode, Node* childNode);

    void adopt(std::unique_ptr<Arrow> arrow);

    void refreshFor               (const Node* movedNode);
    void endNodeDrag              ();
    void rebuildDanglingForNode   (int nodeId);
    void refreshEncapsulatedArrows();

    void remove        (Arrow* arrow);
    void removeForNode (const Node* node);
    void removeMatching(const std::function<bool(Arrow*)>& predicate);
    void clear         ();

    void handleArrowAdded      (int parentNodeId, int childNodeId);
    void handleArrowInfoChanged(int parentNodeId, int childNodeId);

    void updatePreview(Node* node, juce::Point<int> tipOffset);
    void commitPreview();

    void showSnapGhost     (Node* from, Node* to);
    void hideSnapGhost     ();
    void triggerSnapForNode(int nodeId);

    void setSelected   (Arrow* arrow);
    void clearSelection();

    void resetAllProgress ();
    void pauseAllProgress ();
    void resumeAllProgress();
    void resetTrail       (int trailId);

    ArrowInfo              currentArrowInfo;
    std::unique_ptr<Arrow> preview;
    bool                   nodesDragging = false;

private:

    void detach(Arrow* arrow);

    juce::ValueTree connectionTreeFor(int startNodeId, int endNodeId) const;

    static int arrowKey(const Arrow& arrow);

    NodeCanvas&        nodeCanvas;
    GraphState&        graphState;
    juce::UndoManager& undoManager;

    juce::OwnedArray<Arrow> arrows;
    std::unique_ptr<Arrow>  snapGhostArrow;
};
