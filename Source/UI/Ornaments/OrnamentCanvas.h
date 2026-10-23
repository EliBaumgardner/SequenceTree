#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "OrnamentNode.h"
#include "../Node/Arrow.h"

#include <map>
#include <memory>

/*
    The component that draws the figure of the ornament being edited (see OrnamentLibraries): a
    grid and a line at zero, an OrnamentNode for each figure node in the figure tree, and an Arrow
    for the connection and the dangling arrow each node can have. The node that is the host,
    standing for the note itself, is drawn with an inner rim. The pan and zoom are viewTransform
    and modelTransform, set by the port it sits in.

    handleAsyncUpdate makes the nodes and arrows match the figure tree: it removes the components
    whose trees are gone, creates new ones, and places each node from its stored position.
    OrnamentBuildPanel requests it whenever the figure changes. findNodeAt and findArrowNear
    answer picking queries for one part of an arrow (shaft, head, label or dangling tip). The
    canvas only draws and picks; OrnamentFigureController makes every edit.
*/

class OrnamentCanvas : public juce::Component, public juce::AsyncUpdater
{
public:

    enum class ArrowPart;

    explicit OrnamentCanvas(juce::UndoManager& undoManager);

    void paint(juce::Graphics& graphics) override;

    void childrenChanged  () override;
    void handleAsyncUpdate() override;

    OrnamentNode* findNodeAt(juce::Point<int> point) const;

    Arrow* findArrowNear(juce::Point<float> point, float radius, ArrowPart part) const;

    enum class ArrowPart { Shaft, Head, Label, DanglingTip };

    static constexpr int initialViewColumns = 4;
    static constexpr int tipGrabRadius      = 14;
    static constexpr int snapThreshold      = 5;

    juce::ValueTree figure;

    juce::AffineTransform viewTransform;
    juce::AffineTransform modelTransform;

    std::map<int, std::unique_ptr<OrnamentNode>> nodes;
    juce::OwnedArray<Arrow>                      arrows;

private:

    void updateArrows();

    juce::UndoManager& undoManager;
};