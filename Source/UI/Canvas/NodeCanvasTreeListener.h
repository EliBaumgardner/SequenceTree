#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class NodeCanvas;

class NodeCanvasTreeListener : public juce::ValueTree::Listener
{
public:
    explicit NodeCanvasTreeListener(NodeCanvas& canvasIn);

    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex) override;
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& propertyIdentifier) override;

private:
    NodeCanvas& canvas;
};
