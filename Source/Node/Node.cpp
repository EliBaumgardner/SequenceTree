/*
  ==============================================================================

    Node.cpp
    Created: 6 May 2025 8:37:02pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "Node.h"
#include "NodeCanvas.h"

int Node::globalNodeID = 0;

Node::Node(NodeCanvas* nodeCanvas) : nodeCanvas(nodeCanvas),nodeID(++globalNodeID){
    
    editor = std::make_unique<NodeBox>(this);
    
    addAndMakeVisible(upButton);
    addAndMakeVisible(downButton);
    addAndMakeVisible(editor.get());
    
    nodeLogic.setNode(this);
    nodeData.setNode(this);
    nodeController = std::make_unique<ObjectController>(this);
    this->addMouseListener(nodeController.get(), true);
    
    editor->setColour(juce::TextEditor::textColourId, juce::Colours::white);
    editor->bindEditor(nodeData.nodeData,"countLimit");
    //editor.get()->formatDisplay(NodeBox::DisplayMode::CountLimit);
    //editor.refit();
    
//    editor.get()->onTextChange = [=]()
//        {
//            if(editor.get()->getText().getIntValue() != 0){
//                nodeLogic.setCountLimit(editor.get()->getText().getIntValue());
//                nodeData.nodeData.setProperty("countLimit",editor.get()->getText().getIntValue(),nullptr);
//                nodeCanvas->updateProcessorGraph(nodeCanvas->root);
//            }
//        };
    
    upButton.onChanged = [this](){
        double value = editor->bindValue.toString().getDoubleValue();
        value += 1;
        editor->bindValue.setValue(value);
        editor->formatDisplay(editor->mode);
    };
    
    downButton.onChanged = [this](){
        double value = editor->bindValue.toString().getDoubleValue();
        value -= 1;
        editor->bindValue.setValue(value);
        editor->formatDisplay(editor->mode);
    };
}

Node::~Node(){
    
    //this->removeMouseListener(nodeController);
}

void Node::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto circleBorder = bounds.reduced(2.5f);
    auto circleSelect = bounds.reduced(0.5f);
    auto circleHover = bounds.reduced(4.5f);
    auto circleFill = bounds.reduced(5.5f);

    g.setColour(juce::Colours::black);
    g.drawEllipse(circleBorder, 1.0f);

    
    g.setColour(isHighlighted ? nodeColour.darker() : nodeColour);
    g.fillEllipse(circleFill);
    
    if (isHovered)
        g.drawEllipse(circleHover, 2.0f);
    
    if (isSelected)
    {
        juce::Path dottedPath;
        dottedPath.addEllipse(circleSelect);

        juce::PathStrokeType stroke(0.325f);

        float dashLengths[] = { 2.935f, 2.935f };
        stroke.createDashedStroke(dottedPath, dottedPath, dashLengths, 2);

        g.setColour(juce::Colours::black);
        g.strokePath(dottedPath, stroke);
    }
}


void Node::resized(){
    
    auto editorArea = getLocalBounds().reduced(10.0f);
    
    upButton.setBounds(editorArea.removeFromTop(4.0f));
    downButton.setBounds(editorArea.removeFromBottom(4.0f));
    
    editor->setBounds(editorArea);
    editor->setJustification(juce::Justification::centred);
    //editor.refit();
    
    nodeData.nodeData.setProperty("x",getX(),nullptr);
    nodeData.nodeData.setProperty("y",getY(),nullptr);
    nodeData.nodeData.setProperty("radius", getWidth()/2,nullptr);
}

void Node::setHoverVisual(bool isHovered){
    
    this->isHovered = isHovered;
    repaint();
}

void Node::setSelectVisual(bool isSelected){
    
    this->isSelected = isSelected;
    repaint();
}

void Node::setSelectVisual(){
    
    isSelected = !isSelected;
    repaint();
}

void Node::setHighlightVisual(bool isHighlighted){
    
    this->isHighlighted = isHighlighted;
    repaint();
}

void Node::setDisplayMode(NodeBox::DisplayMode mode){
    
    
    juce::ValueTree midiTree("MidiNoteData");
    
    midiTree.setProperty("pitch",60,nullptr);
    midiTree.setProperty("velocity", 60, nullptr);
    midiTree.setProperty("duration", 500, nullptr);
    
    if(nodeData.midiNotes.isEmpty()){
        nodeData.midiNotes.add(midiTree);
    }
    else if(nodeData.midiNotes.size() == 1) {
        midiTree = nodeData.midiNotes.getLast();
    }
    
    if(mode == NodeBox::DisplayMode::Pitch){
        editor->bindEditor(midiTree, "pitch");
        editor->formatDisplay(NodeBox::DisplayMode::Pitch);
    }
    
    if(mode == NodeBox::DisplayMode::Velocity){
        editor->bindEditor(midiTree, "velocity");
        editor->formatDisplay(NodeBox::DisplayMode::Velocity);
    }
    
    if(mode == NodeBox::DisplayMode::Duration){
        editor->bindEditor(midiTree, "duration");
        editor->formatDisplay(NodeBox::DisplayMode::Duration);
    }
    
    if(mode == NodeBox::DisplayMode::CountLimit){
        editor->bindEditor(nodeData.nodeData,"countLimit");
        editor->formatDisplay(NodeBox::DisplayMode::CountLimit);
    }
}

void Node::mouseEnter(const juce::MouseEvent &e) {


}
