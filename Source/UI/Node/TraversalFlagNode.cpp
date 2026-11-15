#include "TraversalFlagNode.h"
#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../Editors/Formats/TraversalFlagFormat.h"

#include <cmath>

TraversalFlagNode::TraversalFlagNode(juce::UndoManager& undoManager, GraphState& graphState)
    : Node              (undoManager),
      traversalNumEditor(undoManager),
      graphState        (graphState)
{
    nodeType        = NodeInfo::Type::TraversalFlag;
    takesArrowHeads = false;
    connectsByDrag  = true;

    setTooltip("Traversal flag");

    traversalNumEditor.setFormat(std::make_unique<TraversalFlagFormat>());
    traversalNumEditor.setInterceptsMouseClicks(false, true);
    traversalNumEditor.setTooltip("Type +N to spawn traversal N, -N to remove it; +Na targets instance a");

    traversalNumEditor.boundValue.setValue(0);

    traversalNumEditor.onValueChange = [this]() {
        const int typedId = traversalNumEditor.boundValue.getValue();
        const int typeId  = std::abs(typedId);

        if (typeId > 0) {
            this->graphState.traversals.addTraversalData(typeId, nullptr);
        }
    };

    countEditor       .setVisible(true);
    switchCountEditor .setVisible(true);
    subLoopLimitEditor.setVisible(false);
    nodeValueEditor   .setVisible(false);

    addAndMakeVisible(traversalNumEditor);
}

void TraversalFlagNode::paint(juce::Graphics& graphics)
{
    const ArsNovaUI::Theme& theme = ArsNovaUI::CustomLookAndFeel::get(*this);

    const auto bounds = getLocalBounds().toFloat();

    juce::Path triangle = buildTrianglePath();

    float pulseScale = 1.0f;

    juce::Colour fillColour = nodeColour;
    juce::Colour outline    = theme.borderStrongColour;

    if (isHighlighted) {
        pulseScale = 1.0f + pulseDepth * std::sin(pulsePhase * juce::MathConstants<float>::pi);
        fillColour = nodeColour.brighter(ArsNovaUI::Theme::hoverFillBrightness);
    }

    if (isHovered) {
        outline = theme.hoverRingColour;
    }

    if (isSelected) {
        outline = theme.selectionRingColour;
    }

    triangle.applyTransform(juce::AffineTransform::scale(pulseScale, pulseScale, bounds.getCentreX(), bounds.getCentreY()));

    graphics.setColour(fillColour);
    graphics.fillPath(triangle);
    graphics.setColour(outline);
    graphics.strokePath(triangle, juce::PathStrokeType(ArsNovaUI::Theme::selectionRingWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void TraversalFlagNode::resized()
{
    const auto bounds         = getLocalBounds().toFloat();
    const auto triangleBounds = buildTrianglePath().getBounds();

    const float centreX         = bounds.getCentreX();
    const float centreY         = bounds.getCentreY();
    const float shortSide       = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const float bladeLength     = (shortSide * 0.5f - bladeInset) * bladeLengthRatio;
    const float numEditorOffset = bladeLength / 3.0f;
    const float bladeAngle      = incomingAngle + juce::MathConstants<float>::halfPi;

    const int numEditorSize  = juce::roundToInt(bladeLength * 0.7f);
    const int editorWidth    = juce::roundToInt(bladeLength * 0.45f);
    const int editorHeight   = juce::roundToInt(bladeLength * 0.30f);
    const int triangleRight  = juce::roundToInt(triangleBounds.getRight());
    const int triangleTop    = juce::roundToInt(triangleBounds.getY());
    const int triangleBottom = juce::roundToInt(triangleBounds.getBottom());
    const int editorX        = triangleRight - editorWidth;
    const int lowerEditorY   = triangleBottom - editorHeight;

    juce::Point<float> numEditorCentre(centreX + numEditorOffset, centreY);

    arrowTipReach = bladeLength * 0.5f;

    numEditorCentre.applyTransform(juce::AffineTransform::rotation(bladeAngle, centreX, centreY));

    const juce::Rectangle<int> numEditorBounds = juce::Rectangle<int>(numEditorSize, numEditorSize).withCentre(numEditorCentre.roundToInt());

    traversalNumEditor.setBounds(numEditorBounds);
    countEditor.setBounds(editorX, triangleTop, editorWidth, editorHeight);
    switchCountEditor.setBounds(editorX, lowerEditorY, editorWidth, editorHeight);
}

void TraversalFlagNode::bindToTree()
{
    Node::bindToTree();

    if (nodeValueTree.isValid()) {
        traversalNumEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::TraversalFlagValue);
    }
}

void TraversalFlagNode::respondToClick(juce::Point<int> localPoint)
{
    if (traversalNumEditor.getBounds().contains(localPoint)) {
        traversalNumEditor.beginEditing();

        return;
    }

    Node::respondToClick(localPoint);
}

void TraversalFlagNode::incomingAngleChanged()
{
    resized();

    repaint();
}

bool TraversalFlagNode::hitTest(int x, int y)
{
    const juce::Point<int>   point(x, y);
    const juce::Point<float> testPoint = point.toFloat();

    for (const ArsNovaUI::ValueEditor* editor : { &countEditor, &switchCountEditor, &nodeValueEditor }) {
        if (editor->isVisible() && editor->getBounds().contains(point)) {
            return true;
        }
    }

    return buildTrianglePath().contains(testPoint);
}

juce::Rectangle<int> TraversalFlagNode::getBoundsAt(juce::Point<int> position) const
{
    const int radius   = ArsNovaUI::Theme::nodeRadius;
    const int flagSide = radius * 4;

    return juce::Rectangle<int>(flagSide, flagSide).withCentre(position);
}

juce::Path TraversalFlagNode::buildTrianglePath() const
{
    const auto bounds = getLocalBounds().toFloat();

    const float centreX        = bounds.getCentreX();
    const float centreY        = bounds.getCentreY();
    const float shortSide      = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const float bladeLength    = (shortSide * 0.5f - bladeInset) * bladeLengthRatio;
    const float baseHalfHeight = bladeLength * 0.5f;
    const float bladeAngle     = incomingAngle + juce::MathConstants<float>::halfPi;

    const juce::Point<float> bladeTip   { centreX + bladeLength, centreY };
    const juce::Point<float> baseLower  { centreX, centreY + baseHalfHeight };
    const juce::Point<float> baseUpper  { centreX, centreY - baseHalfHeight };

    juce::Path triangle;

    triangle.startNewSubPath(bladeTip);
    triangle.lineTo(baseLower);
    triangle.lineTo(baseUpper);
    triangle.closeSubPath();
    triangle.applyTransform(juce::AffineTransform::rotation(bladeAngle, centreX, centreY));

    return triangle;
}
