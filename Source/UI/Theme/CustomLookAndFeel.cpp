#include "CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"
#include "CustomTextCaret.h"

int CustomLookAndFeel::getDefaultScrollbarWidth()
{
    return scrollBarThickness;
}

void CustomLookAndFeel::drawScrollbar(juce::Graphics& graphics, juce::ScrollBar&, int x, int y, int width, int height,
                                      bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                                      bool isMouseOver, bool isMouseDown)
{
    graphics.setColour(scrollBarTrackColour);
    graphics.fillRect(juce::Rectangle<int>(x, y, width, height));

    if (thumbSize <= 0) {
        return;
    }

    juce::Rectangle<int> thumb(thumbStartPosition, y, thumbSize, height);

    if (isScrollbarVertical) {
        thumb = juce::Rectangle<int>(x, thumbStartPosition, width, thumbSize);
    }

    graphics.setColour(scrollBarThumbColour);

    if (isMouseOver || isMouseDown) {
        graphics.setColour(scrollBarThumbHoverColour);
    }

    graphics.fillRoundedRectangle(thumb.toFloat().reduced(scrollBarThumbInset), scrollBarCornerRadius);
}

juce::CaretComponent* CustomLookAndFeel::createCaretComponent(juce::Component* keyFocusOwner)
{
    auto caret = std::make_unique<CustomTextCaret>(keyFocusOwner);
    caret->caretWidth = 1.0f;
    return caret.release();
}

void CustomLookAndFeel::drawCanvas(juce::Graphics& graphics, const NodeCanvas& canvas)
{
    const float spacing      = canvas.gridSpacing;
    const auto  bounds       = canvas.getLocalBounds().toFloat();
    const float crossArm     = 6.0f;
    float       originX      = bounds.getCentreX();
    float       originY      = bounds.getCentreY();

    graphics.fillAll(canvasColour.brighter());

    if (!canvas.gridVisible || spacing < 15.0f) {
        return;
    }

    if (canvas.gridOriginSet) {
        originX = canvas.gridOrigin.x;
        originY = canvas.gridOrigin.y;
    }

    const float firstX = originX - std::ceil((originX - bounds.getX()) / spacing) * spacing;
    const float firstY = originY - std::ceil((originY - bounds.getY()) / spacing) * spacing;

    graphics.setColour(gridColour);

    for (float crossX = firstX; crossX <= bounds.getRight(); crossX += spacing) {
        for (float crossY = firstY; crossY <= bounds.getBottom(); crossY += spacing) {
            graphics.drawLine(crossX - crossArm, crossY, crossX + crossArm, crossY, 0.5f);
            graphics.drawLine(crossX, crossY - crossArm, crossX, crossY + crossArm, 0.5f);
        }
    }
}
