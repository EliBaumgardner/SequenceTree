//
// Created by Eli Baumgardner on 7/21/26.
//

#ifndef SEQUENCETREE_ARROWMANAGER_H
#define SEQUENCETREE_ARROWMANAGER_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>

#include "../../Util/ArrowInfo.h"

class NodeCanvas;
class Node;
class Arrow;
struct ApplicationContext;

class ArrowManager {

public:

    ArrowManager(NodeCanvas& canvas, const ApplicationContext& context);
    ~ArrowManager();

    const juce::OwnedArray<Arrow>& all() const { return arrows; }

    Arrow* find(int parentNodeId, int childNodeId) const;

    Arrow* connect(Node* startNode, Node* endNode);
    Arrow* connectParentToChild(Node* parentNode, Node* childNode);
    void   adopt(Arrow* arrow);
    void   attach(Arrow& arrow);

    void remove(Arrow* arrow);
    void removeForNode(const Node* node);
    void removeMatching(const std::function<bool(Arrow*)>& predicate);
    void clear();

    void  updatePreview(Node* node, juce::Point<int> tipOffset, bool dashed = false);
    void  commitPreview();
    void  cancelPreview();
    bool  hasPreview() const { return preview != nullptr; }
    Node* previewStartNode() const;

    void rebuildDanglingForNode(int nodeId);

    void refreshFor(const Node* movedNode);

    void refreshEncapsulatedArrows();

    void handleArrowAdded      (int parentNodeId, int childNodeId);
    void handleArrowRemoved    (int parentNodeId, int childNodeId);
    void handleArrowInfoChanged(int parentNodeId, int childNodeId);

    void setSelected(Arrow* arrow);
    void clearSelection();

    void resetAllProgress();
    void pauseAllProgress();
    void resumeAllProgress();
    void resetTrail(int trailId);

    void triggerSnapForNode(int nodeId);

    void   showSnapGhost(Node* from, Node* to);
    void   hideSnapGhost();

    ArrowInfo currentArrowInfo;

private:

    static int arrowKey(const Arrow& arrow);

    juce::ValueTree connectionTreeFor(int startNodeId, int endNodeId) const;

    void detach(Arrow* arrow);

    NodeCanvas& canvas;
    const ApplicationContext& applicationContext;

    juce::OwnedArray<Arrow> arrows;
    std::unique_ptr<Arrow>  preview;
    std::unique_ptr<Arrow>  snapGhostArrow;
};

#endif //SEQUENCETREE_ARROWMANAGER_H
