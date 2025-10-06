/*
  ==============================================================================

    MenuItem.h
    Created: 1 Jun 2025 5:41:42pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once
#include "ProjectModules.h"
#include "DynamicEditor.h"
#include "DynamicField.h"
#include "Node.h"
#include "NodeData.h"

class MenuItem : public juce::Component {
  
    public:
    
    enum class ItemType { MidiCCItem,MidiNoteItem };
    
    MenuItem(juce::String labelText, ItemType type,Node* node);
    
    MenuItem();
    ~MenuItem() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;

    void addComponent(juce::Component* compoennt);
    void removeComponent(juce::Component* component);
    
    void changeNode(Node* node);
    
    juce::Array<juce::Component*> children;
    
    int getMenuItemHeight();
    
    bool getOpenState();
    void setOpenState(bool state);
    
    bool open = true;
    
    std::unordered_map<Node*,juce::Array<juce::Component*>> fields;
    std::vector<std::unique_ptr<DynamicField>> fieldStorage;
    
    Node* selectedNode;
    
    private:
    
        ItemType type;
    
        bool isRoot = false;
        int height = 0;
        int childHeightDiv = 12;
        
        std::unique_ptr<DynamicEditor> menuLabel;

        class ArrowButton : public juce::Button {
            
            public:
                std::function<void()> onToggle;
            
                ArrowButton() : juce::Button("arrow"){}
                
                void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override {
                    
                    juce::Path arrow;
                    float w = getWidth();
                    float h = getHeight();
                    
                    if(isOpen){
                        arrow.startNewSubPath(w*0.2,h*0.8);
                        arrow.lineTo(w*0.5,h*0.5);
                        arrow.lineTo(w*0.8,h*0.8);
                    }
                    else{
                        arrow.startNewSubPath(w*0.2,h*0.2);
                        arrow.lineTo(w*0.5,h*0.5);
                        arrow.lineTo(w*0.8,h*0.2);
                    }
                    
                    g.setColour(juce::Colours::black);
                    g.strokePath(arrow, juce::PathStrokeType(2.0f));
                }
            
                void mouseDown(const juce::MouseEvent& event) override {
                    isOpen = !isOpen;
                    repaint();
                    
                    if(onToggle)
                        onToggle();
                }
            
                bool isOpen = false;
        };
    
        class PlusMinusButton : public juce::TextButton {
          
            public:
                
                PlusMinusButton(bool isPlus) : juce::TextButton("button"), isPlus(isPlus){
                    
                    setButtonText(isPlus ? "+" : "-");
                    setColour(juce::TextButton::buttonColourId,juce::Colours::black);
                }
            
            void paintButton (juce::Graphics& g, bool, bool) override {
                
                    g.fillAll(juce::Colours::white);
                
                    auto bounds = getLocalBounds();
                
                    g.setColour(juce::Colours::black);
                    g.drawRect(bounds,2);
                    
                    auto inner = bounds.reduced(4);
                    g.setColour(juce::Colours::white);
                    g.fillRect(inner);
                
                    g.setColour(juce::Colours::black);
                    g.drawText(getButtonText(),inner.reduced(1.0),juce::Justification::centred,false);
            }
            
                bool isPlus;
        };
        
        ArrowButton arrowButton;
        std::unique_ptr<PlusMinusButton> addButton = std::make_unique<PlusMinusButton>(true);
    
        DynamicField* fieldLabel = nullptr;
    
        juce::OwnedArray<PlusMinusButton> minusButtons;
    
};


