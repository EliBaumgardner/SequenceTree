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

struct ArrowLabel
{
    juce::Point<float> centre;
    float angle = 0.0f;
};

class Arrow : public juce::Component
{
public:

  Arrow(Node* startNode, Node* endNode, const ApplicationContext& context);
  Arrow(Node* startNode, juce::Point<int> tipOffset, const ApplicationContext& context);

  bool isDangling() const { return endNode == nullptr; }
  bool isDashed() const;
  bool isTraversalArrow() const;
  bool isSyncArrow() const;

  juce::Point<int>   getTip() const;
  juce::Point<float> getHeadAnchor() const;
  int                getDuration() const;
  bool               showsDurationLabel() const;
  juce::String       getDurationLabel() const;

  ArrowGeometry getGeometry(float animationT) const;
  ArrowLabel    getLabel(const ArrowGeometry& geometry, float headLength) const;
  juce::Path    buildShaftPath(const ArrowGeometry& geometry, float headLength, juce::Point<float> origin) const;

  void paint (juce::Graphics& g) override;
  void resized() override;
  void setArrowBounds();
  void setTipOffset(juce::Point<int> offset);

  void beginDurationEdit();

  void triggerSnapAnimation();
  void setHoverFade(bool shouldBeVisible);
  void initHoverState(bool visibleNow);
  void startProgress(int trailId, int durationMs, juce::Colour colour, bool oneShot = false);
  void resetProgress();
  void resetProgress(int trailId);
  void resumeProgress();
  void advanceAnimation(double frameSec);

  Node* const startNode = nullptr;
  Node* const endNode   = nullptr;

  juce::Point<int> tipOffset;

  int danglingIndex = -1;

  std::unique_ptr<ValueEditor> valueEditor;
  std::unique_ptr<ValueEditor> durationEditor;

  juce::ValueTree arrowTree;

  static constexpr float curvePerpScale        {0.8f};
  static constexpr float curveOffsetFactor     {0.15f};
  static constexpr float headVisibleThreshold  {0.3f};
  static constexpr float labelVisibleThreshold {0.8f};
  static constexpr float headAnchorInset       {8.0f};
  static constexpr float arrowHeadLength       {9.0f};
  static constexpr float arrowHeadLengthHover  {11.0f};
  static constexpr float verticalLabelThreshold{0.2f};
  static constexpr int     arrowBoundsPadding    {40};
  static constexpr int     valueEditorWidth      {30};
  static constexpr int     valueEditorHeight     {12};

  ArrowAnimation animation;
  juce::VBlankAttachment animationFrames;

  bool sourceHovered    = false;
  bool proximityHovered = false;

  bool editingDuration = false;

  bool isGhost  = false;
  bool dashed   = false;
  bool hovered  = false;
  bool selected = false;

private:
};
