//
// Created by Eli Baumgardner on 10/6/25.
//

#ifndef SEQUENCETREE_COUNTER_H
#define SEQUENCETREE_COUNTER_H

#include "../Util/PluginModules.h"
#include "Node.h"
#include "NodeBox.h"

class NodeCanvas;

class Counter : public Node {

    public:

    Counter();
    void resized() override;
    void paint(juce::Graphics& g) override;
    void setDisplayMode(NodeBox::DisplayMode mode);


    private:

    int count;
    juce::Colour outlineColour = juce::Colours::black;
};

#endif //SEQUENCETREE_COUNTER_H