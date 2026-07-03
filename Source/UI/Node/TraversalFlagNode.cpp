//
// Created by Eli Baumgardner on 6/30/26.
//

#include "TraversalFlagNode.h"
#include "NodeTextEditor.h"

#include <cmath>

TraversalFlagNode::TraversalFlagNode(ApplicationContext& context) : Node(context)
{
    nodeType = NodeType::TraversalFlag;

    countEditor.setVisible(true);
    switchCountEditor.setVisible(true);

    subLoopLimitEditor.setVisible(false);
    upButton.setVisible(false);
    downButton.setVisible(false);

    traversalNumEditor = std::make_unique<ValueEditor>(context);
    addAndMakeVisible(traversalNumEditor.get());

    if (nodeTextEditor != nullptr) {
        nodeTextEditor->setVisible(false);
    }
}

void TraversalFlagNode::resized() {

    auto bounds = getLocalBounds().toFloat();
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();

    float half = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    float bladeLength =  (half - 4.0f) * 0.7f;

    juce::Point<float> centre(cx + bladeLength / 3.0f, cy);
    centre.applyTransform(juce::AffineTransform::rotation(incomingAngle + juce::MathConstants<float>::halfPi,
                                                          cx, cy));

    int size = juce::roundToInt(bladeLength * 0.7f);
    traversalNumEditor->setBounds(juce::Rectangle<int>(0, 0, size, size).withCentre(centre.roundToInt()));

    auto triangleBounds = buildTrianglePath().getBounds();

    int editorWidth  = juce::roundToInt(bladeLength * 0.45f);
    int editorHeight = juce::roundToInt(bladeLength * 0.30f);

    countEditor.setBounds(juce::roundToInt(triangleBounds.getRight()) - editorWidth,
                          juce::roundToInt(triangleBounds.getY()),
                          editorWidth, editorHeight);

    switchCountEditor.setBounds(juce::roundToInt(triangleBounds.getRight()) - editorWidth,
                                juce::roundToInt(triangleBounds.getBottom()) - editorHeight,
                                editorWidth, editorHeight);
}

juce::Path TraversalFlagNode::buildTrianglePath() const
{
    auto bounds = getLocalBounds().toFloat();
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();

    float half = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    float bladeLength =  (half - 4.0f) * 0.7f;

    float baseHalfHeight  = bladeLength * 0.5f;

    juce::Path triangle;
    triangle.startNewSubPath(cx + bladeLength, cy);
    triangle.lineTo(cx, cy + baseHalfHeight);
    triangle.lineTo(cx, cy - baseHalfHeight);
    triangle.closeSubPath();

    triangle.applyTransform(juce::AffineTransform::rotation(incomingAngle + juce::MathConstants<float>::halfPi,
                                                            cx, cy));
    return triangle;
}

bool TraversalFlagNode::hitTest(int x, int y)
{
    juce::Point<int> p(x, y);

    if (countEditor.isVisible() && countEditor.getBounds().contains(p)) {
        return true;
    }
    if (switchCountEditor.isVisible() && switchCountEditor.getBounds().contains(p)) {
        return true;
    }
    if (nodeTextEditor != nullptr && nodeTextEditor->isVisible() && nodeTextEditor->getBounds().contains(p)) {
        return true;
    }

    return buildTrianglePath().contains((float) x, (float) y);
}

void TraversalFlagNode::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    juce::Path triangle = buildTrianglePath();

    if (isHighlighted) {
        float pulseScale = 1.0f + 0.1f * std::sin(pulsePhase * juce::MathConstants<float>::pi);
        triangle.applyTransform(juce::AffineTransform::scale(pulseScale, pulseScale,
                                                             bounds.getCentreX(),
                                                             bounds.getCentreY()));
    }

    if (isHighlighted) {
        g.setColour(nodeColour.darker());
    }
    else {
        g.setColour(nodeColour);
    }
    g.fillPath(triangle);

    g.setColour(outlineColour);
    g.strokePath(triangle, juce::PathStrokeType(1.0f));

    if (isHovered) {
        g.strokePath(triangle, juce::PathStrokeType(2.0f));
    }

    if (isSelected) {
        juce::Path dottedPath = triangle;

        juce::PathStrokeType stroke(0.325f);
        float dashLengths[] = { 2.935f, 2.935f };
        stroke.createDashedStroke(dottedPath, dottedPath, dashLengths, 2);

        g.setColour(juce::Colours::black);
        g.strokePath(dottedPath, stroke);
    }
}
