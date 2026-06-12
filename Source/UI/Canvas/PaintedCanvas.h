//
// Created by Eli Baumgardner on 6/10/26.
//

#ifndef SEQUENCETREE_PAINTEDCANVAS_H
#define SEQUENCETREE_PAINTEDCANVAS_H

#include <juce_gui_basics/juce_gui_basics.h>

class ApplicationContext;

class PaintedCanvas : juce::Component {
public:

    PaintedCanvas(ApplicationContext& context);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void paintStroke(juce::Point<float> canvasPos, bool isStart);
    void setBrushColour(juce::Colour colour);
    void drawPaintCursor(bool paintMode);

    juce::Image image;
    juce::Colour canvasColour;
    ApplicationContext& context;

    juce::Point<int> paintPoint;

};


#endif //SEQUENCETREE_PAINTEDCANVAS_H