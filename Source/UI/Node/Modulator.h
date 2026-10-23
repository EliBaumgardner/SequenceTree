#pragma once

#include <juce_graphics/juce_graphics.h>
#include "Node.h"

/*
    The Node for a modulator, drawn as a square of the same area as a node's circle. In pitch
    display mode its main editor shows the ModAmount of its tree, a signed offset from -48 to 48,
    instead of a note's pitch; the other display modes are Node's. hitTest accepts the square and
    the visible badge editors, and getBodyExtent gives the distance from its centre to its edge
    along a direction, which Arrow uses to stop its shaft at the square.

    NodeManager creates it for the ModulatorData, ModulatorRootData and AlternativeModulatorData
    trees.
*/

class NodeCanvas;

class Modulator : public Node
{
public:

    explicit Modulator(juce::UndoManager& undoManager);

    void paint(juce::Graphics& graphics) override;

    void bindValueEditorForMode() override;

    bool hitTest(int x, int y) override;

    float getBodyExtent(juce::Point<float> approachDirection) const override;

    juce::Rectangle<float> getSquareBounds() const;

    static constexpr int minimumPitchOffset = -48;
    static constexpr int maximumPitchOffset =  48;

    static constexpr float equalAreaSideFactor = 0.8862f;
};
