/*
  ==============================================================================

    Arrow.cpp
    Created: 12 Jun 2025 12:45:57am
    Author:  Eli Baumgardner

  ==============================================================================
*/
#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "Arrow.h"

#include "Node.h"
#include "../Theme/CustomLookAndFeel.h"

Arrow::Arrow(Node* startNode, Node* endNode, ApplicationContext& context)
    : startNode(startNode), endNode(endNode)
{
    setLookAndFeel(context.lookAndFeel);

    createDurationEditor(context);
}

Arrow::Arrow(Node* startNode, juce::Point<int> tipOffset, ApplicationContext& context)
    : startNode(startNode), tipOffset(tipOffset)
{
    setLookAndFeel(context.lookAndFeel);
    setInterceptsMouseClicks(false, true);

    valueEditor = std::make_unique<ValueEditor>(context);
    valueEditor->setInterceptsMouseClicks(true, false);
    valueEditor->setTooltip("Count Limit");
    addAndMakeVisible(*valueEditor);

    createDurationEditor(context);
}

void Arrow::createDurationEditor(ApplicationContext& context)
{
    durationEditor = std::make_unique<ValueEditor>(context);
    durationEditor->setInterceptsMouseClicks(true, false);
    durationEditor->setTooltip("Arrow Duration");

    durationEditor->onEditFinished = [this] {
        editingDuration = false;
        durationEditor->setVisible(false);
        repaint();
    };

    addChildComponent(*durationEditor);
}

void Arrow::paint(juce::Graphics &g) {
  CustomLookAndFeel::get(*this).drawArrow(g, *this);
}

juce::Point<int> Arrow::getTip() const
{
    if (endNode != nullptr) {
        return endNode->getNodeCentre();
    }

    if (startNode == nullptr) {
        return tipOffset;
    }

    return startNode->getNodeCentre() + tipOffset;
}

bool Arrow::isDashed() const
{
    if (dashed || isGhost) {
        return true;
    }

    if (startNode == nullptr) {
        return false;
    }

    if (startNode->nodeType == NodeType::TraversalFlag) {
        return true;
    }

    if (endNode == nullptr || endNode->nodeType != NodeType::Root || startNode->isAlternativeNode) {
        return false;
    }

    const ArrowType arrowType = ArrowBindingOps::getArrowInfo(arrowTree).type;

    return arrowType != ArrowType::Traversal && arrowType != ArrowType::StepIntoTree;
}

bool Arrow::isTraversalArrow() const
{
    if (isDangling() || ! arrowTree.isValid()) {
        return false;
    }

    return ArrowBindingOps::getArrowInfo(arrowTree).type == ArrowType::Traversal;
}

bool Arrow::isSyncArrow() const
{
    return ArrowBindingOps::getArrowInfo(arrowTree).isSynced;
}

bool Arrow::connectsTraversalFlag() const
{
    return (startNode != nullptr && startNode->nodeType == NodeType::TraversalFlag)
        || (endNode   != nullptr && endNode->nodeType   == NodeType::TraversalFlag);
}

int Arrow::getDuration() const
{
    if (startNode == nullptr) {
        return 0;
    }

    const juce::Point<int> delta = getTip() - startNode->getNodeCentre();

    return ArrowInfo::durationFromDelta(ArrowBindingOps::getArrowInfo(arrowTree), delta.x, delta.y);
}

bool Arrow::showsDurationLabel() const
{
    const ArrowInfo arrowInfo = ArrowBindingOps::getArrowInfo(arrowTree);

    if (ArrowInfo::bindsTo(arrowInfo, ArrowBinding::DurationBind)) {
        return true;
    }

    return ! ArrowInfo::bindsTo(arrowInfo, ArrowBinding::PitchBind);
}

juce::String Arrow::getDurationLabel() const
{
    if (startNode == nullptr) {
        return "0";
    }

    if (! showsDurationLabel()) {

        const Node* pitchedNode = endNode;

        if (isDangling() || startNode->isAlternativeNode) {
            pitchedNode = startNode;
        }

        if (pitchedNode != nullptr) {
            return juce::String((int) pitchedNode->midiNoteData.getProperty(ValueTreeIdentifiers::MidiPitch,
                                                                            defaultMidiPitch));
        }
    }

    const int duration = getDuration();

    if (startNode->nodeType == NodeType::Modulator) {
        return juce::String(duration / ArrowDurationFormat::millisecondsPerPercent) + "%";
    }

    return juce::String(duration);
}

