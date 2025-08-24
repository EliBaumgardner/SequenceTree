/*
  ==============================================================================

    Node.h
    Created: 6 May 2025 8:37:02pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DynamicEditor.h"

#include "NodeController.h"
#include "NodeLogic.h"
#include "NodeData.h"



class  NodeCanvas;

class Node : public juce::Component {
    
    public:
    
        Node(NodeCanvas* nodeCanvas);
    
        ~Node() override;
    //public methods
        void paint(juce::Graphics& g) override;
        
        void resized() override;
    
        void setHoverVisual(bool isHovered);
    
        void setSelectVisual(bool isSelected);
    
        void setSelectVisual();
    
        void setHighlightVisual(bool isHighlighted);
    
        NodeData* getNodeData();

        juce::Colour nodeColour = juce::Colours::blue;
    
        NodeLogic nodeLogic;
    
        Node* parent = nullptr;
        NodeCanvas* nodeCanvas = nullptr;
        //NodeController* nodeController = nullptr;
    
        std::unique_ptr<NodeController> nodeController = nullptr;

        NodeData nodeData;

        DynamicEditor editor;
    
        bool isRoot = false;

        int nodeID = 0;
        static int globalNodeID;
    
    private:
    
        bool isHovered = false;
        bool isSelected = false;
        bool isHighlighted = false;
    
    
};
