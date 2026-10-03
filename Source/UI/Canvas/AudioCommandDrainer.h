#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

struct ApplicationContext;

class NodeCanvas;

class AudioCommandDrainer
{
public:

    AudioCommandDrainer(NodeCanvas& canvas, const ApplicationContext& context);

    void drainAll();

private:

    void drainHighlights();
    juce::Colour getTraversalColour(int traversalId) const;
    void drainArrows();
    void drainCounts();

    NodeCanvas&         canvas;
    const ApplicationContext& applicationContext;
};
