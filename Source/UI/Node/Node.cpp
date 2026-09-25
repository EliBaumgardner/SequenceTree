 /*
  ==============================================================================

    Node.cpp
    Created: 6 May 2025 8:37:02pm
    Author:  Eli Baumgardner

  ==============================================================================
*/
#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"
#include "Arrow.h"

#include "Node.h"

#include <algorithm>
#include <cmath>
#include <limits>




Node::Node(const ApplicationContext& context)
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
    nodeValueEditor.autoFitText       = true;
    nodeValueEditor.autoFitInsetRatio = nodeValueTextInsetRatio;
    nodeValueEditor.setFormat(std::make_unique<PitchFormat>());
    nodeValueEditor.editable = false;
    nodeValueEditor.bindEditor(midiNoteData, ValueTreeIdentifiers::MidiPitch);
    nodeValueEditor.toBack();

    countEditor.setInterceptsMouseClicks(true, false);
    countEditor.setTooltip("Count Limit");
    countEditor.setFormat(std::make_unique<DualIntFormat>(minimumCountLimit, maximumCountLimit,
                                                          ValueTreeIdentifiers::TriggerLimit));

    subLoopLimitEditor.setFormat(std::make_unique<NumberFormat>(minimumSubLoopLimit, maximumSubLoopLimit));
    subLoopLimitEditor.setTooltip("Sub Loop Count Limit");

    switchCountEditor.setInterceptsMouseClicks(true, false);
    switchCountEditor.setFormat(std::make_unique<NumberFormat>(minimumCountLimit, maximumCountLimit));
    switchCountEditor.setTooltip("Switch Count Limit");

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
    layoutInterior(getLocalBounds());
}

float Node::getBodyExtent(juce::Point<float> approachDirection) const
{
    const juce::Rectangle<float> circle = CustomLookAndFeel::getNodeCircleBounds(getLocalBounds().toFloat());

    const float radius = std::max(0.0f, circle.getWidth() * 0.5f);

    const juce::Point<float> toCircle = circle.getCentre() - (getNodeCentre() - getPosition()).toFloat();

    const float along     = approachDirection.getDotProduct(toCircle);
    const float clearance = along * along - toCircle.getDistanceSquaredFromOrigin() + radius * radius;

    if (clearance <= 0.0f) {
        return radius;
    }

    return std::max(0.0f, std::sqrt(clearance) - along);
}

void Node::layoutInterior(juce::Rectangle<int> nodeSquare)
{
    auto editorArea = CustomLookAndFeel::getNodeCircleBounds(nodeSquare.toFloat()).toNearestInt()
                          .reduced(editorAreaBoundsReduction);

    const int buttonHeight = juce::jmax(2, juce::roundToInt(editorArea.getHeight() * incrementButtonHeightFactor));

    upButton->setBounds(editorArea.removeFromTop(buttonHeight));
    downButton->setBounds(editorArea.removeFromBottom(buttonHeight));

    nodeValueEditor.setBounds(editorArea);

    const int editorWidth  = static_cast<int>(nodeSquare.getWidth()  * nodeEditorWidthFactor);
    const int editorHeight = static_cast<int>(nodeSquare.getHeight() * nodeEditorHeightFactor);

    countEditor.setBounds(nodeSquare.getRight() - editorWidth, nodeSquare.getY(), editorWidth, editorHeight);
    switchCountEditor.setBounds(nodeSquare.getRight() - editorWidth, nodeSquare.getBottom() - editorHeight,
                                editorWidth, editorHeight);
    subLoopLimitEditor.setBounds(nodeSquare.getX(), nodeSquare.getBottom() - editorHeight, editorWidth, editorHeight);
}

