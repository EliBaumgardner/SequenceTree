/*
  ==============================================================================

    ObjectObjectController
    Created: 6 May 2025 8:38:35pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once
#include "../Util/PluginModules.h"
#include "../Util/PluginContext.h"
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

    NodeController();

    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;

    void snapToGrid(juce::UndoManager *undoManager, NodePosition &newPosition, juce::ValueTree draggedNodeTree);

    void mouseUp   (const juce::MouseEvent& e) override;

    void connectNode(int deltaX, int deltaY, juce::Point<float> position);

private:

    NodeCanvas* nodeCanvas = nullptr;

    Node* connectorNode = nullptr;

    bool isDragStart = true;
    bool hasConnection = false;
    bool isDraggingValue = false;

    double dragStartValue = 0.0;
    Node* draggingValueNode = nullptr;

    juce::Point<float> dragParentCenter;

    juce::ValueTree draggedNodeTree;

    int lastX = 0;
    int lastY = 0;
};
