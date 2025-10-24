/*
  ==============================================================================

    ObjectObjectController
    Created: 6 May 2025 8:38:35pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once
#include "../Util/ProjectModules.h"
#include "../Util/ComponentContext.h"


class NodeCanvas;

class Node;

class Counter;

class NodeMenu;

class NodeData;

class ObjectController : public juce::MouseListener {
    
    public:
    
        ObjectController(Node* node);

        void mouseEnter(const juce::MouseEvent& e) override;
        void mouseExit(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent& e) override;

        void setObjects(Node* node);
    
    private:
    
        NodeCanvas* nodeCanvas = nullptr;
        Node* node = nullptr;
        Node* childNode = nullptr;
        Node* connectorNode = nullptr;

        bool isDragStart = true;
        bool hasConnection = false;

        int lastX = 0;
        int lastY = 0;
};
