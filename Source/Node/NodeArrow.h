/*
  ==============================================================================

    NodeArrow.h
    Created: 12 Jun 2025 12:45:57am
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "../Util/PluginModules.h"
#include "../Util/PluginContext.h"

class Node;

class NodeArrow : public juce::Component, juce::ComponentListener {

public:

  NodeArrow(Node* startNode, Node* endNode);
  void paint (juce::Graphics& g) override;
  void resized() override;

  void updatePosition();

  Node* startNode = nullptr;
  Node* endNode = nullptr;

  juce::Colour arrowColour = juce::Colours::black;
};