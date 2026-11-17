#include "Node.h"
#include "Processor/Document/TypedProperties.h"
#include "Arrow.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "Editor/Canvas/NodeCanvas.h"
#include "Processor/Document/ValueTreeIdentifiers.h"

Node::Node(juce::UndoManager& undoManager)
    : ArsNovaUI::CanvasNode(undoManager),
      countEditor          (undoManager),
      switchCountEditor    (undoManager),
      subLoopLimitEditor   (undoManager)
{
    nodeValueEditor.autoFitText       = true;
    nodeValueEditor.autoFitInsetRatio = nodeValueTextInsetRatio;
    nodeValueEditor.fontStyle         = ArsNovaUI::Theme::FontStyle::SemiBold;

    countEditor       .backdrop       = ArsNovaUI::ValueEditor::Backdrop::Badge;
    switchCountEditor .backdrop       = ArsNovaUI::ValueEditor::Backdrop::Badge;
    subLoopLimitEditor.backdrop       = ArsNovaUI::ValueEditor::Backdrop::Badge;

    nodeValueEditor   .setFormat(std::make_unique<ArsNovaUI::PitchFormat>());
    countEditor       .setFormat(std::make_unique<ArsNovaUI::DualIntFormat>(NodeInfo::minimumCountLimit, NodeInfo::maximumCountLimit, ValueTreeIdentifiers::TriggerLimit));
    subLoopLimitEditor.setFormat(std::make_unique<ArsNovaUI::NumberFormat>(NodeInfo::minimumSubLoopLimit, NodeInfo::maximumSubLoopLimit));
    switchCountEditor .setFormat(std::make_unique<ArsNovaUI::NumberFormat>(NodeInfo::minimumCountLimit, NodeInfo::maximumCountLimit));

    nodeValueEditor.editable = false;

    nodeValueEditor.bindEditor(midiNoteData, ValueTreeIdentifiers::MidiPitch);

    setTooltip("Node");
    countEditor       .setTooltip("Count Limit");
    subLoopLimitEditor.setTooltip("Sub Loop Count Limit");
    switchCountEditor .setTooltip("Switch Count Limit");

    nodeValueEditor   .setInterceptsMouseClicks(false, true);
    countEditor       .setInterceptsMouseClicks(false, true);
    switchCountEditor. setInterceptsMouseClicks(false, true);
    subLoopLimitEditor.setInterceptsMouseClicks(false, true);

    addAndMakeVisible(nodeValueEditor);
    addAndMakeVisible(switchCountEditor);
    addAndMakeVisible(countEditor);
    addAndMakeVisible(subLoopLimitEditor);

    addMouseListener(this, true);
}

void Node::paint(juce::Graphics& graphics)
{
    const ArsNovaUI::NodeVisual visual = getNodeVisual(getLocalBounds().toFloat());

    ArsNovaUI::CustomLookAndFeel& lookAndFeel = ArsNovaUI::CustomLookAndFeel::get(*this);

    lookAndFeel.drawCircleNode(graphics, visual);

    if (showIncrementButtons) {
        lookAndFeel.drawIncrementIcon(graphics, incrementUpBounds.toFloat(), true);
        lookAndFeel.drawIncrementIcon(graphics, incrementDownBounds.toFloat(), false);
    }
}

