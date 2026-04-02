/*
  ==============================================================================

    NodeArrow.h
    Created: 12 Jun 2025 12:45:57am
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Util/PluginContext.h"
class Node;

class NodeArrow : public juce::Component, juce::Value::Listener
{
public:

  NodeArrow(Node* parentNode, Node* childNode);
  void paint (juce::Graphics& g) override;
  void setArrowBounds(Node* movedNode);
  void bindToProperty(juce::ValueTree tree, const juce::Identifier propertyID);
  void valueChanged(juce::Value&) override;

  Node* parentNode = nullptr;
  Node* childNode   = nullptr;

  juce::ValueTree boundNodeValueTree;
  juce::Value bindValue;

  bool isUpdatingFromBounds = false;
  bool isUpdatingFromPosition = false;
};