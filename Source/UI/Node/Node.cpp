/*
  ==============================================================================

    Node.cpp
    Created: 6 May 2025 8:37:02pm
    Author:  Eli Baumgardner

  ==============================================================================
*/
#include "../../Graph/ValueTreeState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"
#include "../../Graph/RTGraphBuilder.h"
#include "Arrow.h"

#include "Node.h"




Node::Node(ApplicationContext& context)
    : applicationContext(context),
      nodeValueEditor(context), countEditor(context), switchCountEditor(context), subLoopLimitEditor(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    upButton = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState&) {
            CustomLookAndFeel::get(*this).drawIncrementIcon(g, bounds, true);
        });

    downButton = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState&) {
            CustomLookAndFeel::get(*this).drawIncrementIcon(g, bounds, false);
        });

    upButton->setInterceptsMouseClicks(true,false);

    downButton->setInterceptsMouseClicks(true,false);

    nodeValueEditor.setInterceptsMouseClicks(false, false);
    nodeValueEditor.enableAutoFitText();
    nodeValueEditor.setPitchMode(true);
    nodeValueEditor.setEditable(false);
    nodeValueEditor.setMinimumValue(0);
    nodeValueEditor.bindEditor(midiNoteData, ValueTreeIdentifiers::MidiPitch);
    nodeValueEditor.toBack();

    countEditor.setInterceptsMouseClicks(true, false);
    countEditor.setTooltip("Count Limit");
    countEditor.enableDualValue(ValueTreeIdentifiers::TriggerLimit);

    subLoopLimitEditor.setMinimumValue(0);

    switchCountEditor.setInterceptsMouseClicks(true, false);
    switchCountEditor.setTooltip("Loop Limit");

    upButton->onClick = [this]() {
        incrementNodeValue(1);
    };

    downButton->onClick = [this]() {
        incrementNodeValue(-1);
    };

    addAndMakeVisible(upButton.get());
    addAndMakeVisible(downButton.get());
    addAndMakeVisible(nodeValueEditor);

    addAndMakeVisible(switchCountEditor);
    addAndMakeVisible(countEditor);
    addAndMakeVisible(subLoopLimitEditor);
}

void Node::paint(juce::Graphics& g)
{
    CustomLookAndFeel::get(*this).drawNode(g, getNodeVisual());
}

void Node::resized()
{
    auto circleBounds = CustomLookAndFeel::getNodeCircleBounds(getLocalBounds().toFloat()).toNearestInt();
    auto editorArea   = circleBounds.reduced(editorAreaBoundsReduction);
    int  buttonHeight = juce::jmax(2, (int)(editorArea.getHeight() * 0.2f));

    upButton->setBounds(editorArea.removeFromTop(buttonHeight));
    downButton->setBounds(editorArea.removeFromBottom(buttonHeight));

    nodeValueEditor.setBounds(editorArea);

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
        for (int pendingId : pendingHighlightOffIds) {
            activeHighlights.erase(pendingId);
        }
        pendingHighlightOffIds.clear();

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

    if (mode == NodeDisplayMode::CountLimit) {
        nodeValueEditor.enableDualValue(ValueTreeIdentifiers::TriggerLimit);
    }
    else {
        nodeValueEditor.disableDualValue();
    }

    switch (mode) {

        case NodeDisplayMode::Pitch:
            nodeValueEditor.bindEditor(midiNoteData, ValueTreeIdentifiers::MidiPitch);
            break;

        case NodeDisplayMode::Velocity:
            nodeValueEditor.bindEditor(midiNoteData, ValueTreeIdentifiers::MidiVelocity);
            break;

        case NodeDisplayMode::CountLimit:
            nodeValueEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::CountLimit);
            break;

        case NodeDisplayMode::Channel:
            nodeValueEditor.bindEditor(midiNoteData, ValueTreeIdentifiers::MidiChannel);
            break;

        case NodeDisplayMode::RepeatValue:
            nodeValueEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::RepeatValue);
            break;

        default:
            break;
    }

    const bool pitchMode = (mode == NodeDisplayMode::Pitch);
    nodeValueEditor.setPitchMode(pitchMode);
    nodeValueEditor.setEditable(! pitchMode);
    nodeValueEditor.setMinimumValue(mode == NodeDisplayMode::Channel || mode == NodeDisplayMode::RepeatValue ? 1 : 0);

    nodeValueEditor.repaint();
    repaint();
}

void Node::incrementNodeValue(int incrementValue) {
    double editorValue = nodeValueEditor.boundValue.toString().getDoubleValue();
    editorValue += incrementValue;

    nodeValueEditor.boundValue.setValue(editorValue);
    refreshValueDisplay();
}

void Node::refreshValueDisplay() {
    nodeValueEditor.repaint();
    repaint();

    if (nodeValueTree.isValid() && applicationContext.rtGraphBuilder != nullptr) {
        applicationContext.rtGraphBuilder->makeRTGraph(nodeValueTree);
    }
}
