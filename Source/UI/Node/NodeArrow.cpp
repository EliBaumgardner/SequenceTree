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
    customLookAndFeel->drawNodeArrow(g,*this);
  }
}

void NodeArrow::setArrowBounds(Node* movedNode) {

  juce::Point<int> start = parentNode->getBounds().getCentre();
  juce::Point<int >end   = childNode->getBounds().getCentre();

  int left = std::min(start.x, end.x);
  int top = std::min(start.y, end.y);
  int right = std::max(start.x, end.x);
  int bottom = std::max(start.y,end.y);

  float dx = end.x - start.x;
  float dy = end.y - start.y;

  length = std::sqrt(dx*dx + dy*dy);

  int duration = (int)(length * growthFactor);

  bindValue.setValue(duration);

  juce::Rectangle arrowBounds = juce::Rectangle<int>::leftTopRightBottom(left, top, right, bottom).expanded(2);
  setBounds(arrowBounds);
  repaint();
}

void NodeArrow::bindToProperty(juce::ValueTree tree, const juce::Identifier propertyID) {
  boundNodeValueTree = tree;
  bindValue.referTo(tree.getPropertyAsValue(propertyID,nullptr));
}

void NodeArrow::valueChanged(juce::Value&) {
  if (!updateFromBindValue) {
    return;
  }

  DBG("Value Changed");

  updateFromBindValue = false;

  int duration = bindValue.getValue();


  juce::Point<int> start = parentNode->getBounds().getCentre();
  juce::Point<int> end   = childNode->getBounds().getCentre();

  float dx = end.x - start.x;
  float dy = end.y - start.y;

  float currentLength = std::sqrt(dx * dx + dy * dy);

  jassert(currentLength > 0);

  float unitX = dx / currentLength;
  float unitY = dy / currentLength;

  float newLength = duration / growthFactor;

  float newDx = unitX * newLength;
  float newDy = unitY * newLength;

  juce::Point<int> newEnd(
      start.x + (int)newDx,
      start.y + (int)newDy
  );

  juce::ValueTree parentNodeTree = ValueTreeState::getNode(parentNode->getComponentID().getIntValue());
  juce::ValueTree childNodeTree  = ValueTreeState::getNode(childNode->getComponentID().getIntValue());

  int left = std::min(start.x, newEnd.x);
  int top = std::min(start.y, newEnd.y);
  int right = std::max(start.x, newEnd.x);
  int bottom = std::max(start.y, newEnd.y);

  juce::Rectangle arrowBounds = juce::Rectangle<int>::leftTopRightBottom(left, top, right, bottom).expanded(2);
  setBounds(arrowBounds);

  childNodeTree.setProperty(ValueTreeIdentifiers::XPosition, newEnd.x, nullptr);
  childNodeTree.setProperty(ValueTreeIdentifiers::YPosition, newEnd.y, nullptr);
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
    constexpr float stiffness = 0.28f;
    constexpr float damping   = 0.68f;

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