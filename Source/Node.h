/*
  ==============================================================================

    Node.h
    Created: 6 May 2025 8:37:02pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include "ProjectModules.h"
#include "DynamicEditor.h"
#include "NodeBox.h"

#include "ObjectController.h"
#include "NodeLogic.h"
#include "NodeData.h"

class  NodeCanvas;

class Node : public juce::Component {
    
    public:
    
        Node(NodeCanvas* nodeCanvas);
    
        ~Node() override;
    
        void paint(juce::Graphics& g) override;
        
        void resized() override;
    
        void setHoverVisual(bool isHovered);
    
        void setSelectVisual(bool isSelected);
    
        void setSelectVisual();
    
        void setHighlightVisual(bool isHighlighted);
    
        NodeData* getNodeData();
    
        NodeLogic nodeLogic;
    
        Node* parent = nullptr;
        Node* root = nullptr;
        NodeCanvas* nodeCanvas = nullptr;

        std::unique_ptr<ObjectController> nodeController = nullptr;
        NodeData nodeData;
    
        std::unique_ptr<NodeBox> editor = nullptr;
        NodeBox::DisplayMode mode;
    
        bool isRoot = false;
        
        
    
        int nodeID = 0;
        static int globalNodeID;
    
        juce::Colour nodeColour = juce::Colours::blue;
    
        void setDisplayMode(NodeBox::DisplayMode mode);

        bool isHovered = false;
        bool isSelected = false;
        bool isHighlighted = false;


    //BUTTONS//


        class IncrementButton : public juce::Component {
            
            public:
            
            bool increment;
            std::function<void()> onChanged;
            
            IncrementButton(bool increment) : increment(increment) {
                
            }
            
            void paint(juce::Graphics& g) override {
                
                auto bounds = getLocalBounds().toFloat();
                auto w = bounds.getWidth();
                auto h = bounds.getHeight();
                
                if(increment == true){
                    juce::Path path;
                    
                    path.startNewSubPath(0, h);
                    path.lineTo(w / 2, 0);
                    path.lineTo(w, h);
                    path.closeSubPath();

                    g.setColour(juce::Colours::white);
                    g.fillPath(path);
                    
                    g.setColour(juce::Colours::black);
                    g.strokePath(path, juce::PathStrokeType(1.0f));
                }
                else {
                    juce::Path path;
                    
                    path.startNewSubPath(0,0);
                    path.lineTo(w/2,h);
                    path.lineTo(w,0);
                    path.closeSubPath();
                    
                    g.setColour(juce::Colours::white);
                    g.fillPath(path);
                    
                    g.setColour(juce::Colours::black);
                    g.strokePath(path, juce::PathStrokeType(1.0f));
                }
            }
            
            void mouseDown(const juce::MouseEvent& e) override {
                
                onChanged();
            }
            
        };
    
    
        IncrementButton upButton { true };
        IncrementButton downButton { false };
};
