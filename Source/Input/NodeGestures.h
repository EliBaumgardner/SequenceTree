#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include "../Util/NodeInfo.h"
#include "../Graph/NodeFactory.h"

class Node;
class NodeCanvas;
class GraphState;

class NodeGestures
{
public:

    NodeGestures(NodeCanvas& nodeCanvas, GraphState& graphState, juce::UndoManager& undoManager);

    void createRootNode(juce::Point<float> clickPoint);

    void respondToDrag(Node& node, juce::Point<int> cursor, const juce::ModifierKeys& mods);
    void finishDrag   ();

    void dragArrowHead      (juce::Point<int> cursor);
    void finishArrowHeadDrag();

    NodeCreationMode creationMode = NodeCreationMode::Node;

    juce::Component::SafePointer<Node> arrowHeadNode;

private:

    void handleNodeDrag     (int nodeId, NodePosition newPosition);
    void handleNodeDragStart(Node* node, int nodeId, NodePosition newPosition, const juce::ModifierKeys& mods);
    void snapToGrid         (NodePosition& newPosition, juce::ValueTree draggedNodeTree);

    void checkRootNodeSnap       (juce::Point<int> canvasPoint);
    void connectDraggedNodeToRoot();
    void setDraggedNodeVisible   (bool shouldBeVisible);

    static constexpr float rootSnapThreshold = 20.0f;

    NodeCanvas&        nodeCanvas;
    GraphState&        graphState;
    juce::UndoManager& undoManager;

    juce::Component::SafePointer<Node> snapTargetRoot;

    juce::ValueTree draggedNodeTree;

    int snapSourceNodeId = -1;

    bool isDragStart = true;
};
