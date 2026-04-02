/*
  ==============================================================================

    NodeArrow.cpp
    Created: 12 Jun 2025 12:45:57am
    Author:  Eli Baimgardner

  ==============================================================================
*/
#include "../Util/ValueTreeState.h"
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
  juce::Point<int> end   = childNode->getBounds().getCentre();

  float dx = (float)(end.x - start.x);
  float dy = (float)(end.y - start.y);
  float euclideanLength = std::sqrt(dx * dx + dy * dy);

  int duration = (int)(euclideanLength * 10.0f);

  juce::Rectangle arrowBounds = juce::Rectangle<int>::leftTopRightBottom(
      std::min(start.x, end.x),
      std::min(start.y, end.y),
      std::max(start.x, end.x),
      std::max(start.y, end.y)
  ).expanded(2);

  isUpdatingFromBounds = true;
  bindValue.setValue(duration);
  isUpdatingFromBounds = false;

  setBounds(arrowBounds);
  repaint();
}

void NodeArrow::bindToProperty(juce::ValueTree tree, const juce::Identifier propertyID) {
  boundNodeValueTree = tree;
  bindValue.referTo(tree.getPropertyAsValue(propertyID,nullptr));
}

void NodeArrow::valueChanged(juce::Value&) {
  if (isUpdatingFromBounds) return;

  int duration = (int)bindValue.getValue();
  float newLength = (float)duration / 10.0f;

  juce::Point<int> start    = parentNode->getBounds().getCentre();
  juce::Point<int> childPos = childNode->getBounds().getCentre();

  float dx = (float)(childPos.x - start.x);
  float dy = (float)(childPos.y - start.y);
  float currentLength = std::sqrt(dx * dx + dy * dy);

  if (currentLength < 1.0f) return;

  float nx = dx / currentLength;
  float ny = dy / currentLength;

  int newChildX = start.x + (int)(nx * newLength);
  int newChildY = start.y + (int)(ny * newLength);

  isUpdatingFromBounds = true;
  childNode->setCentrePosition(newChildX, newChildY);

  // Calculate arrow bounds directly without triggering setArrowBounds again
  juce::Point<int> end = childNode->getBounds().getCentre();
  juce::Rectangle arrowBounds = juce::Rectangle<int>::leftTopRightBottom(
      std::min(start.x, end.x),
      std::min(start.y, end.y),
      std::max(start.x, end.x),
      std::max(start.y, end.y)
  ).expanded(2);

  setBounds(arrowBounds);
  repaint();
  isUpdatingFromBounds = false;
}