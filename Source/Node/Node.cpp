/*
  ==============================================================================

    Node.cpp
    Created: 6 May 2025 8:37:02pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "Node.h"
#include "NodeCanvas.h"
#include "../Logic/ObjectController.h"

int Node::globalNodeID = 0;

Node::Node() : nodeID(++globalNodeID)
{
    upButton.setInterceptsMouseClicks(false,false);
    addAndMakeVisible(upButton);

    downButton.setInterceptsMouseClicks(false,false);
    addAndMakeVisible(downButton);

    editor = std::make_unique<NodeBox>(this);
    editor->setInterceptsMouseClicks(false,false);
    editor->setColour(juce::TextEditor::textColourId, juce::Colours::white);
    editor->bindEditor(nodeData.nodeData,"countLimit");
    addAndMakeVisible(editor.get());

    nodeLogic.setNode(this);
    nodeData.setNode(this);

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

Node::~Node()
{
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
    
    if (isHovered) { g.drawEllipse(circleHover, 2.0f); }
    
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

void Node::resized()
{
    auto editorArea = getLocalBounds().reduced(10.0f);
    
    upButton.setBounds(editorArea.removeFromTop(4.0f));
    downButton.setBounds(editorArea.removeFromBottom(4.0f));
    
    editor->setBounds(editorArea);
    editor->setJustification(juce::Justification::centred);
    
    nodeData.nodeData.setProperty("x",getX(),nullptr);
    nodeData.nodeData.setProperty("y",getY(),nullptr);
    nodeData.nodeData.setProperty("radius", getWidth()/2,nullptr);
}

void Node::setHoverVisual(bool isHovered)
{
    this->isHovered = isHovered;
    repaint();
}

void Node::setSelectVisual(bool isSelected)
{
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

void Node::setDisplayMode(NodeBox::DisplayMode mode)
{
    juce::ValueTree midiTree("MidiNoteData");
    
    midiTree.setProperty("pitch",60,nullptr);
    midiTree.setProperty("velocity", 60, nullptr);
    midiTree.setProperty("duration", 500, nullptr);
    
    if (nodeData.midiNotes.isEmpty()) {
    nodeData.midiNotes.add(midiTree);
}
else if (nodeData.midiNotes.size() == 1) {
    midiTree = nodeData.midiNotes.getLast();
}

    switch (mode){
    case NodeBox::DisplayMode::Pitch:
        editor->bindEditor(midiTree, "pitch");
        editor->formatDisplay(NodeBox::DisplayMode::Pitch);
        break;

    case NodeBox::DisplayMode::Velocity:
        editor->bindEditor(midiTree, "velocity");
        editor->formatDisplay(NodeBox::DisplayMode::Velocity);
        break;

    case NodeBox::DisplayMode::Duration:
        editor->bindEditor(midiTree, "duration");
        editor->formatDisplay(NodeBox::DisplayMode::Duration);
        break;

    case NodeBox::DisplayMode::CountLimit:
        editor->bindEditor(nodeData.nodeData, "countLimit");
        editor->formatDisplay(NodeBox::DisplayMode::CountLimit);
        break;

    default:
        break;
    }
}

void Node::mouseEnter(const juce::MouseEvent &e)
{

    if (ComponentContext::canvas->controller != nullptr) {


        ComponentContext::canvas->controller->setObjects(this);
        //addMouseListener(ComponentContext::canvas->controller.get(),true);
    }
}