juce::Point<float> Arrow::getHeadAnchor() const
{
    if (isDangling()) {
        return getTip().toFloat();
    }

    const ArrowGeometry geometry = getGeometry(1.0f);
    if (! geometry.valid) {
        return getTip().toFloat();
    }

    const float endRadius = endNode->getVisualRadius();
    return endNode->getNodeCentre().toFloat() - geometry.chord * (endRadius + headAnchorInset);
}

ArrowGeometry Arrow::getGeometry(float animationT) const
{
    ArrowGeometry geometry;

    if (startNode == nullptr) {
        return geometry;
    }

    const juce::Point<float> centre = startNode->getNodeCentre().toFloat();
    const juce::Point<float> target = getTip().toFloat();

    const juce::Point<float> delta = (target - centre) * animationT;
    const float length = delta.getDistanceFromOrigin();

    if (length < 1.0f) {
        return geometry;
    }

    const juce::Point<float> direction = delta / length;

    juce::Point<float> start = centre;
    juce::Point<float> tip   = centre + delta;

    const bool endIsTraversalFlag = endNode != nullptr && endNode->nodeType == NodeType::TraversalFlag;

    if (isDangling()) {
        start += direction * startNode->getVisualRadius();
    }
    else if (endIsTraversalFlag) {
        const float endHalf        = std::min(endNode->getWidth(), endNode->getHeight()) * 0.5f;
        const float baseHalfHeight = (endHalf - 4.0f) * 0.7f * 0.5f;
        tip += direction * baseHalfHeight;
    }
    else {
        tip -= direction * endNode->getVisualRadius();
    }

    const juce::Point<float> shaft = tip - start;
    const float shaftLength = shaft.getDistanceFromOrigin();

    geometry.centre    = centre;
    geometry.start     = start;
    geometry.tip       = tip;
    geometry.direction = direction;
    geometry.chord     = direction;

    if (shaftLength > 0.0f) {
        geometry.chord = shaft / shaftLength;
    }

    geometry.length    = length;
    geometry.drawHead  = ! endIsTraversalFlag && animationT > headVisibleThreshold;
    geometry.straight  = isDangling() || endIsTraversalFlag
                      || std::abs(shaft.x) < 1.0f || std::abs(shaft.y) < 1.0f;
    geometry.valid     = true;

    if (geometry.straight) {
        return geometry;
    }

    float sign = -1.0f;

    if (shaft.x >= 0.0f) {
        sign = 1.0f;
    }

    const juce::Point<float> perpendicular { -direction.y * curvePerpScale * sign,
                                              direction.x * curvePerpScale * sign };

    const float offset = shaftLength * curveOffsetFactor;

    geometry.control1 = start + shaft * 0.33f + perpendicular * offset;
    geometry.control2 = start + shaft * 0.67f - perpendicular * offset;

    const juce::Point<float> neck       = tip - geometry.control2;
    const float              neckLength = neck.getDistanceFromOrigin();

    if (neckLength > 0.0f) {
        geometry.direction = neck / neckLength;
    }

    return geometry;
}

ArrowLabel Arrow::getLabel(const ArrowGeometry& geometry, float headLength) const
{
    ArrowLabel label;

    if (startNode == nullptr) {
        return label;
    }

    const juce::Point<float> delta = geometry.tip - geometry.centre;

    juce::Point<float> shaftStart = geometry.centre;
    juce::Point<float> shaftEnd   = geometry.tip;

    const float length = delta.getDistanceFromOrigin();

    if (length > 0.0f) {
        const juce::Point<float> unit = delta / length;

        shaftStart += unit * startNode->getVisualRadius();
        shaftEnd   -= unit * headLength;
    }

    label.centre = (shaftStart + shaftEnd) * 0.5f;

    float angle = std::atan2(delta.y, delta.x);

    const float halfPi = juce::MathConstants<float>::halfPi;

    while (angle > halfPi) {
        angle -= juce::MathConstants<float>::pi;
    }

    while (angle <= -halfPi) {
        angle += juce::MathConstants<float>::pi;
    }

    if (std::abs(delta.x) < std::abs(delta.y) * verticalLabelThreshold) {
        angle = 0.0f;
    }

    label.angle = angle;

    return label;
}

juce::Path Arrow::buildShaftPath(const ArrowGeometry& geometry, float headLength, juce::Point<float> origin) const
{
    juce::Path path;

    if (! geometry.valid) {
        return path;
    }

    const juce::Point<float> start = geometry.start - origin;
    const juce::Point<float> tip   = geometry.tip   - origin;

    juce::Point<float> shaftEnd = tip;

    if (geometry.drawHead) {
        shaftEnd = tip - geometry.direction * headLength;
    }

    path.startNewSubPath(start);

    if (geometry.straight) {
        path.lineTo(shaftEnd);
        return path;
    }

    path.cubicTo(geometry.control1 - origin, geometry.control2 - origin, shaftEnd);
    return path;
}

