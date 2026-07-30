//
// Created by Eli Baumgardner on 7/28/26.
//

#ifndef SEQUENCETREE_GLYPHS_H
#define SEQUENCETREE_GLYPHS_H

#include <juce_gui_basics/juce_gui_basics.h>

enum class TriangleDirection { left, right, up, down };

inline void fillTriangle(juce::Graphics& g, juce::Rectangle<float> bounds, TriangleDirection direction)
{
    juce::Path triangle;

    switch (direction) {
        case TriangleDirection::left:
            triangle.startNewSubPath(bounds.getX(),       bounds.getCentreY());
            triangle.lineTo         (bounds.getRight(),   bounds.getY());
            triangle.lineTo         (bounds.getRight(),   bounds.getBottom());
            break;

        case TriangleDirection::right:
            triangle.startNewSubPath(bounds.getRight(),   bounds.getCentreY());
            triangle.lineTo         (bounds.getX(),       bounds.getBottom());
            triangle.lineTo         (bounds.getX(),       bounds.getY());
            break;

        case TriangleDirection::up:
            triangle.startNewSubPath(bounds.getCentreX(), bounds.getY());
            triangle.lineTo         (bounds.getRight(),   bounds.getBottom());
            triangle.lineTo         (bounds.getX(),       bounds.getBottom());
            break;

        case TriangleDirection::down:
            triangle.startNewSubPath(bounds.getCentreX(), bounds.getBottom());
            triangle.lineTo         (bounds.getX(),       bounds.getY());
            triangle.lineTo         (bounds.getRight(),   bounds.getY());
            break;
    }

    triangle.closeSubPath();
    g.fillPath(triangle);
}

#endif //SEQUENCETREE_GLYPHS_H
