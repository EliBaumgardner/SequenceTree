#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class NodeCanvas;

class CanvasFrame : public juce::Component
{
public:

    CanvasFrame(NodeCanvas& nodeCanvas, juce::Component& menuArea, juce::Component& titleBar, juce::Component& bottomBar);

    void paint(juce::Graphics& graphics) override;

    juce::Rectangle<int> canvasArea;

private:

    NodeCanvas&      nodeCanvas;
    juce::Component& menuArea;
    juce::Component& titleBar;
    juce::Component& bottomBar;
};
