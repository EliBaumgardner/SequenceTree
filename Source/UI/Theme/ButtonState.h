//
// Created by Eli Baumgardner on 7/21/26.
//

#ifndef SEQUENCETREE_BUTTONSTATE_H
#define SEQUENCETREE_BUTTONSTATE_H

#include <juce_core/juce_core.h>

struct ButtonState {
    bool isHovered  = false;
    bool isDown     = false;
    bool isSelected = false;

    juce::String text;
};

#endif //SEQUENCETREE_BUTTONSTATE_H
