//
// Created by Eli Baumgardner on 10/6/25.
//

#ifndef SEQUENCETREE_TRAVERSER_H
#define SEQUENCETREE_TRAVERSER_H

#include "../Util/ProjectModules.h"
#include "Node.h"
#include "NodeBox.h"

class NodeCanvas;

class Traverser : public Node {

public:

    Traverser(NodeCanvas* canvas);
    void resized() override;
    void paint(juce::Graphics& g) override;
    void setDisplayMode(NodeBox::DisplayMode mode);


private:

    int count;
    juce::Colour outlineColour = juce::Colours::black;

};

#endif //SEQUENCETREE_TRAVERSER_H