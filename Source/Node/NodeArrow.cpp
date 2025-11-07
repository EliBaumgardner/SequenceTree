/*
  ==============================================================================

    NodeArrow.cpp
    Created: 12 Jun 2025 12:45:57am
    Author:  Eli Baimgardner

  ==============================================================================
*/
#include "NodeArrow.h"
#include "Node.h"

NodeArrow::NodeArrow(Node* startNode, Node* endNode) : startNode(startNode), endNode(endNode) {

  setLookAndFeel(ComponentContext::lookAndFeel);
}

void NodeArrow::paint(juce::Graphics &g) {
  if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) {
    customLookAndFeel->drawNodeArrow(g,*this);
  }
}

void NodeArrow::resized() {

}
