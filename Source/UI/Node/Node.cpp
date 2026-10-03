#include "Node.h"
#include "Arrow.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"
#include "../../Graph/ValueTreeIdentifiers.h"

#include <algorithm>
#include <cmath>

const juce::Colour Node::defaultNodeColour = juce::Colour::fromRGB(195, 174, 132).darker().darker().darker();

Node::Node(const ApplicationContext& context)
    : nodeValueEditor(context),
      countEditor(context),
      switchCountEditor(context),
      subLoopLimitEditor(context),
      applicationContext(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    upButton.painter = [this](juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState&) {
        CustomLookAndFeel::get(*this).drawIncrementIcon(graphics, bounds, true);
    };

    downButton.painter = [this](juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState&) {
        CustomLookAndFeel::get(*this).drawIncrementIcon(graphics, bounds, false);
    };

    nodeValueEditor.autoFitText       = true;
    nodeValueEditor.autoFitInsetRatio = nodeValueTextInsetRatio;

    nodeValueEditor.setFormat(std::make_unique<PitchFormat>());
    countEditor.setFormat(std::make_unique<DualIntFormat>(minimumCountLimit, maximumCountLimit, ValueTreeIdentifiers::TriggerLimit));
    subLoopLimitEditor.setFormat(std::make_unique<NumberFormat>(minimumSubLoopLimit, maximumSubLoopLimit));
    switchCountEditor.setFormat(std::make_unique<NumberFormat>(minimumCountLimit, maximumCountLimit));

    nodeValueEditor.editable = false;

    nodeValueEditor.bindEditor(midiNoteData, ValueTreeIdentifiers::MidiPitch);

    countEditor.setTooltip("Count Limit");
    subLoopLimitEditor.setTooltip("Sub Loop Count Limit");
    switchCountEditor.setTooltip("Switch Count Limit");

    upButton.setInterceptsMouseClicks(true, false);
    downButton.setInterceptsMouseClicks(true, false);
    nodeValueEditor.setInterceptsMouseClicks(false, false);
    countEditor.setInterceptsMouseClicks(true, false);
    switchCountEditor.setInterceptsMouseClicks(true, false);

    upButton.onClick   = [this]() { incrementNodeValue(1); };
    downButton.onClick = [this]() { incrementNodeValue(-1); };

    addAndMakeVisible(upButton);
    addAndMakeVisible(downButton);
    addAndMakeVisible(nodeValueEditor);
    addAndMakeVisible(switchCountEditor);
    addAndMakeVisible(countEditor);
    addAndMakeVisible(subLoopLimitEditor);
}

void Node::paint(juce::Graphics& graphics)
{
    CustomLookAndFeel::get(*this).drawNode(graphics, getNodeVisual(getLocalBounds().toFloat()));
}

void Node::resized()
{
    const juce::Rectangle<int> nodeSquare = getLocalBounds().withTrimmedLeft(interiorLeftInset);

    auto editorArea = CustomLookAndFeel::getNodeCircleBounds(nodeSquare.toFloat()).toNearestInt()
                          .reduced(editorAreaBoundsReduction);

    const int buttonHeight = juce::jmax(2, juce::roundToInt(editorArea.getHeight() * incrementButtonHeightFactor));
    const int editorWidth  = static_cast<int>(nodeSquare.getWidth()  * nodeEditorWidthFactor);
    const int editorHeight = static_cast<int>(nodeSquare.getHeight() * nodeEditorHeightFactor);

    upButton.setBounds(editorArea.removeFromTop(buttonHeight));
    downButton.setBounds(editorArea.removeFromBottom(buttonHeight));

    nodeValueEditor.setBounds(editorArea);

    countEditor.setBounds(nodeSquare.getRight() - editorWidth, nodeSquare.getY(), editorWidth, editorHeight);
    switchCountEditor.setBounds(nodeSquare.getRight() - editorWidth, nodeSquare.getBottom() - editorHeight, editorWidth, editorHeight);
    subLoopLimitEditor.setBounds(nodeSquare.getX(), nodeSquare.getBottom() - editorHeight, editorWidth, editorHeight);
}

void Node::incrementNodeValue(int incrementValue)
{
    const double currentValue = static_cast<double>(nodeValueEditor.boundValue.getValue());

    if (applicationContext.undoManager != nullptr) {
        applicationContext.undoManager->beginNewTransaction();
    }

    nodeValueEditor.setNumericValue(currentValue + incrementValue);

    refreshValueDisplay();
}

void Node::refreshValueDisplay()
{
    nodeValueEditor.repaint();
    repaint();
}

NodeVisual Node::getNodeVisual(juce::Rectangle<float> bounds) const
{
    return { bounds, nodeColour, activeHighlights, isHovered, isSelected, isOutlined,
             isEncapsulationRinged, hasInnerRim, encapsulationRingColour };
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

void Node::setSelectVisual()
{
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

    lastPulseFrameSec  = frameSec;
    pulsePhase        += static_cast<float>(elapsedSec) * pulseRatePerSecond;

    repaint();

    if (pulsePhase >= 1.0f) {
        pulsePhase        = 1.0f;
        lastPulseFrameSec = 0.0;

        for (int pendingId : pendingHighlightOffIds) {
            activeHighlights.erase(pendingId);
        }

        pendingHighlightOffIds.clear();

        isHighlighted = ! activeHighlights.empty();

        pulseFrames = {};
    }
}

juce::Point<int> Node::getNodeCentre() const
{
    return getBounds().getCentre();
}

float Node::getVisualRadius() const
{
    return getHeight() * 0.5f;
}

float Node::getBodyExtent(juce::Point<float> approachDirection) const
{
    const juce::Rectangle<float> circle    = CustomLookAndFeel::getNodeCircleBounds(getLocalBounds().toFloat());
    const float                  radius    = std::max(0.0f, circle.getWidth() * 0.5f);
    const juce::Point<float>     toCircle  = circle.getCentre() - (getNodeCentre() - getPosition()).toFloat();
    const float                  along     = approachDirection.getDotProduct(toCircle);
    const float                  clearance = along * along - toCircle.getDistanceSquaredFromOrigin() + radius * radius;

    if (clearance <= 0.0f) {
        return radius;
    }

    return std::max(0.0f, std::sqrt(clearance) - along);
}

void Node::bindToTree()
{
    if (! nodeValueTree.isValid()) {
        return;
    }

    countEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::CountLimit);
    switchCountEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::SwitchCountLimit);

    if (isAlternativeNode) {
        subLoopLimitEditor.setVisible(false);
        return;
    }

    juce::Identifier subLoopProperty = ValueTreeIdentifiers::SubLoopCountLimit;

    if (nodeValueTree.getType() == ValueTreeIdentifiers::RootNodeData) {
        subLoopProperty = ValueTreeIdentifiers::LoopLimit;
    }

    subLoopLimitEditor.bindEditor(nodeValueTree, subLoopProperty);
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
            nodeValueEditor.setFormat(std::make_unique<DualIntFormat>(minimumCountLimit, maximumCountLimit, ValueTreeIdentifiers::TriggerLimit));
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

void Node::setDisplayMode(NodeDisplayMode newMode)
{
    mode = newMode;

    bindValueEditorForMode();

    nodeValueEditor.repaint();
    repaint();
}
