#pragma once

#include "../Node/Node.h"

class OrnamentNode : public Node
{
public:

    explicit OrnamentNode(juce::UndoManager& undoManager);

    void resized() override;

    void bindToTree            () override;
    void bindValueEditorForMode() override;
    void respondToClick        (juce::Point<int> localPoint) override;

    ValueEditor velocityEditor;
};
