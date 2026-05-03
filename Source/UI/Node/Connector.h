//
// Created by Eli Baumgardner on 10/6/25.
//

#ifndef SEQUENCETREE_TRAVERSER_H
#define SEQUENCETREE_TRAVERSER_H

#include "../../Util/PluginModules.h"
#include "Node.h"
#include "NodeTextEditor.h"

class NodeCanvas;

class Connector : public Node {

public:

    Connector(ApplicationContext& context);
    void resized() override;
    void paint(juce::Graphics& g) override;

private:

    int count;
    juce::Colour outlineColour = juce::Colours::black;

};

#endif //SEQUENCETREE_TRAVERSER_H