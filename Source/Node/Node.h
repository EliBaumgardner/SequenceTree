/*
  ==============================================================================

    Node.h
    Created: 6 May 2025 8:37:02pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include "../CustomLookAndFeel.h"
#include "../Util/ProjectModules.h"
#include "NodeBox.h"
#include "../Logic/ObjectController.h"
#include "NodeData.h"

class NodeController;

class  NodeCanvas;

class Node : public juce::Component {
    
    public:

        Node();
    
        ~Node() override;
    
        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseEnter(const juce::MouseEvent& e) override;
    
        void setHoverVisual(bool isHovered);
        void setSelectVisual(bool isSelected);
        void setSelectVisual();
        void setHighlightVisual(bool isHighlighted);

        void setDisplayMode(NodeBox::DisplayMode mode);

        //Object Variabes//
        Node* parent = nullptr;
        Node* root = nullptr;

        NodeData nodeData;
    
        std::unique_ptr<NodeBox> editor = nullptr;
        NodeBox::DisplayMode mode;
    
        juce::Colour nodeColour = juce::Colour::fromRGB(91,86,76);

        //Primative Variables//
        bool isHovered = false;
        bool isSelected = false;
        bool isHighlighted = false;
        bool isRoot = false;

        int nodeID = 0;
        static int globalNodeID;

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

        bool isConnector = false;

        bool isARoot = false;
};