void Node::resized()
{
    const juce::Rectangle<int> nodeSquare = getLocalBounds().withTrimmedLeft(interiorLeftInset);

    auto editorArea = ArsNovaUI::CustomLookAndFeel::getNodeCircleBounds(nodeSquare.toFloat()).toNearestInt()
                          .reduced(editorAreaBoundsReduction);

    const int minimumButtonHeight = 2;
    const int scaledButtonHeight  = juce::roundToInt(editorArea.getHeight() * incrementButtonHeightFactor);
    const int buttonHeight        = juce::jmax(minimumButtonHeight, scaledButtonHeight);
    const int editorWidth         = static_cast<int>(nodeSquare.getWidth()  * nodeEditorWidthFactor);
    const int editorHeight        = static_cast<int>(nodeSquare.getHeight() * nodeEditorHeightFactor);
    const int rightEditorX        = nodeSquare.getRight() - editorWidth;
    const int lowerEditorY        = nodeSquare.getBottom() - editorHeight;

    incrementUpBounds   = editorArea.removeFromTop(buttonHeight);
    incrementDownBounds = editorArea.removeFromBottom(buttonHeight);

    nodeValueEditor.setBounds(editorArea);
    countEditor.setBounds(rightEditorX, nodeSquare.getY(), editorWidth, editorHeight);
    switchCountEditor.setBounds(rightEditorX, lowerEditorY, editorWidth, editorHeight);
    subLoopLimitEditor.setBounds(nodeSquare.getX(), lowerEditorY, editorWidth, editorHeight);
}

void Node::incrementNodeValue(int incrementValue)
{
    const double currentValue     = nodeValueEditor.boundValue.getValue();
    const double incrementedValue = currentValue + incrementValue;

    undoManager.beginNewTransaction();

    nodeValueEditor.setNumericValue(incrementedValue);

    refreshValueDisplay();
}

void Node::setDisplayMode(NodeInfo::DisplayMode newMode)
{
    mode = newMode;

    bindValueEditorForMode();

    nodeValueEditor.repaint();
    repaint();
}

void Node::setHoverVisual(bool isHovered)
{
    ArsNovaUI::CanvasNode::setHoverVisual(isHovered);

    if (nodeType == NodeInfo::Type::TraversalFlag) {
        for (auto& [childId, arrow] : nodeArrows) {
            if (arrow != nullptr) {
                Arrow* const flagArrow = static_cast<Arrow*>(arrow);

                flagArrow->sourceHovered = isHovered;

                flagArrow->setHoverFade(flagArrow->sourceHovered || flagArrow->proximityHovered);
            }
        }
    }
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

        pulseFrames.start(frameClock, [this](double frameSec) { advancePulse(frameSec); });
    }
    else if (runId == -1) {
        if (pulseFrames.isActive()) {
            for (const auto& entry : activeHighlights) {
                pendingHighlightOffIds.insert(entry.first);
            }
        }
        else {
            activeHighlights.clear();
        }
    }
    else if (pulseFrames.isActive()) {
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

        pulseFrames.stop();
    }
}

void Node::mouseMove(const juce::MouseEvent& event)
{
    const bool shouldShow = nodeValueEditor.isVisible() && contains(event.getEventRelativeTo(this).getPosition()) && ! event.mods.isShiftDown();

    if (shouldShow != showIncrementButtons) {
        showIncrementButtons = shouldShow;

        repaint();
    }
}

void Node::mouseExit(const juce::MouseEvent& event)
{
    const bool shouldShow = nodeValueEditor.isVisible() && contains(event.getEventRelativeTo(this).getPosition()) && ! event.mods.isShiftDown();

    if (shouldShow != showIncrementButtons) {
        showIncrementButtons = shouldShow;

        repaint();
    }
}

void Node::lookAndFeelChanged()
{
    const juce::String defaultColourText = ArsNovaUI::CustomLookAndFeel::get(*this).defaultNodeColour.toString();

    const NodeProperties properties(nodeValueTree);

    juce::String colourText = properties.nodeColour;

    if (properties.nodeColour.isUsingDefault()) {
        colourText = defaultColourText;
    }

    nodeColour = juce::Colour::fromString(colourText);
}

