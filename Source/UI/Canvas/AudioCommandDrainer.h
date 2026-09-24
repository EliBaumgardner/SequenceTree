//
// Created by Eli Baumgardner on 7/21/26.
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

struct ApplicationContext;

class NodeCanvas;

class AudioCommandDrainer {

public:

    AudioCommandDrainer(NodeCanvas& canvas, const ApplicationContext& context);

    void drainAll();

private:

    void drainHighlights();
    void drainArrows();
    void drainCounts();

    juce::Colour getTraversalColour(int traversalId) const;

    NodeCanvas&         canvas;
    const ApplicationContext& applicationContext;

    bool awaitingFirstDrain = true;
};
