#include "Node.h"
#include "Arrow.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../Canvas/NodeCanvas.h"
#include "../../Graph/ValueTreeIdentifiers.h"

#include <algorithm>
#include <cmath>

Node::Node(juce::UndoManager& undoManager)
    : nodeValueEditor(undoManager),
      countEditor(undoManager),
      switchCountEditor(undoManager),
      subLoopLimitEditor(undoManager),
      undoManager(undoManager)
{
    upButton.painter = [this](juce::Graphics& graphics, juce::Rectangle<float> bounds, const ArsNovaUI::ButtonState&) {
        ArsNovaUI::CustomLookAndFeel::get(*this).drawIncrementIcon(graphics, bounds, true);
    };

    downButton.painter = [this](juce::Graphics& graphics, juce::Rectangle<float> bounds, const ArsNovaUI::ButtonState&) {
        ArsNovaUI::CustomLookAndFeel::get(*this).drawIncrementIcon(graphics, bounds, false);
    };

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

    countEditor       .setTooltip("Count Limit");
    subLoopLimitEditor.setTooltip("Sub Loop Count Limit");
    switchCountEditor .setTooltip("Switch Count Limit");

    upButton          .setInterceptsMouseClicks(false, false);
    downButton        .setInterceptsMouseClicks(false, false);
    nodeValueEditor   .setInterceptsMouseClicks(false, true);
    countEditor       .setInterceptsMouseClicks(false, true);
    switchCountEditor. setInterceptsMouseClicks(false, true);
    subLoopLimitEditor.setInterceptsMouseClicks(false, true);

    addChildComponent(upButton);
    addChildComponent(downButton);

    addAndMakeVisible(nodeValueEditor);
    addAndMakeVisible(switchCountEditor);
    addAndMakeVisible(countEditor);
    addAndMakeVisible(subLoopLimitEditor);

    addMouseListener(this, true);
}

void Node::paint(juce::Graphics& graphics)
{
    ArsNovaUI::CustomLookAndFeel::get(*this).drawCircleNode(graphics, getNodeVisual(getLocalBounds().toFloat()));
}

void Node::resized()
{
    const juce::Rectangle<int> nodeSquare = getLocalBounds().withTrimmedLeft(interiorLeftInset);

    auto editorArea = ArsNovaUI::CustomLookAndFeel::getNodeCircleBounds(nodeSquare.toFloat()).toNearestInt()
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

    undoManager.beginNewTransaction();

    nodeValueEditor.setNumericValue(currentValue + incrementValue);

    refreshValueDisplay();
}

void Node::refreshValueDisplay()
{
    nodeValueEditor.repaint();
    repaint();
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
    this->isHovered = isHovered;

    if (nodeType == NodeInfo::Type::TraversalFlag) {
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

void Node::mouseMove(const juce::MouseEvent& event)
{
    const bool showIncrementButtons = nodeValueEditor.isVisible() && contains(event.getEventRelativeTo(this).getPosition()) && ! event.mods.isShiftDown();

    upButton.setVisible(showIncrementButtons);
    downButton.setVisible(showIncrementButtons);
}

void Node::mouseExit(const juce::MouseEvent& event)
{
    const bool showIncrementButtons = nodeValueEditor.isVisible() && contains(event.getEventRelativeTo(this).getPosition()) && ! event.mods.isShiftDown();

    upButton.setVisible(showIncrementButtons);
    downButton.setVisible(showIncrementButtons);
}

void Node::lookAndFeelChanged()
{
    const juce::String defaultColourText = ArsNovaUI::CustomLookAndFeel::get(*this).defaultNodeColour.toString();

    nodeColour = juce::Colour::fromString(nodeValueTree.getProperty(ValueTreeIdentifiers::NodeColour, defaultColourText).toString());
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
        case NodeInfo::DisplayMode::Pitch:
            nodeValueEditor.setFormat(std::make_unique<ArsNovaUI::PitchFormat>());
            nodeValueEditor.bindEditor(midiNoteData, ValueTreeIdentifiers::MidiPitch);
            break;

        case NodeInfo::DisplayMode::Velocity:
            nodeValueEditor.setFormat(std::make_unique<ArsNovaUI::NumberFormat>(NodeInfo::minimumMidiVelocity, NodeInfo::maximumMidiVelocity));
            nodeValueEditor.bindEditor(midiNoteData, ValueTreeIdentifiers::MidiVelocity);
            break;

        case NodeInfo::DisplayMode::CountLimit:
            nodeValueEditor.setFormat(std::make_unique<ArsNovaUI::DualIntFormat>(NodeInfo::minimumCountLimit, NodeInfo::maximumCountLimit, ValueTreeIdentifiers::TriggerLimit));
            nodeValueEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::CountLimit);
            break;

        case NodeInfo::DisplayMode::Channel:
            nodeValueEditor.setFormat(std::make_unique<ArsNovaUI::NumberFormat>(NodeInfo::minimumMidiChannel, NodeInfo::maximumMidiChannel));
            nodeValueEditor.bindEditor(midiNoteData, ValueTreeIdentifiers::MidiChannel);
            break;

        case NodeInfo::DisplayMode::RepeatValue: {
            auto repeatFormat = std::make_unique<ArsNovaUI::NumberFormat>(NodeInfo::minimumRepeatValue, NodeInfo::maximumRepeatValue);

            repeatFormat->prefix = "x";

            nodeValueEditor.setFormat(std::move(repeatFormat));
            nodeValueEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::RepeatValue);
            break;
        }

        case NodeInfo::DisplayMode::Probability: {
            auto probabilityFormat = std::make_unique<ArsNovaUI::NumberFormat>(NodeInfo::minimumProbability, NodeInfo::maximumProbability);

            probabilityFormat->suffix = "%";

            nodeValueEditor.setFormat(std::move(probabilityFormat));
            nodeValueEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::Probability);
            break;
        }

        case NodeInfo::DisplayMode::Duration:
            nodeValueEditor.setFormat(std::make_unique<ArsNovaUI::NumberFormat>(0.0, ArrowInfo::maximumDurationMs));
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

    if (upButton.isVisible() && upButton.getBounds().contains(localPoint)) {
        incrementNodeValue(1);
        return;
    }

    if (downButton.isVisible() && downButton.getBounds().contains(localPoint)) {
        incrementNodeValue(-1);
        return;
    }

    if (nodeValueEditor.isVisible() && nodeValueEditor.getBounds().contains(localPoint)) {
        nodeValueEditor.beginEditing();
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
    const juce::Rectangle<float> circle    = ArsNovaUI::CustomLookAndFeel::getNodeCircleBounds(getLocalBounds().toFloat());
    const float                  radius    = std::max(0.0f, circle.getWidth() * 0.5f);
    const juce::Point<float>     toCircle  = circle.getCentre() - (getNodeCentre() - getPosition()).toFloat();
    const float                  along     = approachDirection.getDotProduct(toCircle);
    const float                  clearance = along * along - toCircle.getDistanceSquaredFromOrigin() + radius * radius;

    if (clearance <= 0.0f) {
        return radius;
    }

    return std::max(0.0f, std::sqrt(clearance) - along);
}

ArsNovaUI::NodeVisual Node::getNodeVisual(juce::Rectangle<float> bounds) const
{
    return { bounds, nodeColour, activeHighlights, isHovered, isSelected, isOutlined,
             isEncapsulationRinged, hasInnerRim, encapsulationRingColour };
}
