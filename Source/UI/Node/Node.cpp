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




Node::Node(ApplicationContext& context) : applicationContext(context), countEditor(context), switchCountEditor(context), subLoopLimitEditor(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    upButton.setInterceptsMouseClicks(true,false);

    downButton.setInterceptsMouseClicks(true,false);

    nodeTextEditor = std::make_unique<NodeTextEditor>(this, context);
    nodeTextEditor->bindEditor(midiNoteData,ValueTreeIdentifiers::MidiPitch);
    nodeTextEditor->toBack();

    countEditor.setInterceptsMouseClicks(true, false);
    countEditor.setTooltip("Count Limit");
    countEditor.enableDualValue(ValueTreeIdentifiers::TriggerLimit);

    subLoopLimitEditor.setMinimumValue(0);

    switchCountEditor.setInterceptsMouseClicks(true, false);
    switchCountEditor.setTooltip("Loop Limit");

    upButton.onChanged = [this]() {
        incrementNodeTextEditorValue(1);
    };

    downButton.onChanged = [this]() {
        incrementNodeTextEditorValue(-1);
    };

    addAndMakeVisible(upButton);
    addAndMakeVisible(downButton);
    addAndMakeVisible(nodeTextEditor.get());

    addAndMakeVisible(switchCountEditor);
    addAndMakeVisible(countEditor);
    addAndMakeVisible(subLoopLimitEditor);
}

void Node::paint(juce::Graphics& g)
{
    CustomLookAndFeel::get(*this).drawNode(g, *this);
}

void Node::resized()
{
    auto circleBounds = CustomLookAndFeel::getNodeCircleBounds(getLocalBounds().toFloat()).toNearestInt();
    auto editorArea   = circleBounds.reduced(editorAreaBoundsReduction);
    int  buttonHeight = juce::jmax(2, (int)(editorArea.getHeight() * 0.2f));

    upButton.setBounds(editorArea.removeFromTop(buttonHeight));
    downButton.setBounds(editorArea.removeFromBottom(buttonHeight));

    nodeTextEditor->setBounds(editorArea);
    nodeTextEditor->setJustification(juce::Justification::centred);

    int editorWidth  = (int)(getWidth()  * nodeEditorWidthFactor);
    int editorHeight = (int)(getHeight() * nodeEditorHeightFactor);

    countEditor.setBounds(getWidth() - editorWidth, 0, editorWidth, editorHeight);
    switchCountEditor.setBounds(getWidth() - editorWidth, getHeight() - editorHeight, editorWidth, editorHeight);
    subLoopLimitEditor.setBounds(0, getHeight() - editorHeight, editorWidth, editorHeight);
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

void Node::setHighlightVisual(bool shouldHighlight)
{
    if (!shouldHighlight) {
        return;
    }

    isHighlighted = true;
    pulsePhase    = 0.0f;
    startTimerHz(60);
    repaint();
}

void Node::timerCallback()
{
    pulsePhase += 0.07f;

    if (pulsePhase >= 1.0f) {
        pulsePhase    = 1.0f;
        isHighlighted = false;
        stopTimer();
    }

    repaint();
}

void Node::setDisplayMode(NodeDisplayMode mode)
{
    this->mode = mode;

    if (nodeValueTree.isValid()) {
        countEditor       .bindEditor(nodeValueTree, ValueTreeIdentifiers::CountLimit);
        switchCountEditor .bindEditor(nodeValueTree, ValueTreeIdentifiers::SwitchCountLimit);
        subLoopLimitEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::SubLoopCountLimit);
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

        case NodeDisplayMode::RepeatValue:
            nodeTextEditor->bindEditor(nodeValueTree, ValueTreeIdentifiers::RepeatValue);
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
