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

void CustomLookAndFeel::drawCanvas(juce::Graphics& graphics, const NodeCanvas& nodeCanvas)
{
    const float                     spacing = nodeCanvas.gridSpacing;
    const auto                      bounds  = nodeCanvas.getLocalBounds().toFloat().transformedBy(nodeCanvas.modelTransform);
    const auto                      visible = graphics.getClipBounds().toFloat().transformedBy(nodeCanvas.modelTransform).expanded(gridDotDiameter).getIntersection(bounds);
    const auto                      dot     = juce::Rectangle<float>(gridDotDiameter, gridDotDiameter);
    float                           originX = bounds.getCentreX();
    float                           originY = bounds.getCentreY();
    juce::Graphics::ScopedSaveState savedState(graphics);

    graphics.fillAll(canvasColour);

    if (!nodeCanvas.gridVisible || spacing < 15.0f) {
        return;
    }

    graphics.addTransform(nodeCanvas.viewTransform);

    if (nodeCanvas.gridOriginSet) {
        originX = nodeCanvas.gridOrigin.x;
        originY = nodeCanvas.gridOrigin.y;
    }

    const float firstX = originX - std::ceil((originX - visible.getX()) / spacing) * spacing;
    const float firstY = originY - std::ceil((originY - visible.getY()) / spacing) * spacing;

    graphics.setColour(gridColour);

    for (float dotX = firstX; dotX <= visible.getRight(); dotX += spacing) {
        for (float dotY = firstY; dotY <= visible.getBottom(); dotY += spacing) {
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

void CustomLookAndFeel::drawFrostedGlass(juce::Graphics& graphics, juce::Component& surface, NodeCanvas& nodeCanvas, juce::Rectangle<int> area)
{
    const int  margin         = juce::roundToInt(frostKernelSize / frostScale);
    const auto backdropArea   = nodeCanvas.getLocalBounds().expanded(margin);
    const auto backdropOrigin = surface.getLocalPoint(&nodeCanvas, backdropArea.getPosition()).toFloat();
    const auto frostedBounds  = (backdropArea.withZeroOrigin().toFloat() * frostScale).toNearestInt();

    juce::Graphics::ScopedSaveState savedState(graphics);

    if (! graphics.clipRegionIntersects(area)) {
        return;
    }

    if (frostedBackdrop.getBounds() != frostedBounds) {
        frostedBackdrop      = juce::Image(juce::Image::ARGB, frostedBounds.getWidth(), frostedBounds.getHeight(), true, juce::SoftwareImageType());
        frostedBackdropStale = true;
    }

    if (frostedBackdropStale) {
        juce::Graphics backdropGraphics(frostedBackdrop);

        frostedBackdrop.clear(frostedBackdrop.getBounds());

        backdropGraphics.addTransform(juce::AffineTransform::scale(frostScale));
        backdropGraphics.setOrigin(-backdropArea.getPosition());
        backdropGraphics.excludeClipRegion(unfrostedArea.reduced(margin));

        nodeCanvas.paint(backdropGraphics);

        for (juce::Component* child : nodeCanvas.getChildren()) {
            if (! child->isVisible() || ! backdropGraphics.clipRegionIntersects(child->getBoundsInParent())) {
                continue;
            }

            juce::Graphics::ScopedSaveState childState(backdropGraphics);

            backdropGraphics.addTransform(child->getTransform());
            backdropGraphics.setOrigin(child->getPosition());

            if (child->getAlpha() < 1.0f) {
                backdropGraphics.beginTransparencyLayer(child->getAlpha());
            }

            child->paint(backdropGraphics);

            if (child->getAlpha() < 1.0f) {
                backdropGraphics.endTransparencyLayer();
            }
        }

        blurFrostedImage(frostedBackdrop);

        frostedBackdropStale = false;
    }

    graphics.reduceClipRegion(area);
    graphics.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    graphics.drawImageTransformed(frostedBackdrop, juce::AffineTransform::scale(1.0f / frostScale).translated(backdropOrigin));

    graphics.setColour(barColour);
    graphics.fillRect(area);
}

void CustomLookAndFeel::blurFrostedImage(juce::Image& frosted)
{
    const int                          centreTap    = frostKernelSize / 2;
    const double                       radiusFactor = -1.0 / (frostBlurRadius * frostBlurRadius * 2.0);
    juce::Image::BitmapData            pixels(frosted, juce::Image::BitmapData::readWrite);
    const int                          rowLength    = pixels.width * pixels.pixelStride;
    std::vector<float>                 rowPass(static_cast<size_t>(rowLength * pixels.height), 0.0f);
    std::vector<float>                 columnPass(static_cast<size_t>(rowLength), 0.0f);
    std::array<float, frostKernelSize> weights {};
    float                              weightSum    = 0.0f;

    for (int tap = 0; tap < frostKernelSize; ++tap) {
        const int offset = tap - centreTap;

        weights[static_cast<size_t>(tap)]  = static_cast<float>(std::exp(radiusFactor * offset * offset));
        weightSum                         += weights[static_cast<size_t>(tap)];
    }

    for (float& weight : weights) {
        weight /= weightSum;
    }

    for (int y = 0; y < pixels.height; ++y) {
        for (int tap = 0; tap < frostKernelSize; ++tap) {
            const int                shift   = tap - centreTap;
            const int                firstX  = std::max(0, -shift);
            const int                lastX   = std::min(pixels.width, pixels.width - shift);
            const float              weight  = weights[static_cast<size_t>(tap)];
            const juce::uint8* const source  = pixels.getLinePointer(y) + (firstX + shift) * pixels.pixelStride;
            float* const             blurred = rowPass.data() + y * rowLength + firstX * pixels.pixelStride;

            for (int index = 0; index < (lastX - firstX) * pixels.pixelStride; ++index) {
                blurred[index] += weight * source[index];
            }
        }
    }

    for (int y = 0; y < pixels.height; ++y) {
        juce::uint8* const target = pixels.getLinePointer(y);

        std::fill(columnPass.begin(), columnPass.end(), 0.0f);

        for (int tap = 0; tap < frostKernelSize; ++tap) {
            const int sourceY = y + tap - centreTap;

            if (sourceY < 0 || sourceY >= pixels.height) {
                continue;
            }

            const float        weight = weights[static_cast<size_t>(tap)];
            const float* const source = rowPass.data() + sourceY * rowLength;

            for (int index = 0; index < rowLength; ++index) {
                columnPass[static_cast<size_t>(index)] += weight * source[index];
            }
        }

        for (int index = 0; index < rowLength; ++index) {
            target[index] = static_cast<juce::uint8>(juce::jmin(0xff, juce::roundToInt(columnPass[static_cast<size_t>(index)])));
        }
    }
}