void Arrow::setArrowBounds()
{
    if (startNode == nullptr) {
        return;
    }

    if (endNode != nullptr) {
        const juce::Point<int> delta = getTip() - startNode->getNodeCentre();
        endNode->incomingAngle = std::atan2((float)delta.y, (float)delta.x);

        if (endNode->nodeType == NodeType::TraversalFlag) {
            endNode->resized();
            endNode->repaint();
        }
    }

    const ArrowGeometry geometry = getGeometry(1.0f);

    if (! geometry.valid) {
        const juce::Point<int> centre = startNode->getNodeCentre();
        setBounds(juce::Rectangle<int>(centre, centre).expanded(arrowBoundsPadding));
        repaint();
        return;
    }

    const juce::Path shaft = buildShaftPath(geometry, 0.0f, {});

    setBounds(shaft.getBounds().expanded((float)arrowBoundsPadding).toNearestInt());

    repaint();
}

void Arrow::resized()
{
    const ArrowGeometry geometry = getGeometry(1.0f);

    if (geometry.valid) {
        const ArrowLabel label = getLabel(geometry, arrowHeadLength);

        const juce::Point<int> editorCentre = (label.centre - getPosition().toFloat()).roundToInt()
                                            - juce::Point<int>(0, valueEditorHeight / 2);

        durationEditor->setBounds(juce::Rectangle<int>(0, 0, valueEditorWidth, valueEditorHeight)
                                      .withCentre(editorCentre));
    }

    if (valueEditor == nullptr) {
        return;
    }

    valueEditor->setBounds(juce::Rectangle<int>(0, 0, valueEditorWidth, valueEditorHeight)
                               .withCentre(getTip() - getPosition()));
}

void Arrow::beginDurationEdit()
{
    if (startNode == nullptr || ! arrowTree.isValid() || ! showsDurationLabel()) {
        return;
    }

    durationEditor->setFormat(std::make_unique<ArrowDurationFormat>(startNode->nodeType == NodeType::Modulator));

    const int durationOverride = arrowTree.getProperty(ValueTreeIdentifiers::ArrowDuration,
                                                       ArrowInfo::noDurationOverride);

    if (durationOverride == ArrowInfo::noDurationOverride) {
        arrowTree.setProperty(ValueTreeIdentifiers::ArrowDuration, getDuration(), nullptr);
    }

    durationEditor->bindEditor(arrowTree, ValueTreeIdentifiers::ArrowDuration);

    editingDuration = true;

    durationEditor->setVisible(true);
    durationEditor->beginEditing();

    repaint();
}

void Arrow::setTipOffset(juce::Point<int> offset)
{
    tipOffset = offset;
    setArrowBounds();
}

void Arrow::triggerSnapAnimation()
{
    animation.snapT        = 0.0f;
    animation.snapVelocity = 0.0f;

    if (! isTimerRunning()) {
        startTimerHz(ArrowAnimation::tickRateHz);
    }
}

void Arrow::setHoverFade(bool shouldBeVisible)
{
    animation.alphaTarget = 0.0f;

    if (shouldBeVisible) {
        animation.alphaTarget = 1.0f;
    }

    if (shouldBeVisible && ! isVisible()) {
        setVisible(true);
    }

    if (! isTimerRunning()) {
        startTimerHz(ArrowAnimation::tickRateHz);
    }
}

void Arrow::initHoverState(bool visibleNow)
{
    animation.alpha = 0.0f;

    if (visibleNow) {
        animation.alpha = 1.0f;
    }

    animation.alphaTarget = animation.alpha;

    setAlpha(animation.alpha);
    setVisible(visibleNow);
}

void Arrow::startProgress(int trailId, int durationMs, juce::Colour colour, bool oneShot)
{
    animation.startTrail(trailId, durationMs, colour, oneShot);

    if (! isTimerRunning()) {
        startTimerHz(ArrowAnimation::tickRateHz);
    }

    repaint();
}

void Arrow::resetProgress()
{
    if (animation.trails.empty()) {
        return;
    }

    animation.trails.clear();
    repaint();
}

void Arrow::resetProgress(int trailId)
{
    if (animation.trails.erase(trailId) > 0) {
        repaint();
    }
}

void Arrow::timerCallback()
{
    const bool stillAnimating = animation.advance();

    setAlpha(animation.alpha);

    if (animation.alpha <= 0.0f && isVisible()) {
        setVisible(false);
    }

    if (! stillAnimating) {
        stopTimer();
    }

    repaint();
}
