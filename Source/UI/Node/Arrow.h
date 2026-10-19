#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ArrowAnimation.h"
#include <ArsNovaUI/Editors/ValueEditor.h>
#include "../../Util/ArrowInfo.h"
#include <ArsNovaUI/Theme/Visuals.h>

class Node;

class Arrow : public juce::Component
{
public:

    Arrow(Node* startNode, Node* endNode, juce::UndoManager& undoManager);
    Arrow(Node* startNode, juce::Point<int> tipOffset, juce::UndoManager& undoManager);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void setArrowBounds();
    void setTipOffset  (juce::Point<int> offset);

    void beginDurationEdit();
    void endDrag          ();

    void triggerSnapAnimation();
    void advanceAnimation    (double frameSec);

    void setHoverFade  (bool shouldBeVisible);
    void initHoverState(bool visibleNow);

    void startProgress (int trailId, int durationMs, int elapsedMs, juce::Colour colour, bool oneShot, TrailSource source);
    void resetProgress ();
    void resetProgress (int trailId);
    void resumeProgress();

    bool isDangling() const
    {
        return endNode == nullptr;
    }

    bool isTraversalArrow  () const;
    bool isSyncArrow       () const;
    bool showsDurationLabel() const;

    int          getDuration     () const;
    juce::String getDurationLabel() const;

    ArsNovaUI::ArrowVisual   getArrowVisual () const;
    ArsNovaUI::ArrowGeometry getGeometry    (float animationProgress) const;
    ArsNovaUI::ArrowLabel    getLabel       (const ArsNovaUI::ArrowGeometry& geometry, float headLength) const;
    juce::Path               buildShaftPath (const ArsNovaUI::ArrowGeometry& geometry, float headLength, juce::Point<float> origin) const;

    juce::Point<int>   getTip       () const;
    juce::Point<float> getHeadAnchor() const;

    ArsNovaUI::ArrowVisual::ShaftStyle shaftStyle() const;

    Node* const startNode = nullptr;
    Node* const endNode   = nullptr;

    juce::Point<int> tipOffset;

    int danglingIndex = -1;

    std::unique_ptr<ArsNovaUI::ValueEditor> valueEditor;
    std::unique_ptr<ArsNovaUI::ValueEditor> durationEditor;

    juce::ValueTree arrowTree;

    static constexpr float curvePerpScale         = 0.8f;
    static constexpr float curveOffsetFactor      = 0.15f;
    static constexpr float headVisibleThreshold   = 0.3f;
    static constexpr float labelVisibleThreshold  = 0.8f;
    static constexpr float headAnchorInset        = 8.0f;
    static constexpr float arrowHeadLength        = 9.0f;
    static constexpr float arrowHeadLengthHover   = 11.0f;
    static constexpr float verticalLabelThreshold = 0.2f;
    static constexpr int   arrowBoundsPadding     = 40;
    static constexpr int   valueEditorWidth       = 30;
    static constexpr int   valueEditorHeight      = 12;

    ArrowAnimation         animation;
    juce::VBlankAttachment animationFrames;

    bool sourceHovered    = false;
    bool proximityHovered = false;

    bool editingDuration = false;

    bool isGhost  = false;
    bool dragging = false;
    bool hovered  = false;
    bool selected = false;
};
