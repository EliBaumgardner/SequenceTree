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


}

void NodeArrow::paint(juce::Graphics &g) {

  auto* a = startNode;
  auto* b = endNode;

  // Arrow size
  float arrowLength = 10.0f;
  float arrowWidth = 5.0f;
  int radius = b->getBounds().getWidth()/2;

  g.setColour(arrowColour);
  int x1 = a->getBounds().getCentreX();
  int y1 = a->getBounds().getCentreY();
  int x2 = b->getBounds().getCentreX();
  int y2 = b->getBounds().getCentreY();

  // Calculate direction vector
  float dx = float(x2 - x1);
  float dy = float(y2 - y1);
  float length = std::sqrt(dx*dx + dy*dy);

  // Normalize direction
  float nx = dx / length;
  float ny = dy / length;

  x2 = x2 - nx*radius;
  y2 = y2 - ny*radius;

  // Draw main line
  g.drawLine(x1, y1, x2, y2, 2.0f);
  g.setColour(juce::Colours::white);
  g.drawLine(x1, y1, x2, y2, 1.0f);

  // Calculate the two points for the arrowhead lines
  float leftX = x2 - arrowLength * nx + arrowWidth * ny;
  float leftY = y2 - arrowLength * ny - arrowWidth * nx;

  float rightX = x2 - arrowLength * nx - arrowWidth * ny;
  float rightY = y2 - arrowLength * ny + arrowWidth * nx;

  // Draw arrowhead lines (black thick)
  g.setColour(juce::Colours::black);
  g.drawLine(x2, y2, leftX, leftY, 2.0f);
  g.drawLine(x2, y2, rightX, rightY, 2.0f);

  // Draw arrowhead lines (white thin) for contrast
  g.setColour(juce::Colours::white);
  g.drawLine(x2, y2, leftX, leftY, 1.0f);
  g.drawLine(x2, y2, rightX, rightY, 1.0f);
}

void resized() {

}

void NodeArrow::updatePosition() {
  auto start = startNode->getBounds().getCentre();
  auto end = endNode->getBounds().getCentre();
  auto bounds = juce::Rectangle<int>::leftTopRightBottom(
      std::min(start.x, end.x),
      std::min(start.y, end.y),
      std::max(start.x, end.x),
      std::max(start.y, end.y)
  );
  setBounds(bounds);
  repaint();
}