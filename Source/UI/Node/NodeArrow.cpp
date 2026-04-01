/*
  ==============================================================================

    NodeArrow.cpp
    Created: 12 Jun 2025 12:45:57am
    Author:  Eli Baimgardner

  ==============================================================================
*/
#include "NodeArrow.h"
#include "Node.h"
#include "../CustomLookAndFeel.h"

NodeArrow::NodeArrow(Node* startNode, Node* endNode) : parentNode(startNode), childNode(endNode){
  setLookAndFeel(ComponentContext::lookAndFeel);
}

void NodeArrow::paint(juce::Graphics &g) {
  if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) {
    customLookAndFeel->drawNodeArrow(g,*this);
  }
}

void NodeArrow::setArrowBounds(Node* movedNode) {

  juce::Point<int> start;
  juce::Point<int> end;

  if (childNode == movedNode) {
    end = childNode->getBounds().getCentre();
    start = parentNode->getBounds().getCentre();
  }
  else if (parentNode == movedNode) {
    end = parentNode->getBounds().getCentre();
    start = parentNode->getBounds().getCentre();
  }

  juce::Rectangle arrowBounds = juce::Rectangle<int>::leftTopRightBottom(
        std::min(start.x,end.x),
        std::min(start.y,end.y),
        std::max(start.x,end.x),
        std::max(start.y,end.y)
        ).expanded(2);

  setBounds(arrowBounds);
  repaint();
}

void NodeArrow::bindToProperty(juce::ValueTree tree, const juce::Identifier propertyID) {

}
