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
#include "ArrowAnimation.h"
#include "../Editors/ValueEditor.h"
#include "../../Util/ArrowInfo.h"

class Node;

struct ArrowGeometry
{
    juce::Point<float> centre;
    juce::Point<float> start;
    juce::Point<float> tip;
    juce::Point<float> direction;
    juce::Point<float> chord;
    juce::Point<float> control1;
    juce::Point<float> control2;

    float length   = 0.0f;
    bool  straight = true;
    bool  drawHead = false;
    bool  valid    = false;
};

class Arrow : public juce::Component, juce::Timer
{
public:

  Arrow(Node* startNode, Node* endNode, ApplicationContext& context);
  Arrow(Node* startNode, juce::Point<int> tipOffset, ApplicationContext& context);
  ~Arrow() override { stopTimer(); }

  bool isDangling() const { return endNode == nullptr; }
  bool isDashed() const;
  bool isTraversalArrow() const;
  bool connectsTraversalFlag() const;

  juce::Point<int>   getTip() const;
  juce::Point<float> getHeadAnchor() const;
  int                getDuration() const;
  juce::String       getDurationLabel() const;

  ArrowGeometry getGeometry(float animationT) const;
  juce::Path    buildShaftPath(const ArrowGeometry& geometry, float headLength, juce::Point<float> origin) const;

  void paint (juce::Graphics& g) override;
  void resized() override;
  void setArrowBounds();
  void setTipOffset(juce::Point<int> offset);

  void triggerSnapAnimation();
  void setHoverFade(bool shouldBeVisible);
  void initHoverState(bool visibleNow);
  void startProgress(int trailId, int durationMs, juce::Colour colour, bool oneShot = false);
  void resetProgress();
  void resetProgress(int trailId);
  void timerCallback() override;

  Node* const startNode = nullptr;
  Node* const endNode   = nullptr;

  juce::Point<int> tipOffset;

  int danglingIndex = -1;

  std::unique_ptr<ValueEditor> valueEditor;

  juce::ValueTree arrowTree;

  static inline const float curvePerpScale        {0.8f};
  static inline const float curveOffsetFactor     {0.15f};
  static inline const float headVisibleThreshold  {0.3f};
  static inline const float labelVisibleThreshold {0.8f};
  static inline const float headAnchorInset       {8.0f};
  static inline const int   arrowBoundsPadding    {40};
  static inline const int   valueEditorWidth      {30};
  static inline const int   valueEditorHeight     {12};

  ArrowAnimation animation;

  bool sourceHovered    = false;
  bool proximityHovered = false;

  bool isGhost  = false;
  bool dashed   = false;
  bool hovered  = false;
  bool selected = false;
};
