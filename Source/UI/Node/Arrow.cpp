/*
  ==============================================================================

    Arrow.cpp
    Created: 12 Jun 2025 12:45:57am
    Author:  Eli Baumgardner

  ==============================================================================
*/
#include "../../Graph/ValueTreeState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "Arrow.h"

#include "Node.h"
#include "../Theme/CustomLookAndFeel.h"

Arrow::Arrow(Node* startNode, Node* endNode, ApplicationContext& context)
    : startNode(startNode), endNode(endNode)
{
    setLookAndFeel(context.lookAndFeel);
    bindValue.addListener(this);
}

Arrow::Arrow(Node* startNode, juce::Point<int> tipOffset, ApplicationContext& context)
    : startNode(startNode), tipOffset(tipOffset)
{
    setLookAndFeel(context.lookAndFeel);
    setInterceptsMouseClicks(false, true);
    bindValue.addListener(this);

    valueEditor = std::make_unique<ValueEditor>(context);
    addAndMakeVisible(*valueEditor);
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

    if (isTraversalArrow()) {
        return false;
    }

    return endNode != nullptr
        && endNode->nodeType == NodeType::Root
        && ! startNode->isAlternativeNode;
}

ArrowInfo Arrow::getArrowInfo() const
{
    return ValueTreeState::readArrowInfo(arrowTree,
                                         startNode != nullptr && startNode->isAlternativeNode);
}

bool Arrow::isTraversalArrow() const
{
    if (isDangling() || ! arrowTree.isValid()) {
        return false;
    }

    return getArrowInfo().type == ArrowType::Traversal;
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

    return arrowDurationFromDelta(getArrowInfo(), delta.x, delta.y);
}

