//
// Created by Eli Baumgardner on 11/4/25.
//

#include "CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"
#include "CustomTextCaret.h"

void CustomLookAndFeel::drawCanvas(juce::Graphics &g, const NodeCanvas &canvas)
{

    g.fillAll(canvasColour.brighter());

    if (!canvas.gridVisible) {
        return;
    }

    float spacing = canvas.gridSpacing;
    if (spacing < 15.0f) {
        return;
    }

    auto bounds = canvas.getLocalBounds().toFloat();
    float ox;
    float oy;
    if (canvas.gridOriginSet) {
        ox = canvas.gridOrigin.x;
        oy = canvas.gridOrigin.y;
    }
    else {
        ox = bounds.getCentreX();
        oy = bounds.getCentreY();
    }


    const float armLen = 6.0f;
    g.setColour(gridColour);

    float startX = ox - std::ceil((ox - bounds.getX()) / spacing) * spacing;
    float startY = oy - std::ceil((oy - bounds.getY()) / spacing) * spacing;

    for (float x = startX; x <= bounds.getRight(); x += spacing)
    {
        for (float y = startY; y <= bounds.getBottom(); y += spacing)
        {
            g.drawLine(x - armLen, y, x + armLen, y, 0.5f);
            g.drawLine(x, y - armLen, x, y + armLen, 0.5f);
        }
    }
}

juce::CaretComponent* CustomLookAndFeel::createCaretComponent(juce::Component* keyFocusOwner) {
    auto* caret = new CustomTextCaret(keyFocusOwner);
    caret->caretWidth = 1.0f;
    return caret;
}

int CustomLookAndFeel::getDefaultScrollbarWidth()
{
    return scrollBarThickness;
}

void CustomLookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar&, int x, int y, int width, int height,
                                      bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                                      bool isMouseOver, bool isMouseDown)
{
    g.setColour(scrollBarTrackColour);
    g.fillRect(juce::Rectangle<int>(x, y, width, height));

    if (thumbSize <= 0) {
        return;
    }

    juce::Rectangle<int> thumb(thumbStartPosition, y, thumbSize, height);

    if (isScrollbarVertical) {
        thumb = juce::Rectangle<int>(x, thumbStartPosition, width, thumbSize);
    }

    g.setColour(scrollBarThumbColour);

    if (isMouseOver || isMouseDown) {
        g.setColour(scrollBarThumbHoverColour);
    }

    g.fillRoundedRectangle(thumb.toFloat().reduced(scrollBarThumbInset), scrollBarCornerRadius);
}
