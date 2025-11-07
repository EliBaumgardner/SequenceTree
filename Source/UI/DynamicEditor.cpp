/*
  ==============================================================================

    DynamicEditor.cpp
    Created: 18 May 2025 10:55:47am
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "DynamicEditor.h"
#include "../Util/ComponentContext.h"
#include "../Node/NodeCanvas.h"

DynamicEditor::DynamicEditor(){

    setLookAndFeel(ComponentContext::lookAndFeel);

    //refit the text when it changes
    onTextChange = [this](){
        
        bool changed = textChanged(bindValue.toString());
        
        if(showHint && !wasFocused && !changed){
        
            setColour(juce::TextEditor::textColourId, juce::Colours::grey);
            setText(hintText);
        }
        else if(editable){
            
            
            setColour(juce::TextEditor::textColourId, juce::Colours::black);
            showHint = false;
            if(tree.hasType("MidiNoteData")){
               ComponentContext::canvas->makeRTGraph(ComponentContext::canvas->root);
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
    
    getTextValue().referTo(bindValue);
    //onTextChange();
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

void DynamicEditor::paint(juce::Graphics& g){

    if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawEditor(g,*this); }
    //TextEditor::paint(g);                   //paint editor + ...
 
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
}

void DynamicEditor::formatDisplay(DisplayMode mode){
    
    juce::String displayValue = bindValue.toString();
    
    std::cout<<displayValue<<std::endl;
    double value = displayValue.getDoubleValue();
    
    juce::String display;
    
    if(mode == DisplayMode::Pitch){
        value += 1;
        int valueRange = (int)value % 127;
        int pitchValue = (int)valueRange % 11;
        int octave = (valueRange+1) / 12;
        
        juce::String pitchNames[] = {
            juce::String(L"C"),
            juce::String(L"C♯"),
            juce::String(L"D"),
            juce::String(L"D♯"),
            juce::String(L"E"),
            juce::String(L"F"),
            juce::String(L"F♯"),
            juce::String(L"G"),
            juce::String(L"G♯"),
            juce::String(L"A"),
            juce::String(L"A♯"),
            juce::String(L"B")
        };

        display = pitchNames[pitchValue] + juce::String(octave);
    }
    
    if(mode == DisplayMode::Velocity){
        int velocity = (int)value;
        display = juce::String(velocity);
    }
    
    if(mode == DisplayMode::Duration){
        display = juce::String(value);
    }
    
    setText(display);
    refit();
}

int DynamicEditor::noteToNumber(juce::String string){
    
    //note  //accidental  //octave
    
    char note = string[1];
    juce::juce_wchar accidental;
    char octave;
    
    int midiNumber = 0;
    int accidentalValue = 0;
    
    if(string.length() > 2){
        accidental = string[2];
        octave = string[3];
        
        switch(accidental) {
            case 0x266F: accidentalValue = 1;
                break;
                
            case 0x266D: accidentalValue = -1;
                break;
        }
    }
    else {
        octave = string [2];
    }
    
    
    midiNumber = note - 97;
    midiNumber += (octave - 49) * 12;
    midiNumber += accidentalValue;
    
    return midiNumber;
}
