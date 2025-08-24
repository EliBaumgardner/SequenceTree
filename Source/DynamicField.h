#pragma once

#include <JuceHeader.h>
#include "DynamicEditor.h"
#include "NodeData.h"

class MenuItem;

class DynamicField : public juce::Component {
public:
    DynamicField(juce::ValueTree tree, bool withMinusButton);
    
    enum class DynamicLabel {
        MidiNote
    };
    
    DynamicField(DynamicLabel label);
    
    ~DynamicField() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void layout(Component* component, int index);
    
    void setValueTree(juce::ValueTree tree);
    
    void createEditor(std::string name,juce::ValueTree tree, juce::Identifier propertyID, bool hint);
    
    void createLabel(std::string name);
    
    bool withMinusButton = false;
    juce::ValueTree tree;

private:
    juce::OwnedArray<DynamicEditor> activeEditors;
    
    class MinusButton : public juce::TextButton {
        public:
        MinusButton() : juce::TextButton("button"){
            setButtonText("-");
        }
        
        void paintButton(juce::Graphics& g, bool, bool) override {
            
            g.fillAll(juce::Colours::white);
            auto bounds = getLocalBounds();
            
            g.setColour(juce::Colours::black);
            g.drawRect(bounds, 2);
            
            auto inner = bounds.reduced(4);
            g.setColour(juce::Colours::white);
            g.fillRect(inner);
            
            g.setColour(juce::Colours::black);
            g.drawText(getButtonText(),inner.reduced(1.0),juce::Justification::centred,false);
        }
        
        bool isPlus;
        
    };
    
    struct EditorInfo {
        std::unique_ptr<DynamicEditor> editor;
        juce::Identifier propertyID;
        juce::ValueTree tree;
    };
   
    juce::OwnedArray<EditorInfo> editorInfos;
    MinusButton minusButton;
};
