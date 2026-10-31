#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <map>
#include <memory>
#include <ArsNovaUI/Canvas/CanvasArrow.h>
#include <ArsNovaUI/Editors/ValueEditor.h>
#include "../../Audio/ThreadMessaging/BridgeCommands.h"
#include "../../Util/ArrowInfo.h"
#include <ArsNovaUI/Theme/Visuals.h>

/*
    The component that shows one arrow on the canvas. It derives from ArsNovaUI::CanvasArrow, which
    holds the arrow's start and end nodes (or its start node and loose tip when it is dangling),
    its tree, its shape and its snap and fade animation. arrowTree is the connection's tree, or
    the dangling arrow's tree, which holds its ArrowInfo::Settings (see ArrowBindingOps).

    The arrow's length is its meaning. getDuration turns the offset from the start node's centre
    to the tip into milliseconds with ArrowInfo::Settings::durationFromDelta, and getDurationLabel
    shows it (as a percentage when the arrow starts at a modulator, or as the pitched node's pitch
    when the arrow is bound to pitch alone). beginDurationEdit lets the user type a duration
    instead, which is stored as the arrow's fixed duration. A dangling arrow also has an editor for
    its count limit.

    getArrowVisual puts the geometry, the label and the animation state into the
    ArsNovaUI::ArrowVisual that the look-and-feel draws. The trails are the marks that show a
    traversal walking along the arrow: startProgress begins or restarts one under its trail id, and
    it goes from 0 to 1 over its duration, a oneShot trail being removed when it finishes. A
    trail's source is Live (from the playing audio) or Preview (from note preview). While
    trailsPaused is set the Live trails stand still, and resumeProgress moves their start times
    forward by the time spent paused. advanceExtraAnimation moves them on the display frames
    CanvasArrow already runs. For an arrow that leaves a traversal flag, sourceHovered and
    proximityHovered say whether the flag or the cursor is near enough to fade the arrow in.

    NodeArrowCanvas creates every arrow; AudioCommandDrainer starts its trails through
    startProgress, and the arrow gestures hit-test and drag it through CanvasHitTester.
*/

class Node;

class Arrow : public ArsNovaUI::CanvasArrow
{
public:

    Arrow(ArsNovaUI::CanvasNode* startNode, ArsNovaUI::CanvasNode* endNode, juce::UndoManager& undoManager);
    Arrow(ArsNovaUI::CanvasNode* startNode, juce::Point<int> tipOffset, juce::UndoManager& undoManager);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void beginDurationEdit() override;

    void startProgress(int trailId, int durationMs, int elapsedMs, juce::Colour colour, bool oneShot, BridgeCommands::TrailSource source);
    void resetProgress(int trailId);
    void resetProgress ();
    void resumeProgress ();

    bool advanceExtraAnimation() override;
    bool showsDurationLabel   () const override;
    bool isTraversalArrow     () const;
    bool isSyncArrow          () const;

    int getDuration() const;

    juce::String getDurationLabel() const override;

    ArsNovaUI::ArrowVisual getArrowVisual() const;

    ArsNovaUI::ArrowVisual::ShaftStyle shaftStyle() const;

    struct Trail
    {
        float progress = 0.0f;

        double startMs = 0.0;

        int durationMs = 1;

        juce::Colour colour { juce::Colours::white };

        bool active  = false;
        bool oneShot = false;

        BridgeCommands::TrailSource source = BridgeCommands::TrailSource::Live;
    };

    std::unique_ptr<ArsNovaUI::ValueEditor> valueEditor;

    std::map<int, Trail> trails;

    double pausedAtMs = 0.0;

    bool sourceHovered    = false;
    bool proximityHovered = false;
    bool trailsPaused     = false;
};