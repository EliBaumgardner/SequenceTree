//
// Created by Eli Baumgardner on 10/6/25.
//

#ifndef SEQUENCETREE_MODULATOR_H
#define SEQUENCETREE_MODULATOR_H

#include "../Util/PluginModules.h"
#include "Node.h"
#include "NodeTextEditor.h"

class NodeCanvas;

class Modulator : public Node {

    public:

    Modulator(ApplicationContext& context);
    void resized() override;
    void paint(juce::Graphics& g) override;
    void setDisplayMode(NodeTextEditor::DisplayMode mode);


    private:

    int count;
    juce::Colour outlineColour = juce::Colours::black;
};

#endif //SEQUENCETREE_MODULATOR_H