void Node::bindToTree()
{
    if (! nodeValueTree.isValid()) {
        return;
    }

    countEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::CountLimit);
    switchCountEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::SwitchCountLimit);

    if (arrowPointsToParent) {
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
        case NodeInfo::DisplayMode::Pitch:
            nodeValueEditor.setFormat(std::make_unique<ArsNovaUI::PitchFormat>());
            nodeValueEditor.bindEditor(midiNoteData, ValueTreeIdentifiers::MidiPitch);
            nodeValueEditor.setTooltip("Pitch");
            break;

        case NodeInfo::DisplayMode::Velocity:
            nodeValueEditor.setFormat(std::make_unique<ArsNovaUI::NumberFormat>(NodeInfo::minimumMidiVelocity, NodeInfo::maximumMidiVelocity));
            nodeValueEditor.bindEditor(midiNoteData, ValueTreeIdentifiers::MidiVelocity);
            nodeValueEditor.setTooltip("Velocity");
            break;

        case NodeInfo::DisplayMode::CountLimit:
            nodeValueEditor.setFormat(std::make_unique<ArsNovaUI::DualIntFormat>(NodeInfo::minimumCountLimit, NodeInfo::maximumCountLimit, ValueTreeIdentifiers::TriggerLimit));
            nodeValueEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::CountLimit);
            nodeValueEditor.setTooltip("Count Limit");
            break;

        case NodeInfo::DisplayMode::Channel:
            nodeValueEditor.setFormat(std::make_unique<ArsNovaUI::NumberFormat>(NodeInfo::minimumMidiChannel, NodeInfo::maximumMidiChannel));
            nodeValueEditor.bindEditor(midiNoteData, ValueTreeIdentifiers::MidiChannel);
            nodeValueEditor.setTooltip("MIDI Channel");
            break;

        case NodeInfo::DisplayMode::RepeatValue: {
            auto repeatFormat = std::make_unique<ArsNovaUI::NumberFormat>(NodeInfo::minimumRepeatValue, NodeInfo::maximumRepeatValue);

            repeatFormat->prefix = "x";

            nodeValueEditor.setFormat(std::move(repeatFormat));
            nodeValueEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::RepeatValue);
            nodeValueEditor.setTooltip("Repeat");
            break;
        }

        case NodeInfo::DisplayMode::Probability: {
            auto probabilityFormat = std::make_unique<ArsNovaUI::NumberFormat>(NodeInfo::minimumProbability, NodeInfo::maximumProbability);

            probabilityFormat->suffix = "%";

            nodeValueEditor.setFormat(std::move(probabilityFormat));
            nodeValueEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::Probability);
            nodeValueEditor.setTooltip("Probability");
            break;
        }

        case NodeInfo::DisplayMode::Duration:
            nodeValueEditor.setFormat(std::make_unique<ArsNovaUI::NumberFormat>(0.0, ArrowInfo::maximumDurationMs));
            nodeValueEditor.setTooltip("Duration");
            nodeValueEditor.editable = false;
            break;
    }
}

void Node::respondToClick(juce::Point<int> localPoint)
{
    for (ArsNovaUI::ValueEditor* badge : { &countEditor, &switchCountEditor, &subLoopLimitEditor }) {
        if (badge->isVisible() && badge->getBounds().contains(localPoint)) {
            badge->beginEditing();
            return;
        }
    }

    if (showIncrementButtons && incrementUpBounds.contains(localPoint)) {
        incrementNodeValue(1);
        return;
    }

    if (showIncrementButtons && incrementDownBounds.contains(localPoint)) {
        incrementNodeValue(-1);
        return;
    }

    ArsNovaUI::CanvasNode::respondToClick(localPoint);
}

juce::String Node::getTooltip()
{
    const juce::Point<int> mousePoint = getMouseXYRelative();

    for (juce::Component* child : getChildren()) {
        auto* const editor = dynamic_cast<ArsNovaUI::ValueEditor*>(child);

        if (editor != nullptr && editor->isVisible() && editor->getBounds().contains(mousePoint) && editor->getTooltip().isNotEmpty()) {
            return editor->getTooltip();
        }
    }

    return juce::SettableTooltipClient::getTooltip();
}

ArsNovaUI::NodeVisual Node::getNodeVisual(juce::Rectangle<float> bounds) const
{
    return { bounds, nodeColour, activeHighlights, isHovered, isSelected, isOutlined,
             isEncapsulationRinged, hasInnerRim, encapsulationRingColour };
}
