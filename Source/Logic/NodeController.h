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
#include "../Util/NodePosition.h"


class NodeCanvas;

class Node;

class Counter;

class NodeMenu;

class NodeData;


class NodeFactory;



class NodeController : public juce::MouseListener {
    
public:


    NodeController();

    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void dragNode(Node* node, NodePosition nodePosition);
    void addNode(Node* parentNode,NodePosition nodePosition, juce::UndoManager* undoManager);
    void connectNode(int deltaX, int deltaY, juce::Point<float> position);

    void setObjects(juce::Component& component);
    void handleNodeRelease();

    enum class NodeControllerMode {Node,Connector};
    NodeControllerMode nodeControllerMode;

private:

    NodeCanvas* nodeCanvas = nullptr;

    Node* connectorNode = nullptr;

    bool isDragStart = true;
    bool hasConnection = false;

    int lastX = 0;
    int lastY = 0;
};
