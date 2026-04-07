/*
  ==============================================================================

    NodeArrow.cpp
    Created: 12 Jun 2025 12:45:57am
    Author:  Eli Baimgardner

  ==============================================================================
*/
#include "../Util/ValueTreeState.h"
#include "../Util/ValueTreeIdentifiers.h""
#include "NodeArrow.h"

#include "Node.h"
#include "../CustomLookAndFeel.h"

NodeArrow::NodeArrow(Node* startNode, Node* endNode) : parentNode(startNode), childNode(endNode){
  setLookAndFeel(ComponentContext::lookAndFeel);

  bindValue.addListener(this);
}

void NodeArrow::paint(juce::Graphics &g) {
  if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) {
    customLookAndFeel->drawNodeArrow(g,*this, textEditor);
  }
}

void NodeArrow::setArrowBounds(Node* movedNode) {

  juce::Point<int> start = parentNode->getBounds().getCentre();
  juce::Point<int> end   = childNode->getBounds().getCentre();

  float dx = float(end.x - start.x);
  float dy = float(end.y - start.y);

  length = std::sqrt(dx*dx + dy*dy);

  childNode->incomingAngle = std::atan2(dy, dx);

  duration = (int)(std::abs(dx) * durationAmount);
  bindValue.setValue(duration);
  textEditor.setText(juce::String(duration));

  // Build the curve in canvas coordinates to get the true bounding box.
  int parentRadius = parentNode->getWidth() / 2;
  int childRadius  = childNode->getWidth()  / 2;

  float dirX = (length > 0.0f) ? dx / length : 1.0f;
  float dirY = (length > 0.0f) ? dy / length : 0.0f;

  float arrowEndX = float(end.x) - dirX * float(childRadius);
  float arrowEndY = float(end.y) - dirY * float(childRadius);

  float connDirX = std::cos(parentNode->incomingAngle);
  float connDirY = std::sin(parentNode->incomingAngle);

  float startX = float(start.x) + float(parentRadius) * connDirX;
  float startY = float(start.y) + float(parentRadius) * connDirY;

  float toEndX   = arrowEndX - startX;
  float toEndY   = arrowEndY - startY;
  float toEndLen = std::sqrt(toEndX*toEndX + toEndY*toEndY);

  juce::Path curvePath;
  if (toEndLen > 1.0f)
  {
    float toEndDirX    = toEndX / toEndLen;
    float toEndDirY    = toEndY / toEndLen;
    float tangentLen   = toEndLen * 0.4f;

    float cp1X = startX + tangentLen * connDirX;
    float cp1Y = startY + tangentLen * connDirY;
    float cp2X = arrowEndX - tangentLen * toEndDirX;
    float cp2Y = arrowEndY - tangentLen * toEndDirY;

    curvePath.startNewSubPath(startX, startY);
    curvePath.cubicTo(cp1X, cp1Y, cp2X, cp2Y, arrowEndX, arrowEndY);
  }
  else
  {
    curvePath.startNewSubPath(float(start.x), float(start.y));
    curvePath.lineTo(arrowEndX, arrowEndY);
  }

  // Expand enough for stroke (2px), arrowhead (~12px), and label text (~30px).
  auto arrowBounds = curvePath.getBounds().expanded(40.0f).toNearestInt();

  setBounds(arrowBounds);
  repaint();
}

void NodeArrow::bindToProperty(juce::ValueTree tree, const juce::Identifier propertyID) {
  boundNodeValueTree = tree;
  bindValue.referTo(tree.getPropertyAsValue(propertyID,nullptr));
}

void NodeArrow::valueChanged(juce::Value&) {
  // if (!updateFromBindValue) {
  //   return;
  // }
  //
  // DBG("Value Changed");
  //
  // updateFromBindValue = false;
  //
  // int duration = bindValue.getValue();
  //
  //
  // juce::Point<int> start = parentNode->getBounds().getCentre();
  // juce::Point<int> end   = childNode->getBounds().getCentre();
  //
  // float dx = end.x - start.x;
  // float dy = end.y - start.y;
  //
  // float currentLength = std::sqrt(dx * dx + dy * dy);
  //
  // jassert(currentLength > 0);
  //
  // float unitX = dx / currentLength;
  // float unitY = dy / currentLength;
  //
  // float newLength = duration / durationAmount;
  //
  // float newDx = unitX * newLength;
  // float newDy = unitY * newLength;
  //
  // juce::Point<int> newEnd(
  //     start.x + (int)newDx,
  //     start.y + (int)newDy
  // );
  //
  // juce::ValueTree parentNodeTree = ValueTreeState::getNode(parentNode->getComponentID().getIntValue());
  // juce::ValueTree childNodeTree  = ValueTreeState::getNode(childNode->getComponentID().getIntValue());
  //
  // int left = std::min(start.x, newEnd.x);
  // int top = std::min(start.y, newEnd.y);
  // int right = std::max(start.x, newEnd.x);
  // int bottom = std::max(start.y, newEnd.y);
  //
  // juce::Rectangle arrowBounds = juce::Rectangle<int>::leftTopRightBottom(left, top, right, bottom).expanded(2);
  // setBounds(arrowBounds);
  //
  // childNodeTree.setProperty(ValueTreeIdentifiers::XPosition, newEnd.x, nullptr);
  // childNodeTree.setProperty(ValueTreeIdentifiers::YPosition, newEnd.y, nullptr);
}

void NodeArrow::updateBoundProperty(int boundValue) {
  bindValue.setValue(boundValue);
}

void NodeArrow::triggerSnapAnimation()
{
    animT        = 0.0f;
    animVelocity = 0.0f;
    startTimerHz(60);
}

void NodeArrow::timerCallback()
{
    constexpr float stiffness = 0.20f;
    constexpr float damping   = 0.30f;

    animVelocity += (1.0f - animT) * stiffness;
    animVelocity *= damping;
    animT        += animVelocity;

    if (std::abs(animT - 1.0f) < 0.001f && std::abs(animVelocity) < 0.001f)
    {
        animT = 1.0f;
        stopTimer();
    }
    repaint();
}
