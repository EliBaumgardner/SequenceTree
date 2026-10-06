#include "TraversalFlagNode.h"
#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Util/ApplicationContext.h"
#include "../Theme/CustomLookAndFeel.h"

#include <cmath>

TraversalFlagNode::TraversalFlagNode(const ApplicationContext& context)
    : Node(context),
      traversalNumEditor(context)
{
    nodeType = NodeType::TraversalFlag;

    traversalNumEditor.setFormat(std::make_unique<TraversalFlagFormat>());
    traversalNumEditor.setInterceptsMouseClicks(false, true);
    traversalNumEditor.setTooltip("Type +N to spawn traversal N, -N to remove it; +Na targets instance a");

    traversalNumEditor.boundValue.setValue(0);

    traversalNumEditor.onValueChange = [this]() {
        const int typeId = std::abs(static_cast<int>(traversalNumEditor.boundValue.getValue()));

        if (typeId > 0) {
            applicationContext.graphState->traversals.addTraversalData(typeId, nullptr);
        }
    };

    countEditor.setVisible(true);
    switchCountEditor.setVisible(true);
    subLoopLimitEditor.setVisible(false);
    nodeValueEditor.setVisible(false);

    addAndMakeVisible(traversalNumEditor);
}

void TraversalFlagNode::paint(juce::Graphics& graphics)
{
    const Theme& theme       = CustomLookAndFeel::get(*this);
    const auto   bounds      = getLocalBounds().toFloat();
    juce::Path   triangle    = buildTrianglePath();
    float        pulseScale  = 1.0f;
    juce::Colour fillColour  = nodeColour;
    juce::Colour outline     = theme.borderStrongColour;

    if (isHighlighted) {
        pulseScale = 1.0f + 0.1f * std::sin(pulsePhase * juce::MathConstants<float>::pi);
        fillColour = nodeColour.brighter(Theme::hoverFillBrightness);
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
    graphics.strokePath(triangle, juce::PathStrokeType(Theme::selectionRingWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void TraversalFlagNode::resized()
{
    const auto  bounds         = getLocalBounds().toFloat();
    const float centreX        = bounds.getCentreX();
    const float centreY        = bounds.getCentreY();
    const float bladeLength    = (juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f - 4.0f) * 0.7f;
    const int   numEditorSize  = juce::roundToInt(bladeLength * 0.7f);
    const auto  triangleBounds = buildTrianglePath().getBounds();
    const int   editorWidth    = juce::roundToInt(bladeLength * 0.45f);
    const int   editorHeight   = juce::roundToInt(bladeLength * 0.30f);
    juce::Point<float> numEditorCentre(centreX + bladeLength / 3.0f, centreY);

    numEditorCentre.applyTransform(juce::AffineTransform::rotation(incomingAngle + juce::MathConstants<float>::halfPi, centreX, centreY));

    traversalNumEditor.setBounds(juce::Rectangle<int>(0, 0, numEditorSize, numEditorSize).withCentre(numEditorCentre.roundToInt()));

    countEditor.setBounds(juce::roundToInt(triangleBounds.getRight()) - editorWidth, juce::roundToInt(triangleBounds.getY()), editorWidth, editorHeight);

    switchCountEditor.setBounds(juce::roundToInt(triangleBounds.getRight()) - editorWidth, juce::roundToInt(triangleBounds.getBottom()) - editorHeight,
                                editorWidth, editorHeight);
}

juce::Path TraversalFlagNode::buildTrianglePath() const
{
    const auto  bounds         = getLocalBounds().toFloat();
    const float centreX        = bounds.getCentreX();
    const float centreY        = bounds.getCentreY();
    const float bladeLength    = (juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f - 4.0f) * 0.7f;
    const float baseHalfHeight = bladeLength * 0.5f;
    juce::Path  triangle;

    triangle.startNewSubPath(centreX + bladeLength, centreY);
    triangle.lineTo(centreX, centreY + baseHalfHeight);
    triangle.lineTo(centreX, centreY - baseHalfHeight);
    triangle.closeSubPath();
    triangle.applyTransform(juce::AffineTransform::rotation(incomingAngle + juce::MathConstants<float>::halfPi, centreX, centreY));

    return triangle;
}

bool TraversalFlagNode::hitTest(int x, int y)
{
    const juce::Point<int> point(x, y);

    for (const ValueEditor* editor : { &countEditor, &switchCountEditor, &nodeValueEditor }) {
        if (editor->isVisible() && editor->getBounds().contains(point)) {
            return true;
        }
    }

    return buildTrianglePath().contains(static_cast<float>(x), static_cast<float>(y));
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
