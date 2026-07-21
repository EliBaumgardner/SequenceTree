/*
  ==============================================================================

    ObjectObjectController
    Created: 6 May 2025 8:38:35pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once
#include "../Util/PluginModules.h"
#include "../Util/ApplicationContext.h"
#include "../Util/NodeInfo.h"


class NodeCanvas;

class Node;

class Modulator;

class NodeMenu;

class NodeData;

class NodeFactory;

class DanglingArrow;



class NodeController : public juce::MouseListener {

public:

    enum class NodeControllerMode {Node, Modulator, TraversalFlag};
    NodeControllerMode nodeControllerMode;

    NodeController(ApplicationContext& context);

    void mouseEnter          (const juce::MouseEvent& e) override;
    void mouseExit           (const juce::MouseEvent& e) override;
    void mouseMove           (const juce::MouseEvent& e) override;
    void mouseDrag           (const juce::MouseEvent& e) override;
    void mouseUp             (const juce::MouseEvent& e) override;
    void mouseDown           (const juce::MouseEvent& e) override;

    void snapToGrid          (juce::UndoManager *undoManager, NodePosition &newPosition, juce::ValueTree draggedNodeTree);
    juce::Point<int> snapPointToGrid (juce::Point<int> point) const;

    void handleNodeDrag      (juce::UndoManager *undoManager, int nodeId, NodePosition newPosition);
    void handleNodeDragStart (juce::UndoManager *undoManager, Node *node, int nodeId, NodePosition newPosition, const juce::ModifierKeys& mods);

    void updateConnectionPreview (Node *node, const NodePosition& newPosition, bool dashed);

    void checkRootNodeSnap   (const NodePosition& pos);

    Node* findConnectionTarget (juce::Point<int> point, int excludeNodeId) const;
    void  commitFlagConnection (int sourceNodeId, Node* target);

    NodeArrow* findArrowNear   (juce::Point<float> point, float radius) const;
    NodeArrow* findArrowHeadNear (juce::Point<float> point, float radius) const;
    void       deleteArrow     (NodeArrow* arrow);

    DanglingArrow* findDanglingArrowNear (juce::Point<float> point, float radius) const;

private:

    static constexpr float rootSnapThreshold      = 60.0f;
    static constexpr float danglingArrowGrabRadius = 14.0f;
    static constexpr float flagArrowVicinity       = 28.0f;
    static constexpr float arrowHoverRadius        = 8.0f;
    static constexpr float arrowHeadGrabRadius     = 16.0f;

    ApplicationContext& applicationContext;

    DanglingArrow* draggingDanglingArrow = nullptr;

    Node* snapTargetRoot           = nullptr;

    Node* draggingArrowHeadNode    = nullptr;
    bool pressedOnArrow            = false;

    bool creatingDanglingArrow     = false;
    bool draggingFlagConnection    = false;
    int  flagConnectionSourceId    = -1;
    Node* flagConnectionTarget     = nullptr;
    bool isDragStart               = true;
    bool hasConnection             = false;
    bool isDraggingValue           = false;

    double dragStartValue          = 0.0;
    Node*  draggingValueNode       = nullptr;

    juce::Point<float> dragParentCenter;

    juce::ValueTree draggedNodeTree;

    int snapSourceNodeId = -1;
    int lastX            =  0;
    int lastY            =  0;
};
