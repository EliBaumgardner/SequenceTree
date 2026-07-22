//
// Created by Eli Baumgardner on 7/21/26.
//

#ifndef SEQUENCETREE_NODEVISUAL_H
#define SEQUENCETREE_NODEVISUAL_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <map>

struct NodeVisual {
    juce::Rectangle<float> bounds;
    juce::Colour colour;
    const std::map<int, juce::Colour>& highlights;
    bool isHovered  = false;
    bool isSelected = false;
};

#endif //SEQUENCETREE_NODEVISUAL_H
