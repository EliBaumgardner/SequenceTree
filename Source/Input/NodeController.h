/*
  ==============================================================================

    ObjectObjectController
    Created: 6 May 2025 8:38:35pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once
#include "../Util/PluginModules.h"
#include "../Util/ApplicationContext.h"
#include "../Util/NodeInfo.h"
#include "../UI/PopupWindow.h"


class NodeCanvas;

class Node;

class Modulator;

class NodeMenu;

class NodeData;

class NodeFactory;

class Arrow;



class NodeController : public juce::MouseListener {

public:

    enum class NodeControllerMode {Node, Modulator, TraversalFlag};
    NodeControllerMode nodeControllerMode;

    NodeController(ApplicationContext& context);
    ~NodeController() override;

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

    Arrow* findArrowNear     (juce::Point<float> point, float radius) const;
    Arrow* findArrowHeadNear (juce::Point<float> point, float radius) const;
    void   deleteArrow       (Arrow* arrow);
    void   showArrowContextMenu (Arrow* arrow);
    juce::ValueTree getArrowConnectionTree (Arrow* arrow) const;

private:

    enum class DragState {
        Idle,
        EditingValue,
        MovingArrowHead,
        MovingDanglingTip,
        ArrowSelected,
        CreatingDanglingArrow,
        ConnectingFlag
    };

    void handleCanvasMouseDown (const juce::MouseEvent& e, NodeCanvas& canvas);
    void handleNodeMouseDown   (const juce::MouseEvent& e, Node& node);

    void handleCanvasMouseDrag (const juce::MouseEvent& e, NodeCanvas& canvas);
    void handleNodeMouseDrag   (const juce::MouseEvent& e, Node& node);

    void dragValue         (const juce::MouseEvent& e);
    void dragDanglingTip   (const juce::MouseEvent& e, NodeCanvas& canvas);
    void dragFlagConnection(const juce::MouseEvent& e, Node& node, const NodePosition& newPosition);

    void finishArrowHeadDrag         (NodeCanvas& canvas);
    void finishDanglingTipDrag       (NodeCanvas& canvas);
    void finishFlagConnection        (NodeCanvas& canvas);
    void finishDanglingArrowCreation (NodeCanvas& canvas);
    void connectDraggedNodeToRoot    (NodeCanvas& canvas);

    void endDrag  (NodeCanvas& canvas);
    void hideGrid (NodeCanvas& canvas) const;
    void showGrid (NodeCanvas& canvas) const;

    static constexpr float rootSnapThreshold      = 60.0f;
    static constexpr float danglingArrowGrabRadius = 14.0f;
    static constexpr float flagArrowVicinity       = 28.0f;
    static constexpr float arrowHoverRadius        = 8.0f;
    static constexpr float arrowHeadGrabRadius     = 16.0f;
    static constexpr int   dragThreshold           = 5;
    static constexpr int   defaultNodeRadius       = 20;

    ApplicationContext& applicationContext;

    DragState dragState = DragState::Idle;

    Arrow* draggingDanglingArrow = nullptr;

    PopupWindowLauncher allowedTraversalsLauncher { "Allowed Traversals" };

    Node* snapTargetRoot           = nullptr;

    Node* draggingArrowHeadNode    = nullptr;

    int   flagConnectionSourceId   = -1;
    Node* flagConnectionTarget     = nullptr;

    bool isDragStart               = true;

    double dragStartValue          = 0.0;
    Node*  draggingValueNode       = nullptr;

    juce::Point<float> dragParentCenter;

    juce::ValueTree draggedNodeTree;

    int snapSourceNodeId = -1;
};
