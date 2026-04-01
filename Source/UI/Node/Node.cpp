/*
  ==============================================================================

    Node.cpp
    Created: 6 May 2025 8:37:02pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "Node.h"
#include "NodeCanvas.h"
#include "../UI/CustomLookAndFeel.h"



Node::Node()
{
    setLookAndFeel(ComponentContext::lookAndFeel);

    upButton.setInterceptsMouseClicks(true,false);
    addAndMakeVisible(upButton);

    downButton.setInterceptsMouseClicks(true,false);
    addAndMakeVisible(downButton);

    nodeTextEditor = std::make_unique<NodeTextEditor>(this);
    nodeTextEditor->bindEditor(midiNoteData,ValueTreeState::MidiPitch);

    addAndMakeVisible(nodeTextEditor.get());
    nodeTextEditor->toBack();

    upButton.onChanged = [this](){
        double editorValue = nodeTextEditor->bindValue.toString().getDoubleValue();

        editorValue += 1;
        nodeTextEditor->bindValue.setValue(editorValue);
        nodeTextEditor->formatDisplay(mode);
    };

    downButton.onChanged = [this](){
        double editorValue = nodeTextEditor->bindValue.toString().getDoubleValue();
        editorValue -= 1;
        nodeTextEditor->bindValue.setValue(editorValue);
        nodeTextEditor->formatDisplay(mode);
    };
}

void Node::paint(juce::Graphics& g)
{
    if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) {
        customLookAndFeel->drawNode(g,*this);
    }
}

void Node::resized()
{
    auto editorArea = getLocalBounds().reduced(10.0f);
    
    upButton.setBounds(editorArea.removeFromTop(4.0f));
    downButton.setBounds(editorArea.removeFromBottom(4.0f));
    
    nodeTextEditor->setBounds(editorArea);
    nodeTextEditor->setJustification(juce::Justification::centred);
}

void Node::setHoverVisual(bool isHovered)
{
    DBG("setting hover visual");
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

void Node::setDisplayMode(NodeDisplayMode mode)
{
    switch (mode){

        case NodeDisplayMode::Pitch:
            nodeTextEditor->bindEditor(nodeValueTree, ValueTreeState::MidiPitch);
            nodeTextEditor->formatDisplay(NodeDisplayMode::Pitch);
            break;

        case NodeDisplayMode::Velocity:
            nodeTextEditor->bindEditor(nodeValueTree, ValueTreeState::MidiVelocity);
            nodeTextEditor->formatDisplay(NodeDisplayMode::Velocity);
            break;

        case NodeDisplayMode::Duration:
            nodeTextEditor->bindEditor(nodeValueTree, ValueTreeState::MidiDuration);
            nodeTextEditor->formatDisplay(NodeDisplayMode::Duration);
            break;

        case NodeDisplayMode::CountLimit:
            nodeTextEditor->bindEditor(nodeValueTree, ValueTreeState::CountLimit);
            nodeTextEditor->formatDisplay(NodeDisplayMode::CountLimit);
            break;

        default:
            break;
    }
}

