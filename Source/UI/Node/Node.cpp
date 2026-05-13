/*
  ==============================================================================

    Node.cpp
    Created: 6 May 2025 8:37:02pm
    Author:  Eli Baimgardner

  ==============================================================================
*/
#include "../../Graph/ValueTreeState.h"
#include "NodeTextEditor.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"
#include "NodeArrow.h"


#include "Node.h"




Node::Node(ApplicationContext& context) : applicationContext(context), countEditor(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    upButton.setInterceptsMouseClicks(true,false);
    addAndMakeVisible(upButton);

    downButton.setInterceptsMouseClicks(true,false);
    addAndMakeVisible(downButton);

    nodeTextEditor = std::make_unique<NodeTextEditor>(this, context);
    nodeTextEditor->bindEditor(midiNoteData,ValueTreeIdentifiers::MidiPitch);

    addAndMakeVisible(nodeTextEditor.get());
    nodeTextEditor->toBack();

    countEditor.setInterceptsMouseClicks(true, false);
    countEditor.setTooltip("Count Limit");
    addAndMakeVisible(countEditor);

    upButton.onChanged = [this]() {
        incrementNodeTextEditorValue(1);
    };

    downButton.onChanged = [this]() {
        incrementNodeTextEditorValue(-1);
    };
}

void Node::paint(juce::Graphics& g)
{
    CustomLookAndFeel::get(*this).drawNode(g, *this);
}

void Node::resized()
{
    auto editorArea = getLocalBounds().reduced(10.0f);
    int  buttonH    = juce::jmax(2, (int)(editorArea.getHeight() * 0.2f));

    upButton.setBounds(editorArea.removeFromTop(buttonH));
    downButton.setBounds(editorArea.removeFromBottom(buttonH));

    nodeTextEditor->setBounds(editorArea);
    nodeTextEditor->setJustification(juce::Justification::centred);

    int cw = (int)(getWidth()  * 0.45f);
    int ch = (int)(getHeight() * 0.30f);
    countEditor.setBounds(getWidth() - cw, 0, cw, ch);
}

void Node::setHoverVisual(bool isHovered)
{
    this->isHovered = isHovered;
    repaint();
}

void Node::setSelectVisual(bool isSelected)
{
    this->isSelected = isSelected;
    if (onSelected) {
        onSelected(this, isSelected);
    }
    repaint();
}

void Node::setSelectVisual() {
    isSelected = !isSelected;
    if (onSelected) {
        onSelected(this, isSelected);
    }
    repaint();
}

void Node::setHighlightVisual(bool h)
{
    if (h) {
        pendingHighlightOff = false;
        isHighlighted = true;
        pulsePhase = 0.0f;
        startTimerHz(60);
        repaint();
    } else {
        if (isTimerRunning()) {
            pendingHighlightOff = true;
        } else {
            isHighlighted = false;
            repaint();
        }
    }
}

void Node::timerCallback()
{
    pulsePhase += 0.07f;
    if (pulsePhase >= 1.0f) {
        pulsePhase = 1.0f;
        stopTimer();
        if (pendingHighlightOff) {
            pendingHighlightOff = false;
            isHighlighted = false;
        }
    }
    repaint();
}

void Node::setDisplayMode(NodeDisplayMode mode)
{
    this->mode = mode;

    if (nodeValueTree.isValid()) {
        countEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::CountLimit);
    }

    switch (mode) {

        case NodeDisplayMode::Pitch:
            nodeTextEditor->bindEditor(midiNoteData, ValueTreeIdentifiers::MidiPitch);
            break;

        case NodeDisplayMode::Velocity:
            nodeTextEditor->bindEditor(midiNoteData, ValueTreeIdentifiers::MidiVelocity);
            break;

        case NodeDisplayMode::CountLimit:
            nodeTextEditor->bindEditor(nodeValueTree, ValueTreeIdentifiers::CountLimit);
            break;

        case NodeDisplayMode::Channel:
            nodeTextEditor->bindEditor(midiNoteData, ValueTreeIdentifiers::MidiChannel);
            break;

        default:
            break;
    }

    nodeTextEditor->formatDisplay(mode);
}

void Node::incrementNodeTextEditorValue(int incrementValue) {
    double editorValue = nodeTextEditor->bindValue.toString().getDoubleValue();
    editorValue += incrementValue;

    nodeTextEditor->bindValue.setValue(editorValue);
    nodeTextEditor->formatDisplay(nodeTextEditor->mode);
}
