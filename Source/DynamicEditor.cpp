/*
  ==============================================================================

    DynamicEditor.cpp
    Created: 18 May 2025 10:55:47am
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "DynamicEditor.h"
#include "ComponentContext.h"
#include "NodeCanvas.h"
#include "Node.h"

DynamicEditor::DynamicEditor(){
    
    makeBoundsVisible(true);
   
    //refit the text when it changes
    onTextChange = [this](){
        
        bool changed = textChanged(bindValue.toString());
        
        if(showHint && !wasFocused && !changed){
        
            setColour(juce::TextEditor::textColourId, juce::Colours::grey);
            setText(hintText);
        }
        else if(editable){
            
            //setText(bindValue.toString());
            getTextValue().referTo(bindValue);
            setColour(juce::TextEditor::textColourId, juce::Colours::black);
            showHint = false;
            if(tree.hasType("MidiNoteData")){
               ComponentContext::canvas->updateProcessorGraph(ComponentContext::canvas->root);
            }
        }
        else {
            
            setColour(juce::TextEditor::textColourId, juce::Colours::black);
            setText(labelText);
        }
        refit();
    };
    
    baseFont = juce::Font(getFont());
    
}

DynamicEditor::~DynamicEditor(){
    getTextValue().referTo(juce::Value());  // unbind TextEditor
    bindValue = juce::Value();              // unbind from ValueTree
}

void DynamicEditor::bindEditor(juce::ValueTree tree, const juce::Identifier propertyID){
    
    this->tree = tree;
    if(bindValue.toString().isEmpty()){
        bindValue.referTo(tree.getPropertyAsValue(propertyID,nullptr));
        oldString = bindValue.toString();
    }
    else {
        bindValue.referTo(tree.getPropertyAsValue(propertyID,nullptr));
    }
    
    onTextChange();
}

void DynamicEditor::refit(){
        auto bounds = getLocalBounds().toFloat().reduced(4.0f);
        
        float length = baseFont.getStringWidthFloat(getText());
        float height = baseFont.getHeight();
        float ratio = std::min(bounds.getWidth()/length,bounds.getHeight()/height);
        
        height *= ratio;
        
        auto font = baseFont;
        font.setHeight(height);
        setFont(font);
        applyFontToAllText(font);
}

void DynamicEditor::setEditable(bool editable){
    this->editable = editable;
    wasFocused = editable;
    
    setReadOnly(!editable);
    setCaretVisible(editable);
    setInterceptsMouseClicks(editable, false);
}

void DynamicEditor::setGroup(juce::String groupID){
    this->groupID = groupID;
}

void DynamicEditor::clearBindings() {
    getTextValue().referTo(juce::Value());
    bindValue.referTo(juce::Value());
}

void DynamicEditor::makeBoundsVisible(bool isBoundsVisible){
    
    //set whether the textbox has visible boundaries
    if(isBoundsVisible){
        setColour(juce::TextEditor::textColourId, juce::Colours::black);
        setOpaque(false);
        setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::TextEditor::outlineColourId, juce::Colours::black);
        setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::black);
        setColour(juce::TextEditor::highlightColourId, juce::Colours::black);
    }
    else {
        setColour(juce::TextEditor::textColourId, juce::Colours::black);
        setOpaque(false);
        setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::TextEditor::highlightColourId, juce::Colours::transparentBlack);
    }
    repaint();
}

void DynamicEditor::paint(juce::Graphics& g){
    
    TextEditor::paint(g);                   //paint editor + ...
 
}

void DynamicEditor::displayHint(bool showHint, juce::String hintText){
    this->showHint = showHint;
    this->hintText = hintText;
    //setText(hintText);
}

void DynamicEditor::mouseDown(const juce::MouseEvent& e) {
    juce::TextEditor::mouseDown(e);
    wasFocused = true;
    onTextChange();
    
}

void DynamicEditor::setLabelText(juce::String string){
    labelText = string;
    setText(labelText);
}

bool DynamicEditor::textChanged(juce::String string){
    if(string == oldString || string == hintText)
        return false;
    
    oldString = string;
    return true;
};
