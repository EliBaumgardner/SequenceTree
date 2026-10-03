#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>

#include "../../Util/ArrowInfo.h"

class NodeCanvas;
class Node;
class Arrow;
struct ApplicationContext;

class ArrowManager
{
public:

    ArrowManager(NodeCanvas& canvas, const ApplicationContext& context);
    ~ArrowManager();

    const juce::OwnedArray<Arrow>& all() const
    {
        return arrows;
    }

    Arrow* find(int parentNodeId, int childNodeId) const;

    Arrow* connect(Node* startNode, Node* endNode);
    Arrow* connectParentToChild(Node* parentNode, Node* childNode);
    void   adopt(std::unique_ptr<Arrow> arrow);

    void remove(Arrow* arrow);
    void removeForNode(const Node* node);
    void removeMatching(const std::function<bool(Arrow*)>& predicate);
    void clear();

    void updatePreview(Node* node, juce::Point<int> tipOffset, bool dashed = false);
    void commitPreview();

    void rebuildDanglingForNode(int nodeId);

    void refreshFor(const Node* movedNode);

    void refreshEncapsulatedArrows();

    void handleArrowAdded(int parentNodeId, int childNodeId);
    void handleArrowInfoChanged(int parentNodeId, int childNodeId);

    void setSelected(Arrow* arrow);
    void clearSelection();

    void resetAllProgress();
    void pauseAllProgress();
    void resumeAllProgress();
    void resetTrail(int trailId);

    void triggerSnapForNode(int nodeId);

    void showSnapGhost(Node* from, Node* to);
    void hideSnapGhost();

    ArrowInfo              currentArrowInfo;
    std::unique_ptr<Arrow> preview;

private:

    static int arrowKey(const Arrow& arrow);

    juce::ValueTree connectionTreeFor(int startNodeId, int endNodeId) const;

    void detach(Arrow* arrow);

    NodeCanvas&               canvas;
    const ApplicationContext& applicationContext;

    juce::OwnedArray<Arrow> arrows;
    std::unique_ptr<Arrow>  snapGhostArrow;
};
