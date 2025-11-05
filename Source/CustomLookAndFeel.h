//
// Created by Eli Baumgardner on 11/4/25.
//

#ifndef SEQUENCETREE_CUSTOMLOOKANDFEEL_H
#define SEQUENCETREE_CUSTOMLOOKANDFEEL_H

#include <juce_gui_basics/juce_gui_basics.h>

class Node;

class CustomLookAndFeel : public juce::LookAndFeel_V4
{

public:

    CustomLookAndFeel() = default;

    virtual void drawNode(juce::Graphics& g, const Node& node);

private:
    juce::Colour nodeColour = juce::Colours::blue;

};


#endif //SEQUENCETREE_CUSTOMLOOKANDFEEL_H