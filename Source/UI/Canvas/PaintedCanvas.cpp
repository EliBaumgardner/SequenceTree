//
// Created by Eli Baumgardner on 6/10/26.
//

#include "PaintedCanvas.h"
#include "../Util/ApplicationContext.h""

PaintedCanvas::PaintedCanvas(ApplicationContext &context) : context(context){

}

void PaintedCanvas::paint(juce::Graphics& g) {
    g.drawImageAt(image, 0, 0);
}

void PaintedCanvas::resized() {


}



void PaintedCanvas::drawPaintCursor(bool paintMode) {

    auto createMouseCursor = [](int size) {
        juce::Image img(juce::Image::ARGB, size, size, true);
        juce::Graphics g(img);
        g.setColour(juce::Colours::black);
        g.drawEllipse(img.getBounds().toFloat().reduced(1.0f), 1.0f);

        const int hotspot = size / 2;
        return juce::MouseCursor(img, hotspot, hotspot);
    };

    if (paintMode) {
        setMouseCursor(juce::MouseCursor(createMouseCursor(24)));
    }
    else {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

