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

    if (nodeType == NodeType::TraversalFlag) {
        for (auto& [childId, arrow] : nodeArrows) {
            if (arrow != nullptr) {
                arrow->sourceHovered = isHovered;
                arrow->refreshHoverVisibility();
            }
        }
    }

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

void Node::setHighlightVisual(int traversalId, bool shouldHighlight, juce::Colour colour)
{
    if (shouldHighlight) {
        pendingHighlightOffIds.erase(traversalId);
        activeHighlights[traversalId] = colour;
        pulsePhase = 0.0f;
        startTimerHz(60);
    }
    else if (traversalId == -1) {
        if (isTimerRunning()) {
            for (const auto& entry : activeHighlights) {
                pendingHighlightOffIds.insert(entry.first);
            }
        }
        else {
            activeHighlights.clear();
        }
    }
    else if (isTimerRunning()) {
        pendingHighlightOffIds.insert(traversalId);
    }
    else {
        activeHighlights.erase(traversalId);
    }

    isHighlighted = ! activeHighlights.empty();
    repaint();
}

void Node::timerCallback()
{
    pulsePhase += 0.07f;

    if (pulsePhase >= 1.0f) {
        pulsePhase = 1.0f;
        stopTimer();

        for (int id : pendingHighlightOffIds) {
            activeHighlights.erase(id);
        }
        pendingHighlightOffIds.clear();
        isHighlighted = ! activeHighlights.empty();
    }

    repaint();
}

void Node::setDisplayMode(NodeDisplayMode mode)
{
    this->mode = mode;

    if (nodeValueTree.isValid()) {
        countEditor       .bindEditor(nodeValueTree, ValueTreeIdentifiers::CountLimit);
        switchCountEditor .bindEditor(nodeValueTree, ValueTreeIdentifiers::SwitchCountLimit);

        juce::Identifier subLoopProperty = (nodeType == NodeType::Root)
            ? ValueTreeIdentifiers::LoopLimit
            : ValueTreeIdentifiers::SubLoopCountLimit;

        subLoopLimitEditor.bindEditor(nodeValueTree, subLoopProperty);
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

    const bool pitchMode = (mode == NodeDisplayMode::Pitch);
    nodeTextEditor->setReadOnly(pitchMode);
    nodeTextEditor->setCaretVisible(! pitchMode);

    nodeTextEditor->formatDisplay(mode);
}

void Node::incrementNodeTextEditorValue(int incrementValue) {
    double editorValue = nodeTextEditor->bindValue.toString().getDoubleValue();
    editorValue += incrementValue;

    nodeTextEditor->bindValue.setValue(editorValue);
    nodeTextEditor->formatDisplay(nodeTextEditor->mode);
}