void Node::setHoverVisual(bool isHovered)
{
    this->isHovered = isHovered;

    if (nodeType == NodeType::TraversalFlag) {
        for (auto& [childId, arrow] : nodeArrows) {
            if (arrow != nullptr) {
                arrow->sourceHovered = isHovered;
                arrow->setHoverFade(arrow->sourceHovered || arrow->proximityHovered);
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

void Node::setHighlightVisual(int runId, bool shouldHighlight, juce::Colour colour)
{
    if (shouldHighlight) {
        for (int pendingId : pendingHighlightOffIds) {
            activeHighlights.erase(pendingId);
        }
        pendingHighlightOffIds.clear();

        activeHighlights[runId] = colour;
        pulsePhase = 0.0f;

        if (pulseFrames.isEmpty()) {
            pulseFrames = juce::VBlankAttachment(this, [this](double frameSec) { advancePulse(frameSec); });
        }
    }
    else if (runId == -1) {
        if (! pulseFrames.isEmpty()) {
            for (const auto& entry : activeHighlights) {
                pendingHighlightOffIds.insert(entry.first);
            }
        }
        else {
            activeHighlights.clear();
        }
    }
    else if (! pulseFrames.isEmpty()) {
        pendingHighlightOffIds.insert(runId);
    }
    else {
        activeHighlights.erase(runId);
    }

    isHighlighted = ! activeHighlights.empty();
    repaint();
}

void Node::advancePulse(double frameSec)
{
    double elapsedSec = 0.0;

    if (lastPulseFrameSec > 0.0) {
        elapsedSec = frameSec - lastPulseFrameSec;
    }

    lastPulseFrameSec = frameSec;
    pulsePhase += static_cast<float>(elapsedSec) * pulseRatePerSecond;

    repaint();

    if (pulsePhase >= 1.0f) {
        pulsePhase        = 1.0f;
        lastPulseFrameSec = 0.0;

        for (int id : pendingHighlightOffIds) {
            activeHighlights.erase(id);
        }
        pendingHighlightOffIds.clear();
        isHighlighted = ! activeHighlights.empty();

        pulseFrames = {};
    }
}

void Node::bindToTree()
{
    if (! nodeValueTree.isValid()) {
        return;
    }

    countEditor       .bindEditor(nodeValueTree, ValueTreeIdentifiers::CountLimit);
    switchCountEditor .bindEditor(nodeValueTree, ValueTreeIdentifiers::SwitchCountLimit);

    juce::Identifier subLoopProperty = ValueTreeIdentifiers::SubLoopCountLimit;

    if (nodeValueTree.getType() == ValueTreeIdentifiers::RootNodeData) {
        subLoopProperty = ValueTreeIdentifiers::LoopLimit;
    }

    subLoopLimitEditor.bindEditor(nodeValueTree, subLoopProperty);
}

void Node::setDisplayMode(NodeDisplayMode newMode)
{
    mode = newMode;

    bindValueEditorForMode();

    nodeValueEditor.repaint();
    repaint();
}

void Node::bindValueEditorForMode()
{
    nodeValueEditor.editable = true;

    switch (mode) {

        case NodeDisplayMode::Pitch:
            nodeValueEditor.setFormat(std::make_unique<PitchFormat>());
            nodeValueEditor.bindEditor(midiNoteData, ValueTreeIdentifiers::MidiPitch);
            nodeValueEditor.editable = false;
            break;

        case NodeDisplayMode::Velocity:
            nodeValueEditor.setFormat(std::make_unique<NumberFormat>(minimumMidiVelocity, maximumMidiVelocity));
            nodeValueEditor.bindEditor(midiNoteData, ValueTreeIdentifiers::MidiVelocity);
            break;

        case NodeDisplayMode::CountLimit:
            nodeValueEditor.setFormat(std::make_unique<DualIntFormat>(minimumCountLimit, maximumCountLimit,
                                                                      ValueTreeIdentifiers::TriggerLimit));
            nodeValueEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::CountLimit);
            break;

        case NodeDisplayMode::Channel:
            nodeValueEditor.setFormat(std::make_unique<NumberFormat>(minimumMidiChannel, maximumMidiChannel));
            nodeValueEditor.bindEditor(midiNoteData, ValueTreeIdentifiers::MidiChannel);
            break;

        case NodeDisplayMode::RepeatValue: {
            auto repeatFormat = std::make_unique<NumberFormat>(minimumRepeatValue, maximumRepeatValue);
            repeatFormat->prefix = "x";

            nodeValueEditor.setFormat(std::move(repeatFormat));
            nodeValueEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::RepeatValue);
            break;
        }

        case NodeDisplayMode::Probability: {
            auto probabilityFormat = std::make_unique<NumberFormat>(minimumProbability, maximumProbability);
            probabilityFormat->suffix = "%";

            nodeValueEditor.setFormat(std::move(probabilityFormat));
            nodeValueEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::Probability);
            break;
        }

        case NodeDisplayMode::Duration:
            nodeValueEditor.setFormat(std::make_unique<NumberFormat>(0.0, ArrowInfo::maximumDurationMs));
            nodeValueEditor.editable = false;
            break;
    }
}

void Node::incrementNodeValue(int incrementValue) {
    const double currentValue = static_cast<double>(nodeValueEditor.boundValue.getValue());

    if (applicationContext.undoManager != nullptr) {
        applicationContext.undoManager->beginNewTransaction();
    }

    nodeValueEditor.setNumericValue(currentValue + incrementValue);
    refreshValueDisplay();
}

void Node::refreshValueDisplay() {
    nodeValueEditor.repaint();
    repaint();
}
