/*
  ==============================================================================

    Node.cpp
    Created: 6 May 2025 8:37:02pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "Node.h"
#include "NodeCanvas.h"
#include "../../../../../../../../Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/Frameworks/CoreServices.framework/Frameworks/CarbonCore.framework/Headers/Components.h"
#include "../Logic/NodeController.h"

int Node::globalNodeID = 0;


Node::Node() : nodeID(++globalNodeID)
{

    DBG("NODE CREATED");
    nodeData.setNode(this);

    nodeData.nodeData.setProperty("nodeID",nodeID,nullptr);
    nodeData.nodeData.setProperty("countLimit", 1, nullptr);
    setLookAndFeel(ComponentContext::lookAndFeel);

    upButton.setInterceptsMouseClicks(true,false);
    addAndMakeVisible(upButton);

    downButton.setInterceptsMouseClicks(true,false);
    addAndMakeVisible(downButton);

    editor = std::make_unique<NodeBox>(this);
    editor->setInterceptsMouseClicks(false,false);
    editor->setColour(juce::TextEditor::textColourId, juce::Colours::white);
    editor->bindEditor(nodeData.nodeData,"countLimit");

    addAndMakeVisible(editor.get());
    editor->toBack();

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

void Node::paint(juce::Graphics& g)
{
    DBG("NODE PAINT TRIGGERED");
    if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) {
        customLookAndFeel->drawNode(g,*this);
        DBG("NODE PAINTED");
    }
}

void Node::resized()
{
    auto editorArea = getLocalBounds().reduced(10.0f);
    
    upButton.setBounds(editorArea.removeFromTop(4.0f));
    downButton.setBounds(editorArea.removeFromBottom(4.0f));
    
    editor->setBounds(editorArea);
    editor->setJustification(juce::Justification::centred);
    
    nodeData.nodeData.setProperty("x",getX(), nullptr);
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

    DBG("SETTING NODE DISPLAY MODE");

    juce::ValueTree midiTree("MidiNoteData");
    
    midiTree.setProperty("pitch",60,nullptr);
    midiTree.setProperty("velocity", 60, nullptr);
    midiTree.setProperty("duration", 500, nullptr);
    
    if (nodeData.midiNotes.getNumChildren() == 0)       { nodeData.midiNotes.addChild(midiTree, -1, nullptr);        }
    else if (nodeData.midiNotes.getNumChildren() == 1){ midiTree = nodeData.midiNotes.getChild(0); }

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
    jassert(ComponentContext::nodeController != nullptr);

    ComponentContext::nodeController->setObjects(*this);
}

