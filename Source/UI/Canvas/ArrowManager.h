//
// Created by Eli Baumgardner on 7/21/26.
//

#ifndef SEQUENCETREE_ARROWMANAGER_H
#define SEQUENCETREE_ARROWMANAGER_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class NodeCanvas;
class Node;
class Arrow;
struct ApplicationContext;

class ArrowManager {

public:

    ArrowManager(NodeCanvas& canvas, ApplicationContext& context);
    ~ArrowManager();

    const juce::OwnedArray<Arrow>& all() const { return arrows; }

    Arrow* find(int parentNodeId, int childNodeId) const;

    Arrow* connect(Node* startNode, Node* endNode);
    void   adopt(Arrow* arrow);

    void remove(Arrow* arrow);
    void removeForNode(Node* node);
    void removeMatching(const std::function<bool(Arrow*)>& predicate);
    void clear();

    void refreshFor(Node* movedNode);

    void handleArrowAdded  (int parentNodeId, int childNodeId);
    void handleArrowRemoved(int parentNodeId, int childNodeId);

    void setSelected(Arrow* arrow);
    void clearSelection();

    void resetAllProgress();
    void resetGraphProgress(int graphId, int traversalId);

    void triggerSnapForNode(int nodeId);

    void   showSnapGhost(Node* from, Node* to);
    void   hideSnapGhost();
    Arrow* snapGhost() const { return snapGhostArrow; }

private:

    NodeCanvas& canvas;
    ApplicationContext& applicationContext;

    juce::OwnedArray<Arrow> arrows;
    Arrow* snapGhostArrow = nullptr;
};

#endif //SEQUENCETREE_ARROWMANAGER_H
