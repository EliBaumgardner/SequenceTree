/*
  ==============================================================================

    NodeArrow.cpp
    Created: 12 Jun 2025 12:45:57am
    Author:  Eli Baimgardner

  ==============================================================================
*/
#include "../../Graph/ValueTreeState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "NodeArrow.h"

#include "Node.h"
#include "../Theme/CustomLookAndFeel.h"

NodeArrow::NodeArrow(Node* startNode, Node* endNode, ApplicationContext& context)
    : startNode(startNode), endNode(endNode)
{
    setLookAndFeel(context.lookAndFeel);
    bindValue.addListener(this);
}

void NodeArrow::paint(juce::Graphics &g) {
  CustomLookAndFeel::get(*this).drawNodeArrow(g, *this, textEditor);
}

void NodeArrow::setArrowBounds(Node* movedNode) {

  juce::Point<int> start = startNode->getNodeCentre();
  juce::Point<int> end   = endNode->getNodeCentre();

  float dx = float(end.x - start.x);
  float dy = float(end.y - start.y);

  length = std::sqrt(dx*dx + dy*dy);

  endNode->incomingAngle = std::atan2(dy, dx);

  if (endNode->nodeType == NodeType::TraversalFlag) {
      endNode->resized();
      endNode->repaint();
  }

  if (startNode->isAlternativeNode) {
      duration = (int)(std::abs(dy) * durationAmount);
  }
  else {
      duration = (int)(std::abs(dx) * durationAmount);
  }


  if (startNode->nodeType == NodeType::Modulator) {
      textEditor.setText(juce::String(duration / 10) + "%");
  }
  else {
      textEditor.setText(juce::String(duration));
  }

  int parentRadius = startNode->getHeight() / 2;
  int childRadius  = (endNode->nodeType == NodeType::TraversalFlag) ? 0 : endNode->getHeight() / 2;

  float dirX = (length > 0.0f) ? dx / length : 1.0f;
  float dirY = (length > 0.0f) ? dy / length : 0.0f;

  float arrowEndX = float(end.x) - dirX * float(childRadius);
  float arrowEndY = float(end.y) - dirY * float(childRadius);

  juce::Path curvePath;
  {
    float absDx = std::abs(dx);
    float absDy = std::abs(dy);

    if (absDx < 1.0f || absDy < 1.0f) {
      curvePath.startNewSubPath(float(start.x), float(start.y));
      curvePath.lineTo(arrowEndX, arrowEndY);
    }
    else {
      float sign  = (dx >= 0.0f) ? 1.0f : -1.0f;
      float perpX = -dirY * sign;
      float perpY =  dirX * sign;
      float segLen = std::sqrt(dx * dx + dy * dy);
      float offset = segLen * 0.15f;

      float cp1X = float(start.x) + dx * 0.33f + perpX * offset;
      float cp1Y = float(start.y) + dy * 0.33f + perpY * offset;
      float cp2X = float(start.x) + dx * 0.67f - perpX * offset;
      float cp2Y = float(start.y) + dy * 0.67f - perpY * offset;

      curvePath.startNewSubPath(float(start.x), float(start.y));
      curvePath.cubicTo(cp1X, cp1Y, cp2X, cp2Y, arrowEndX, arrowEndY);
    }
  }

  auto arrowBounds = curvePath.getBounds().expanded(40.0f).toNearestInt();

  setBounds(arrowBounds);
  repaint();
}

void NodeArrow::bindToProperty(juce::ValueTree tree, const juce::Identifier propertyID) {
  boundNodeValueTree = tree;
  bindValue.referTo(tree.getPropertyAsValue(propertyID,nullptr));
}

void NodeArrow::valueChanged(juce::Value&) {
}

void NodeArrow::updateBoundProperty(int boundValue) {
  bindValue.setValue(boundValue);
}

void NodeArrow::triggerSnapAnimation()
{
    animT        = 0.0f;
    animVelocity = 0.0f;
    ensureAnimationTimerRunning();
}

void NodeArrow::startProgress(int durationMs)
{
    progressT          = 0.0f;
    progressActive     = durationMs > 0;
    progressStartMs    = juce::Time::getMillisecondCounterHiRes();
    progressDurationMs = juce::jmax(1, durationMs);
    ensureAnimationTimerRunning();
    repaint();
}

void NodeArrow::resetProgress()
{
    progressT      = 0.0f;
    progressActive = false;
    repaint();
}

void NodeArrow::ensureAnimationTimerRunning()
{
    if (! isTimerRunning()) {
        startTimerHz(animationTimerHz);
    }
}

bool NodeArrow::isSnapSettled() const
{
    return std::abs(animT - 1.0f) < snapSettledEpsilon
        && std::abs(animVelocity) < snapSettledEpsilon;
}

bool NodeArrow::advanceSnapAnimation()
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

bool NodeArrow::advanceProgressAnimation()
{
    if (! progressActive) {
        return true;
    }

    const double currentTimeMs      = juce::Time::getMillisecondCounterHiRes();
    const double elapsedMs          = currentTimeMs - progressStartMs;
    const double normalisedProgress = elapsedMs / static_cast<double>(progressDurationMs);

    if (normalisedProgress >= 1.0) {
        progressT      = 1.0f;
        progressActive = false;
        return true;
    }

    progressT = static_cast<float>(juce::jlimit(0.0, 1.0, normalisedProgress));
    return false;
}

void NodeArrow::timerCallback()
{
    const bool snapDone     = advanceSnapAnimation();
    const bool progressDone = advanceProgressAnimation();

    if (snapDone && progressDone) {
        stopTimer();
    }

    repaint();
}
