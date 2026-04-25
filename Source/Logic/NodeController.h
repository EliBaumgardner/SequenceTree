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



class NodeController : public juce::MouseListener {

public:

    enum class NodeControllerMode {Node,Connector,Modulator};
    NodeControllerMode nodeControllerMode;

    NodeController(ApplicationContext& context);

    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;

    void snapToGrid(juce::UndoManager *undoManager, NodePosition &newPosition, juce::ValueTree draggedNodeTree);

    void mouseUp   (const juce::MouseEvent& e) override;

    void checkRootNodeSnap(const NodePosition& pos);

private:

    static constexpr float rootSnapThreshold = 60.0f;

    ApplicationContext& applicationContext;

    Node* connectorNode  = nullptr;
    Node* snapTargetRoot = nullptr;

    bool isDragStart     = true;
    bool hasConnection   = false;
    bool isDraggingValue = false;

    double dragStartValue    = 0.0;
    Node*  draggingValueNode = nullptr;

    juce::Point<float> dragParentCenter;

    juce::ValueTree draggedNodeTree;

    int snapSourceNodeId = -1;
    int lastX = 0;
    int lastY = 0;
};
