#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ArrowAnimation.h"
#include <ArsNovaUI/Editors/ValueEditor.h>
#include "../../Util/ArrowInfo.h"
#include <ArsNovaUI/Theme/Visuals.h>

/*
    The component that shows one arrow on the canvas. An arrow either connects two nodes, from
    startNode to endNode, or is dangling: it starts at a node and ends at a loose tip at tipOffset
    from it, with endNode null. arrowTree is the connection's tree, or the dangling arrow's tree,
    which holds its ArrowInfo::Settings (see ArrowBindingOps).

    The arrow's length is its meaning. getDuration turns the offset from the start node's centre
    to the tip into milliseconds with ArrowInfo::Settings::durationFromDelta, and getDurationLabel shows
    it (as a percentage when the arrow starts at a modulator, or as the pitched node's pitch when
    the arrow is bound to pitch alone). beginDurationEdit lets the user type a duration instead,
    which is stored as the arrow's fixed duration. A dangling arrow also has an editor for its
    count limit.

    getGeometry works out where the shaft starts and ends, stopping at the edge of the end node's
    body, and whether it is a straight line or a curve. getArrowVisual puts that, the label and
    the animation state into the ArsNovaUI::ArrowVisual that the look-and-feel draws. animation
    (see ArrowAnimation) holds the snap, fade and trails that show the walk moving along the
    arrow, and an animation frame callback runs only while something is moving.

    ArrowManager creates and removes every arrow; AudioCommandDrainer starts its trails through
    startProgress, and the arrow gestures hit-test and drag it through CanvasHitTester.
*/

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

    void startProgress (int trailId, int durationMs, int elapsedMs, juce::Colour colour, bool oneShot, BridgeCommands::TrailSource source);
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
