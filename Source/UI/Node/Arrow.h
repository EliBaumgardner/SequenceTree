/*
  ==============================================================================

    Arrow.h
    Created: 12 Jun 2025 12:45:57am
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Util/ApplicationContext.h"
#include "ArrowProgress.h"

class Node;

struct ArrowGeometry
{
    juce::Point<float> centre;
    juce::Point<float> start;
    juce::Point<float> tip;
    juce::Point<float> direction;
    juce::Point<float> chord;

    float length   = 0.0f;
    bool  straight = true;
    bool  drawHead = false;
    bool  valid    = false;
};

class Arrow : public juce::Component, juce::Value::Listener, juce::Timer
{
public:

  Arrow(Node* startNode, Node* endNode, ApplicationContext& context);
  Arrow(Node* startNode, juce::Point<int> tipOffset, ApplicationContext& context);
  ~Arrow() override { stopTimer(); }

  bool isDangling() const { return endNode == nullptr; }
  bool isDashed() const;

  juce::Point<int>   getTip() const;
  juce::Point<float> getHeadAnchor() const;
  int                getDuration() const;
  juce::String       getDurationLabel() const;

  ArrowGeometry getGeometry(float animationT) const;
  juce::Path    buildShaftPath(ArrowGeometry& geometry, float headLength, juce::Point<float> origin) const;

  void paint (juce::Graphics& g) override;
  void setArrowBounds();
  void setTipOffset(juce::Point<int> offset);

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

  juce::Point<int> tipOffset;

  juce::ValueTree arrowTree;
  juce::ValueTree boundNodeValueTree;
  juce::Value bindValue;

  static inline const float durationAmount      {5.0f};
  static inline const int   animationTimerHz    {60};
  static inline const float snapSpringStiffness {0.20f};
  static inline const float snapSpringDamping   {0.30f};
  static inline const float snapSettledEpsilon  {0.001f};
  static inline const float hoverFadeStep       {0.08f};
  static inline const float hoverFadeEpsilon    {0.001f};
  static inline const float curvePerpScale      {0.8f};
  static inline const float curveOffsetFactor   {0.15f};
  static inline const float headVisibleThreshold  {0.3f};
  static inline const float labelVisibleThreshold {0.8f};
  static inline const float headAnchorInset     {8.0f};
  static inline const int   arrowBoundsPadding  {40};

  float animT = 1.0f;

  float hoverAlpha       = 1.0f;
  float hoverAlphaTarget = 1.0f;

  bool sourceHovered     = false;
  bool proximityHovered  = false;

  ArrowProgress progress;

  bool updateFromBindValue = false;
  bool isGhost   = false;
  bool dashed    = false;
  bool hovered   = false;
  bool selected  = false;

private:
  float  animVelocity       = 0.0f;

  void ensureAnimationTimerRunning();
  bool advanceSnapAnimation();
  bool advanceHoverFade();
  bool isSnapSettled() const;
};
