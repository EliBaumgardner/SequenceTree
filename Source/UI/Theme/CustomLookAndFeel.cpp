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
    const float                     spacing = canvas.gridSpacing;
    const auto                      bounds  = canvas.getLocalBounds().toFloat().transformedBy(canvas.modelTransform);
    const auto                      dot     = juce::Rectangle<float>(gridDotDiameter, gridDotDiameter);
    float                           originX = bounds.getCentreX();
    float                           originY = bounds.getCentreY();
    juce::Graphics::ScopedSaveState savedState(graphics);

    graphics.fillAll(canvasColour);

    if (!canvas.gridVisible || spacing < 15.0f) {
        return;
    }

    graphics.addTransform(canvas.viewTransform);

    if (canvas.gridOriginSet) {
        originX = canvas.gridOrigin.x;
        originY = canvas.gridOrigin.y;
    }

    const float firstX = originX - std::ceil((originX - bounds.getX()) / spacing) * spacing;
    const float firstY = originY - std::ceil((originY - bounds.getY()) / spacing) * spacing;

    graphics.setColour(gridColour);

    for (float dotX = firstX; dotX <= bounds.getRight(); dotX += spacing) {
        for (float dotY = firstY; dotY <= bounds.getBottom(); dotY += spacing) {
            graphics.fillEllipse(dot.withCentre({ dotX, dotY }));
        }
    }
}

void CustomLookAndFeel::drawPane(juce::Graphics& graphics, juce::Rectangle<float> bounds)
{
    const auto area = bounds.reduced(borderThickness * 0.5f);

    graphics.setColour(surfaceColour);
    graphics.fillRoundedRectangle(area, groupCornerRadius);

    graphics.setColour(borderColour);
    graphics.drawRoundedRectangle(area, groupCornerRadius, borderThickness);
}

void CustomLookAndFeel::drawFrostedGlass(juce::Graphics& graphics, juce::Component& surface, juce::Component& backdrop, juce::Rectangle<int> area)
{
    const int                       margin       = juce::roundToInt(static_cast<float>(frostKernelSize) / frostScale);
    const auto                      backdropArea = backdrop.getLocalArea(&surface, area).expanded(margin);
    juce::Image                     frosted      = backdrop.createComponentSnapshot(backdropArea, false, frostScale);
    juce::ImageConvolutionKernel    kernel(frostKernelSize);
    juce::Graphics::ScopedSaveState savedState(graphics);

    kernel.createGaussianBlur(frostBlurRadius);
    kernel.applyToImage(frosted, frosted, frosted.getBounds());

    graphics.reduceClipRegion(area);
    graphics.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    graphics.drawImage(frosted, area.expanded(margin).toFloat());

    graphics.setColour(barColour);
    graphics.fillRect(area);
}
