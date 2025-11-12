//
// Created by Eli Baumgardner on 10/6/25.
//

#ifndef SEQUENCETREE_TRAVERSER_H
#define SEQUENCETREE_TRAVERSER_H

#include "../Util/PluginModules.h"
#include "Node.h"
#include "NodeBox.h"

class NodeCanvas;

class RelayNode : public Node {

public:

    RelayNode();
    void resized() override;
    void paint(juce::Graphics& g) override;
    void setDisplayMode(NodeBox::DisplayMode mode);


private:

    int count;
    juce::Colour outlineColour = juce::Colours::black;

};

#endif //SEQUENCETREE_TRAVERSER_H