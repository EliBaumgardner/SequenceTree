/*
  ==============================================================================

    SelectionBar.h
    Created: 27 Aug 2025 6:30:12pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PianoRoll.h"
#include "ComponentContext.h"
#include "Node.h"
#include "NodeCanvas.h"
#include "DynamicEditor.h"
#include "NodeBox.h"


static constexpr float buttonBoundsReduction = 3.0f;
static constexpr float buttonBorderThickness = 2.0f;
static constexpr float buttonContentBounds = 4.0f;
static constexpr float buttonSelectedThickness = 4.0f;

class SelectionBar : public juce::Component {
    
    public:
    
    SelectionBar();
    void resized() override;
    void paint(juce::Graphics& g) override;
    
    
    
    private:
    
    class NodeButton : public juce::Component {
        
        public:
        
        bool buttonSelected = true;
        std::function<void()> onClick;
        
        NodeButton() {}
        void paint(juce::Graphics& g) override {
            
            auto bounds = getLocalBounds().toFloat().reduced(buttonBoundsReduction);
            auto circleBounds = getLocalBounds().toFloat().reduced(buttonContentBounds);
            g.setColour(juce::Colours::black);
            
            if(buttonSelected){
                g.drawRect(bounds,buttonSelectedThickness);
            }
            else {
                g.drawRect(bounds,buttonBorderThickness);
            }
            
            g.drawEllipse(circleBounds, 1.0f);
            
        }
        
        void mouseDown(const juce::MouseEvent& e) override {
            onClick();
        }
        
    };
    
    //BUTTONS//
    
    class InspectButton : public juce::Component {
        
        public:
        
        bool buttonSelected = false;
        
        std::function<void()> onClick;
        InspectButton() {}
        
        void paint (juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat().reduced(buttonBoundsReduction);
            g.setColour(juce::Colours::black);
            
            if(buttonSelected){
                g.drawRect(bounds,buttonSelectedThickness);
            }
            else {
                g.drawRect(bounds, buttonBorderThickness);
            }
            
            auto cursorBounds = getLocalBounds().toFloat().reduced(buttonContentBounds);
            float w = cursorBounds.getWidth();
            float h = cursorBounds.getHeight();
            
            juce::Path cursorPath;
            
            cursorPath.startNewSubPath(9.0f, 0.0f);
            
            cursorPath.lineTo(0.0f, h);
            
            cursorPath.lineTo(w * 0.3f, h * 0.5f);
            
            cursorPath.lineTo(w, h );
            
            cursorPath.closeSubPath();
            
            juce::AffineTransform translation = juce::AffineTransform::translation(5.0f, 3.0f);
            cursorPath.applyTransform(translation);
            
            // Fill and outline
            g.setColour(juce::Colours::black);
            g.fillPath(cursorPath);
            
            g.setColour(juce::Colours::white);
            g.strokePath(cursorPath, juce::PathStrokeType(1.0f));
        }
        
        void mouseDown(const juce::MouseEvent& e) override {
            onClick();
        }
        
    };
    
    class PianoRollButton : public juce::Component {
        
        public:
        
        class PianoRollWindow : public juce::DocumentWindow {
            
            public:
            
            PianoRoll* pianoRoll = nullptr;
            
            PianoRollWindow(const juce::String name, juce::Colour backgroundColour, int requiredButtons, bool addToDeskTop) : DocumentWindow(name,backgroundColour, requiredButtons, addToDeskTop) {
                
                pianoRoll = new PianoRoll();
                setContentOwned(pianoRoll, true);
                setResizable(true,true);
                
            }
            
            void closeButtonPressed() override {
                setVisible(false);
            }
            
            
        };
        
        std::unique_ptr<PianoRollWindow> pianoRollWindow = nullptr;
        bool buttonSelected = false;
        
        PianoRollButton() {}
        
        void paint(juce::Graphics& g) override {
            //find piano icon
            
            auto bounds = getLocalBounds().toFloat().reduced(buttonBoundsReduction);
            g.setColour(juce::Colours::black);
            
            if(buttonSelected){
                g.drawRect(bounds,buttonSelectedThickness);
            }
            else {
                g.drawRect(bounds,buttonBorderThickness);
            }
        }
        
        void mouseDown(const juce::MouseEvent& e) override {
            //open documentwindow containing piano roll;
            if(pianoRollWindow != nullptr){
                pianoRollWindow.reset();
            }
            
            pianoRollWindow = std::make_unique<PianoRollWindow>("",juce::Colours::white,juce::DocumentWindow::closeButton,true);
            pianoRollWindow->centreWithSize(200,200);
            pianoRollWindow->setVisible(true);
            
        }
    };
    
    class SelectionButton : public juce::Component {
        
        public:
        
        class PopUpButton : public juce::Component {
            
            public:
            
            std::function<void()> onClick;
            PopUpButton() {};
            void paint(juce::Graphics& g) override {
                auto bounds = getLocalBounds().toFloat().reduced(buttonBoundsReduction);
                auto contentBounds = getLocalBounds().toFloat().reduced(buttonContentBounds);
                g.setColour(juce::Colours::black);
                g.drawRect(bounds,buttonBorderThickness);
                
                juce::Path vPath;
                vPath.startNewSubPath(bounds.getTopLeft());
                vPath.lineTo(getWidth()/2,getHeight());
                vPath.lineTo(bounds.getTopRight());
                vPath.closeSubPath();
                
                juce::AffineTransform translation = juce::AffineTransform::translation(5.0f,3.0f);
                
                vPath.applyTransform(translation);
                g.strokePath(vPath, juce::PathStrokeType(1.0f));
                
            }
            
            void mouseDown(const juce::MouseEvent& e) override {
                onClick();
            }
        };
        
        //VARIABLES
        PopUpButton button;
        DynamicEditor display;
        juce::PopupMenu menu;
        juce::String selectedOption = "";
        //
        
        SelectionButton() {
            addAndMakeVisible(display);
            addAndMakeVisible(button);
            
            menu.addItem(1, "show pitch");
            menu.addItem(2, "show velocity");
            menu.addItem(3, "show duration");
            menu.addItem(4, "show countLimit");
            
            button.onClick = [this]() {
                menu.showMenuAsync (juce::PopupMenu::Options(), [this] (int result)
                {
                    if(result == 1) { selectedOption = "show pitch"; ComponentContext::canvas->setSelectionMode(NodeBox::DisplayMode::Pitch);}
                    if(result == 2) { selectedOption = "show velocity"; ComponentContext::canvas->setSelectionMode(NodeBox::DisplayMode::Velocity);}
                    if(result == 3) { selectedOption = "show duration"; ComponentContext::canvas->setSelectionMode(NodeBox::DisplayMode::Duration);}
                    if(result == 4) { selectedOption = "show coutLimit";
                        ComponentContext::canvas->setSelectionMode(NodeBox::DisplayMode::CountLimit);}
                    
                    resized();
                });
            };
        }

        void paint(juce::Graphics& g) override {
            
            auto bounds = getLocalBounds().toFloat().reduced(buttonBoundsReduction);
            g.setColour(juce::Colours::black);
            g.drawRect(bounds,buttonBorderThickness);
        }
        
        void resized() override {
            auto contentBounds = getLocalBounds().reduced(buttonContentBounds);
            auto displayWidth = contentBounds.getWidth() * 2/3;

            display.setBounds(contentBounds.removeFromLeft(displayWidth));
            display.setText(selectedOption);
            display.refit();
            
            button.setBounds(contentBounds);
            
        }
        
        private:
    };
    
    SelectionButton selectionButton;
    PianoRollButton pianoRollButton;
    NodeButton nodeButton;
    InspectButton inspectButton;
};

