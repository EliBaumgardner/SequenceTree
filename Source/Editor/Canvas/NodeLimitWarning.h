#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class GraphState;

class NodeLimitWarning : private juce::ValueTree::Listener, private juce::AsyncUpdater
{
public:

    NodeLimitWarning(GraphState& graphState, juce::Component& target);
    ~NodeLimitWarning() override;

private:

    void handleAsyncUpdate() override;

    void valueTreeChildAdded  (juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex) override;

    GraphState&      graphState;
    juce::Component& target;

    bool hasWarned = false;
};
