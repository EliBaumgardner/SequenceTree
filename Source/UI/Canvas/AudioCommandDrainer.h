//
// Created by Eli Baumgardner on 7/21/26.
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

struct ApplicationContext;

class NodeCanvas;

class AudioCommandDrainer {

public:

    AudioCommandDrainer(NodeCanvas& canvas, ApplicationContext& context);

    void drainAll() const;

private:

    void drainHighlights() const;
    void drainProgress() const;
    void drainArrowResets() const;
    void drainCounts() const;

    juce::Colour getTraversalColour(int traversalId) const;

    NodeCanvas&         canvas;
    ApplicationContext& applicationContext;
};
