/*
  ==============================================================================

    NodeArrow.h
    Created: 12 Jun 2025 12:45:57am
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Util/ApplicationContext.h"

class Node;

class NodeArrow : public juce::Component, juce::Value::Listener, juce::Timer
{
public:

  NodeArrow(Node* parentNode, Node* childNode, ApplicationContext& context);
  ~NodeArrow() override { stopTimer(); }
  void paint (juce::Graphics& g) override;
  void setArrowBounds(Node* movedNode);

  void updateBoundProperty(int boundValue);
  void bindToProperty(juce::ValueTree tree, const juce::Identifier propertyID);
  void valueChanged(juce::Value&) override;

  void triggerSnapAnimation();
  void startProgress(int durationMs);
  void resetProgress();
  void timerCallback() override;

  Node* parentNode = nullptr;
  Node* childNode   = nullptr;

  juce::ValueTree boundNodeValueTree;
  juce::Value bindValue;

  static inline const float durationAmount      {5.0f};
  static inline const int   animationTimerHz    {60};
  static inline const float snapSpringStiffness {0.20f};
  static inline const float snapSpringDamping   {0.30f};
  static inline const float snapSettledEpsilon  {0.001f};

  float length = 0;
  float animT  = 1.0f;
  int duration = 0;

  float progressT      = 0.0f;
  bool  progressActive = false;

  bool updateFromBindValue = false;
  bool isGhost = false;

  juce::TextEditor textEditor;

private:
  float  animVelocity       = 0.0f;
  double progressStartMs    = 0.0;
  int    progressDurationMs = 0;

  void ensureAnimationTimerRunning();
  bool advanceSnapAnimation();
  bool advanceProgressAnimation();
  bool isSnapSettled() const;
};