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

class NodeArrow : public juce::Component, juce::Value::Listener, juce::Timer
{
public:

  NodeArrow(Node* parentNode, Node* childNode);
  ~NodeArrow() override { stopTimer(); }
  void paint (juce::Graphics& g) override;
  void setArrowBounds(Node* movedNode);

  void updateBoundProperty(int boundValue);
  void bindToProperty(juce::ValueTree tree, const juce::Identifier propertyID);
  void valueChanged(juce::Value&) override;

  void triggerSnapAnimation();
  void timerCallback() override;

  Node* parentNode = nullptr;
  Node* childNode   = nullptr;

  juce::ValueTree boundNodeValueTree;
  juce::Value bindValue;

  static inline const float durationAmount {5.0f};

  float length = 0;
  float animT  = 1.0f;

  bool updateFromBindValue= false;

  juce::TextEditor textEditor;

private:
  float animVelocity = 0.0f;
};