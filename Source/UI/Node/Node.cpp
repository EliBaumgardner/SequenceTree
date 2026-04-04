/*
  ==============================================================================

    Node.cpp
    Created: 6 May 2025 8:37:02pm
    Author:  Eli Baimgardner

  ==============================================================================
*/
#include "../Util/ValueTreeState.h"
#include "NodeTextEditor.h"
#include "../Util/ValueTreeIdentifiers.h"
#include "../Util/PluginContext.h"
#include "../UI/CustomLookAndFeel.h"
#include "NodeCanvas.h"
#include "../UI/Node/NodeArrow.h"


#include "Node.h"




Node::Node()
{
    setLookAndFeel(ComponentContext::lookAndFeel);

    upButton.setInterceptsMouseClicks(true,false);
    addAndMakeVisible(upButton);

    downButton.setInterceptsMouseClicks(true,false);
    addAndMakeVisible(downButton);

    nodeTextEditor = std::make_unique<NodeTextEditor>(this);
    nodeTextEditor->bindEditor(midiNoteData,ValueTreeIdentifiers::MidiPitch);

    addAndMakeVisible(nodeTextEditor.get());
    nodeTextEditor->toBack();

    upButton.onChanged = [this](){
        incrementNodeTextEditorValue(1);

    };

    downButton.onChanged = [this](){
        incrementNodeTextEditorValue(-1);
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
    this->mode = mode;

    switch (mode){

        case NodeDisplayMode::Pitch:
            nodeTextEditor->bindEditor(midiNoteData, ValueTreeIdentifiers::MidiPitch);
            break;

        case NodeDisplayMode::Velocity:
            nodeTextEditor->bindEditor(midiNoteData, ValueTreeIdentifiers::MidiVelocity);
            break;

        case NodeDisplayMode::Duration:
            nodeTextEditor->bindEditor(midiNoteData, ValueTreeIdentifiers::MidiDuration);
            break;

        case NodeDisplayMode::CountLimit:
            nodeTextEditor->bindEditor(nodeValueTree, ValueTreeIdentifiers::CountLimit);
            break;

        default:
            break;
    }

    nodeTextEditor->formatDisplay(mode);
}

void Node::incrementNodeTextEditorValue(int incrementValue) {
    if (nodeArrow == nullptr) {
        return;
    }
    nodeArrow->updateFromBindValue = true;
    double editorValue = nodeTextEditor->bindValue.toString().getDoubleValue();
    editorValue += incrementValue;

    nodeTextEditor->bindValue.setValue(editorValue);
    nodeTextEditor->formatDisplay(nodeTextEditor->mode);
}
