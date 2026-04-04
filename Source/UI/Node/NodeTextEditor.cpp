/*
  ==============================================================================
`
    NodeTextEditor.cpp
    Created: 1 Sep 2025 1:28:52pm
    Author:  Eli Baimgardner

  ==============================================================================
*/


#include "NodeCanvas.h"
#include "Node.h"
#include "NodeArrow.h"
#include "../Util/ValueTreeState.h"
#include "../Util/PluginContext.h"


#include "NodeTextEditor.h"

 static const juce::String pitchNames[] = {
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

NodeTextEditor::NodeTextEditor(Node* node) : node(node) {

    addListener(this);
    makeBoundsVisible(false);
    setReadOnly(false);

    setInterceptsMouseClicks(false,false);
    setColour(juce::TextEditor::textColourId, juce::Colours::white);
    
    baseFont = juce::Font(getFont());
}

void NodeTextEditor::paint(juce::Graphics& g) {
    TextEditor::paint(g);
}

void NodeTextEditor::refit(){
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

void NodeTextEditor::bindEditor(juce::ValueTree tree, const juce::Identifier propertyID){
    
    this->nodeTree = tree;
    
    bindValue.referTo(tree.getPropertyAsValue(propertyID,nullptr));
    
}

void NodeTextEditor::formatDisplay(NodeDisplayMode mode) {

    jassert(node);

    this->mode = mode;

    juce::String displayValue = bindValue.toString();
    double value = displayValue.getDoubleValue();
    
    juce::String display;

    int nodeId = node->getComponentID().getIntValue();

    juce::ValueTree nodeValueTree = ValueTreeState::getNode(nodeId);

    if (mode == NodeDisplayMode::Pitch){
        int midiNote = juce::jlimit(0, 127, (int)value);

        int pitchValue = midiNote % 12;
        int octave = (midiNote / 12) - 1;

        display = pitchNames[pitchValue] + juce::String(octave);
    }

    if(mode == NodeDisplayMode::Velocity){
        int velocity = (int)value;
        display = juce::String(velocity);
    }

    if(mode == NodeDisplayMode::Duration){
        display = juce::String(value);
    }

    if(mode == NodeDisplayMode::CountLimit){
        display = juce::String(value);
    }
    
    setText(display);
    refit();

    ComponentContext::canvas->makeRTGraph(nodeValueTree);
}

int NodeTextEditor::noteToNumber(juce::String string){
    
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

void NodeTextEditor::makeBoundsVisible(bool isBoundsVisible){
    
    //set whether the textbox has visible boundaries
    if(isBoundsVisible){
        setColour(juce::TextEditor::textColourId, juce::Colours::black);
        setOpaque(false);
        setColour(juce::TextEditor::backgroundColourId, juce::Colours::white);
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

void NodeTextEditor::mouseDrag(const juce::MouseEvent& e){
    int distanceFromStart = e.getDistanceFromDragStartY();

    if(distanceFromStart > 10){
        
        int attenuate = juce::jlimit(0, 127,distanceFromStart);
        
        int midiValue = static_cast<int>(bindValue.getValue()) + attenuate;
        
        bindValue.setValue(midiValue);
        
        formatDisplay(mode);
    }
}

void NodeTextEditor::textEditorReturnKeyPressed(juce::TextEditor &editor) {

    DBG("return key pressed");
    node->nodeArrow->updateFromBindValue = true;

    int text = editor.getText().getIntValue();

    node->nodeArrow->bindValue.setValue(text);

}