juce::String Arrow::getDurationLabel() const
{
    if (startNode == nullptr) {
        return "0";
    }

    const ArrowInfo arrowInfo = getArrowInfo();

    if (! arrowBindsTo(arrowInfo, ArrowBinding::DurationBind)
        && arrowBindsTo(arrowInfo, ArrowBinding::PitchBind)) {

        if (isDangling()) {
            return juce::String((int) startNode->midiNoteData.getProperty(ValueTreeIdentifiers::MidiPitch,
                                                                          arrowDefaultBasePitch));
        }

        const Node* const parentNode = startNode->isAlternativeNode ? endNode : startNode;

        const juce::Point<int> delta = startNode->isAlternativeNode
                                     ? startNode->getNodeCentre() - getTip()
                                     : getTip() - startNode->getNodeCentre();

        const int basePitch = parentNode->midiNoteData.getProperty(ValueTreeIdentifiers::MidiPitch,
                                                                   arrowDefaultBasePitch);

        return juce::String(arrowPitchFromDelta(arrowInfo, basePitch, delta.x, delta.y));
    }

    const int duration = getDuration();

    if (startNode->nodeType == NodeType::Modulator) {
        return juce::String(duration / 10) + "%";
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

    return geometry;
}

juce::Path Arrow::buildShaftPath(ArrowGeometry& geometry, float headLength, juce::Point<float> origin) const
{
    juce::Path path;

    if (! geometry.valid) {
        return path;
    }

    const juce::Point<float> start = geometry.start - origin;
    const juce::Point<float> tip   = geometry.tip   - origin;
    const juce::Point<float> shaft = tip - start;

    if (geometry.straight) {
        juce::Point<float> shaftEnd = tip;

        if (geometry.drawHead) {
            shaftEnd = tip - geometry.direction * headLength;
        }

        path.startNewSubPath(start);
        path.lineTo(shaftEnd);
        return path;
    }

    float sign = -1.0f;

    if (shaft.x >= 0.0f) {
        sign = 1.0f;
    }


    const juce::Point<float> perpendicular { -geometry.direction.y * curvePerpScale * sign,
                                              geometry.direction.x * curvePerpScale * sign };

    const float offset = shaft.getDistanceFromOrigin() * curveOffsetFactor;

    const juce::Point<float> control1 = start + shaft * 0.33f + perpendicular * offset;
    const juce::Point<float> control2 = start + shaft * 0.67f - perpendicular * offset;

    const juce::Point<float> neck    = tip - control2;
    const float              neckLen = neck.getDistanceFromOrigin();

    if (neckLen > 0.0f) {
        geometry.direction = neck / neckLen;
    }

    juce::Point<float> shaftEnd = tip;

    if (geometry.drawHead) {
        shaftEnd = tip - geometry.direction * headLength;
    }

    path.startNewSubPath(start);
    path.cubicTo(control1, control2, shaftEnd);
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

    ArrowGeometry geometry = getGeometry(1.0f);

    if (! geometry.valid) {
        const juce::Point<int> centre = startNode->getNodeCentre();
        setBounds(juce::Rectangle<int>(centre, centre).expanded(arrowBoundsPadding));
        repaint();
        return;
    }

    const juce::Path shaft = buildShaftPath(geometry, 0.0f, {});

    setBounds(shaft.getBounds().expanded((float)arrowBoundsPadding).toNearestInt());

    if (valueEditor != nullptr) {
        valueEditor->setBounds(juce::Rectangle<int>(0, 0, valueEditorWidth, valueEditorHeight)
                                   .withCentre(getTip() - getPosition()));
    }

    repaint();
}

void Arrow::setTipOffset(juce::Point<int> offset)
{
    tipOffset = offset;
    setArrowBounds();
}

void Arrow::bindToProperty(juce::ValueTree tree, const juce::Identifier propertyID) {
  boundNodeValueTree = tree;
  bindValue.referTo(tree.getPropertyAsValue(propertyID,nullptr));
}

void Arrow::valueChanged(juce::Value&) {
}

void Arrow::updateBoundProperty(int boundValue) {
  bindValue.setValue(boundValue);
}

void Arrow::triggerSnapAnimation()
{
    animT        = 0.0f;
    animVelocity = 0.0f;
    ensureAnimationTimerRunning();
}

void Arrow::setHoverFade(bool shouldBeVisible)
{
    if (shouldBeVisible) {
        hoverAlphaTarget = 1.0f;
    } else {
        hoverAlphaTarget = 0.0f;
    }


    if (shouldBeVisible && ! isVisible()) {
        setVisible(true);
    }

    ensureAnimationTimerRunning();
}

void Arrow::initHoverState(bool visibleNow)
{
    if (visibleNow) {
        hoverAlpha = 1.0f;
    } else {
        hoverAlpha = 0.0f;
    }

    hoverAlphaTarget = hoverAlpha;
    setAlpha(hoverAlpha);
    setVisible(visibleNow);
}

bool Arrow::advanceHoverFade()
{
    if (hoverAlphaTarget < hoverAlpha && ! isSnapSettled()) {
        return false;
    }

    if (std::abs(hoverAlpha - hoverAlphaTarget) < hoverFadeEpsilon) {
        if (hoverAlpha != hoverAlphaTarget) {
            hoverAlpha = hoverAlphaTarget;
            setAlpha(hoverAlpha);
        }
        if (hoverAlphaTarget <= 0.0f && isVisible()) {
            setVisible(false);
        }
        return true;
    }

    float step = -hoverFadeStep;

    if (hoverAlpha < hoverAlphaTarget) {
        step = hoverFadeStep;
    }

    hoverAlpha = juce::jlimit(0.0f, 1.0f, hoverAlpha + step);
    setAlpha(hoverAlpha);
    return false;
}

void Arrow::startProgress(int traversalId, int durationMs, juce::Colour colour, bool oneShot)
{
    if (connectsTraversalFlag()) {
        return;
    }

    progress.start(traversalId, durationMs, colour, oneShot);
    ensureAnimationTimerRunning();
    repaint();
}

void Arrow::resetProgress()
{
    progress.reset();
    repaint();
}

void Arrow::resetProgress(int traversalId)
{
    progress.reset(traversalId);
    repaint();
}

void Arrow::ensureAnimationTimerRunning()
{
    if (! isTimerRunning()) {
        startTimerHz(animationTimerHz);
    }
}

bool Arrow::isSnapSettled() const
{
    return std::abs(animT - 1.0f) < snapSettledEpsilon
        && std::abs(animVelocity) < snapSettledEpsilon;
}

bool Arrow::advanceSnapAnimation()
{
    if (isSnapSettled()) {
        return true;
    }

    animVelocity += (1.0f - animT) * snapSpringStiffness;
    animVelocity *= snapSpringDamping;
    animT        += animVelocity;

    if (isSnapSettled()) {
        animT        = 1.0f;
        animVelocity = 0.0f;
        return true;
    }
    return false;
}

void Arrow::timerCallback()
{
    const bool snapDone       = advanceSnapAnimation();
    const bool progressActive = progress.advance();
    const bool hoverDone      = advanceHoverFade();

    if (snapDone && ! progressActive && hoverDone) {
        stopTimer();
    }

    repaint();
}
