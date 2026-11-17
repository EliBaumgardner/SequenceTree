#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/*
    A component laid over the whole editor that draws the border around the canvas: a rounded
    outline at canvasArea, with the area outside the outline (but not under the menu area, title
    bar or bottom bar) filled with frosted glass over the NodeCanvas. It ignores the mouse.

    PluginEditor adds it above the canvas's port and below the bars, and sets canvasArea whenever
    it lays out the window.
*/

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
