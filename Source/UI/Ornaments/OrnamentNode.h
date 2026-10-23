#pragma once

#include "../Node/Node.h"

/*
    The Node for one figure node of an ornament. Its main editor shows the node's pitch offset from
    the note the ornament is applied to (OrnamentOffset, signed, within
    OrnamentLibraries::maximumOffset), and a badge editor in the corner of the circle shows its
    velocity as a percentage (OrnamentVelocity). It has none of a graph node's count limit,
    switch count or sub-loop badges.

    OrnamentCanvas creates one for each node in the figure being edited.
*/

class OrnamentNode : public Node
{
public:

    explicit OrnamentNode(juce::UndoManager& undoManager);

    void resized() override;

    void bindToTree            () override;
    void bindValueEditorForMode() override;
    void respondToClick        (juce::Point<int> localPoint) override;

    ArsNovaUI::ValueEditor velocityEditor;
};
