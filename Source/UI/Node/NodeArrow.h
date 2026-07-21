/*
  ==============================================================================

    NodeArrow.h
    Created: 12 Jun 2025 12:45:57am
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Util/ApplicationContext.h"
#include "ArrowProgress.h"

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
  void setHoverFade(bool shouldBeVisible);
  void initHoverState(bool visibleNow);
  void refreshHoverVisibility() { setHoverFade(sourceHovered || proximityHovered); }
  void startProgress(int traversalId, int durationMs, juce::Colour colour, bool oneShot = false);
  void resetProgress();
  void resetProgress(int traversalId);
  void timerCallback() override;

  Node* startNode = nullptr;
  Node* endNode   = nullptr;

  juce::ValueTree boundNodeValueTree;
  juce::Value bindValue;

  static inline const float durationAmount      {5.0f};
  static inline const int   animationTimerHz    {60};
  static inline const float snapSpringStiffness {0.20f};
  static inline const float snapSpringDamping   {0.30f};
  static inline const float snapSettledEpsilon  {0.001f};
  static inline const float hoverFadeStep       {0.08f};
  static inline const float hoverFadeEpsilon    {0.001f};

  float length = 0;
  float animT  = 1.0f;
  int duration = 0;

  float hoverAlpha       = 1.0f;
  float hoverAlphaTarget = 1.0f;

  bool sourceHovered     = false;
  bool proximityHovered  = false;

  ArrowProgress progress;

  bool updateFromBindValue = false;
  bool isGhost = false;
  bool hovered = false;
  bool selected = false;

  juce::TextEditor textEditor;

private:
  float  animVelocity       = 0.0f;

  void ensureAnimationTimerRunning();
  bool advanceSnapAnimation();
  bool advanceHoverFade();
  bool isSnapSettled() const;
};