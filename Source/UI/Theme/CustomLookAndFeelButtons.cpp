#include "CustomLookAndFeel.h"
#include "../Buttons/IconButton.h"
#include "../Editors/FileLabel.h"

enum class GlyphPaint { Stroked, Dashed, Filled, Tinted };

juce::Rectangle<float> CustomLookAndFeel::drawButtonTile(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    juce::Colour fill = juce::Colours::transparentBlack;

    if (state.isHovered) {
        fill = hoverColour;
    }

    if (state.isDown) {
        fill = hoverColour.withMultipliedAlpha(2.0f);
    }

    if (state.isSelected) {
        switch (state.look) {
            case ButtonState::Look::Tinted: fill = accentSoftColour; break;
            case ButtonState::Look::Accent: fill = accentColour;     break;
            case ButtonState::Look::Raised: fill = selectedColour;   break;
        }
    }

    graphics.setColour(fill);
    graphics.fillRoundedRectangle(bounds, paneCornerRadius);

    if (state.isSelected && state.look == ButtonState::Look::Raised) {
        graphics.setColour(borderColour);
        graphics.drawRoundedRectangle(bounds.reduced(borderThickness * 0.5f), paneCornerRadius, borderThickness);
    }

    if (state.text.isEmpty()) {
        return bounds;
    }

    graphics.setColour(pressableButtonColour(state));
    graphics.setFont(font(FontStyle::Regular, textHeight));
    graphics.drawFittedText(state.text, bounds.withTrimmedLeft(bounds.getHeight() * 0.85f).toNearestInt(), juce::Justification::centredLeft, 1);

    return bounds.withWidth(bounds.getHeight());
}

juce::Colour CustomLookAndFeel::pressableButtonColour(const ButtonState& state) const
{
    if (state.isSelected) {
        switch (state.look) {
            case ButtonState::Look::Tinted: return accentColour;
            case ButtonState::Look::Accent: return onAccentColour;
            case ButtonState::Look::Raised: return textColour;
        }
    }

    if (state.isHovered || state.isDown) {
        return textColour;
    }

    return softTextColour;
}

static void drawGlyph(juce::Graphics& graphics, juce::Rectangle<float> bounds, const juce::String& pathData, GlyphPaint paint)
{
    const float          side     = juce::jmin(bounds.getWidth(), bounds.getHeight()) * Theme::glyphSizeRatio;
    const auto           area     = bounds.withSizeKeepingCentre(side, side);
    const float          scale    = side / Theme::glyphGridSize;
    const float          dashes[] = { 2.2f * scale, 2.4f * scale };
    juce::PathStrokeType stroke(side * Theme::glyphStrokeRatio, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    juce::Path           glyph    = juce::Drawable::parseSVGPath(pathData);

    glyph.applyTransform(juce::AffineTransform::scale(scale).translated(area.getX(), area.getY()));

    switch (paint) {
        case GlyphPaint::Stroked:
            graphics.strokePath(glyph, stroke);
            break;

        case GlyphPaint::Dashed:
            stroke.createDashedStroke(glyph, glyph, dashes, 2);
            graphics.fillPath(glyph);
            break;

        case GlyphPaint::Filled:
            graphics.fillPath(glyph);
            break;

        case GlyphPaint::Tinted: {
            const juce::Graphics::ScopedSaveState savedState(graphics);

            graphics.setOpacity(Theme::glyphFillAlpha);
            graphics.fillPath(glyph);
            break;
        }
    }
}

void CustomLookAndFeel::drawNodeIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, "M4 12a8 8 0 1 0 16 0a8 8 0 1 0-16 0z", GlyphPaint::Stroked);
    drawGlyph(graphics, bounds, "M9.2 12a2.8 2.8 0 1 0 5.6 0a2.8 2.8 0 1 0-5.6 0z", GlyphPaint::Filled);
}

void CustomLookAndFeel::drawTreeIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, "M9.7 5a2.3 2.3 0 1 0 4.6 0a2.3 2.3 0 1 0-4.6 0zM3.7 18.5a2.3 2.3 0 1 0 4.6 0a2.3 2.3 0 1 0-4.6 0z"
                                "M15.7 18.5a2.3 2.3 0 1 0 4.6 0a2.3 2.3 0 1 0-4.6 0zM11 7.1 7 16.3M13 7.1l4 9.2", GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawTraversalIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, "M3.2 18.5a2.3 2.3 0 1 0 4.6 0a2.3 2.3 0 1 0-4.6 0zM16.2 5.5a2.3 2.3 0 1 0 4.6 0a2.3 2.3 0 1 0-4.6 0z"
                                "M8 18.5h7.5a3.5 3.5 0 0 0 0-7h-7a3.5 3.5 0 0 1 0-7H16", GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawPlayIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    if (state.isSelected) {
        drawGlyph(graphics, bounds, "M7.4 5h1.2a1.4 1.4 0 0 1 1.4 1.4v11.2a1.4 1.4 0 0 1-1.4 1.4H7.4A1.4 1.4 0 0 1 6 17.6V6.4A1.4 1.4 0 0 1 7.4 5z"
                                    "M15.4 5h1.2a1.4 1.4 0 0 1 1.4 1.4v11.2a1.4 1.4 0 0 1-1.4 1.4h-1.2a1.4 1.4 0 0 1-1.4-1.4V6.4A1.4 1.4 0 0 1 15.4 5z",
                  GlyphPaint::Filled);
        return;
    }

    drawGlyph(graphics, bounds, "M7.5 5.6v12.8a1 1 0 0 0 1.53.85l10.2-6.4a1 1 0 0 0 0-1.7L9.03 4.75A1 1 0 0 0 7.5 5.6z", GlyphPaint::Filled);
}

void CustomLookAndFeel::drawNodeModeIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    const juce::String circle = "M5 12a7 7 0 1 0 14 0a7 7 0 1 0-14 0z";

    graphics.setColour(pressableButtonColour(state));

    if (state.isSelected) {
        drawGlyph(graphics, bounds, circle, GlyphPaint::Tinted);
    }

    drawGlyph(graphics, bounds, circle, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawModulatorIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    const juce::String square = "M8 5.5h8a2.5 2.5 0 0 1 2.5 2.5v8a2.5 2.5 0 0 1-2.5 2.5H8a2.5 2.5 0 0 1-2.5-2.5V8A2.5 2.5 0 0 1 8 5.5z";

    graphics.setColour(pressableButtonColour(state));

    if (state.isSelected) {
        drawGlyph(graphics, bounds, square, GlyphPaint::Tinted);
    }

    drawGlyph(graphics, bounds, square, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawTraversalFlagIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    const juce::String triangle = "M12 4.8 19.6 18.5H4.4z";

    graphics.setColour(pressableButtonColour(state));

    if (state.isSelected) {
        drawGlyph(graphics, bounds, triangle, GlyphPaint::Tinted);
    }

    drawGlyph(graphics, bounds, triangle, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawDisplayArrowIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, "m6 9 6 6 6-6", GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawIncrementIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, bool pointsUp)
{
    const float  glyphSide = bounds.getHeight() * incrementGlyphScale;
    juce::String chevron   = "m6 9 6 6 6-6";

    if (pointsUp) {
        chevron = "m6 15 6-6 6 6";
    }

    graphics.setColour(dimTextColour);

    drawGlyph(graphics, bounds.withSizeKeepingCentre(glyphSide, glyphSide), chevron, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawEyeIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, "M2.5 12s3.5-6.5 9.5-6.5 9.5 6.5 9.5 6.5-3.5 6.5-9.5 6.5S2.5 12 2.5 12z"
                                "M9.2 12a2.8 2.8 0 1 0 5.6 0a2.8 2.8 0 1 0-5.6 0z", GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawTempoIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, "M9.2 3.5h5.6L19 20.5H5zM12 16.5l4.5-8M7 15.5h10", GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawRulesButton(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state,
                                        float fontHeight)
{
    const auto       area       = bounds.reduced(borderThickness * 0.5f);
    const juce::Font labelFont  = font(FontStyle::Regular, fontHeight);
    const float      glyphWidth = area.getHeight() * glyphSizeRatio;
    const float      gap        = fontHeight * 0.5f;
    const float      groupWidth = glyphWidth + gap + juce::GlyphArrangement::getStringWidth(labelFont, state.text);
    auto             content    = area.withSizeKeepingCentre(juce::jmin(groupWidth, area.getWidth()), area.getHeight());
    const auto       glyphSlot  = content.removeFromLeft(glyphWidth);
    juce::Colour     fill       = raisedColour;

    if (state.isHovered || state.isDown) {
        fill = selectedColour;
    }

    graphics.setColour(fill);
    graphics.fillRoundedRectangle(area, paneCornerRadius);

    graphics.setColour(borderStrongColour);
    graphics.drawRoundedRectangle(area, paneCornerRadius, borderThickness);

    graphics.setColour(textColour);

    drawGlyph(graphics, glyphSlot.withSizeKeepingCentre(area.getHeight(), area.getHeight()), "m8 7-5 5 5 5M16 7l5 5-5 5", GlyphPaint::Stroked);

    content.removeFromLeft(gap);

    graphics.setFont(labelFont);
    graphics.drawFittedText(state.text, content.toNearestInt(), juce::Justification::centredLeft, 1);
}

void CustomLookAndFeel::drawAddIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, "M12 5v14M5 12h14", GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawRemoveIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, "M5 12h14", GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawUndoIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, "M9 14 4 9l5-5M4 9h10.5a5.5 5.5 0 0 1 0 11H11", GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawRedoIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, "m15 14 5-5-5-5M20 9H9.5a5.5 5.5 0 0 0 0 11H13", GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawResetIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, "M6 5.5v13", GlyphPaint::Stroked);
    drawGlyph(graphics, bounds, "M18.5 6.4v11.2a.9.9 0 0 1-1.4.74L9.4 13a1.2 1.2 0 0 1 0-2l7.7-5.34a.9.9 0 0 1 1.4.74z", GlyphPaint::Filled);
}

void CustomLookAndFeel::drawSyncIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, "M10 13a5 5 0 0 0 7.54.54l3-3a5 5 0 0 0-7.07-7.07l-1.72 1.71"
                                "M14 11a5 5 0 0 0-7.54-.54l-3 3a5 5 0 0 0 7.07 7.07l1.71-1.71", GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawPaintToolIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    const juce::String tip = "M4 20.5c2.8 0 4.6-1.3 4.6-3.4a2.3 2.3 0 0 0-4.6 0c0 1.4-.4 2.4-1 3.4z";

    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, tip, GlyphPaint::Tinted);
    drawGlyph(graphics, bounds, "M9.3 14.7 19.2 4.8a1.7 1.7 0 0 1 2.4 2.4l-9.9 9.9" + tip, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawArrowToolIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, "M2.7 17a2.3 2.3 0 1 0 4.6 0a2.3 2.3 0 1 0-4.6 0z", GlyphPaint::Filled);
    drawGlyph(graphics, bounds, "M7 15 18 5M11.5 5H18v6.5", GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawSpanToolIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, "M5.5 6.5h13a3 3 0 0 1 3 3v5a3 3 0 0 1-3 3h-13a3 3 0 0 1-3-3v-5a3 3 0 0 1 3-3z", GlyphPaint::Dashed);
    drawGlyph(graphics, bounds, "M5 12a2 2 0 1 0 4 0a2 2 0 1 0-4 0zM15 12a2 2 0 1 0 4 0a2 2 0 1 0-4 0z", GlyphPaint::Filled);
    drawGlyph(graphics, bounds, "M9 12h6", GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawQuaverToolIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, "M5.4 18.63a3.3 2.5 -20 1 0 6.2-2.26a3.3 2.5 -20 1 0-6.2 2.26z", GlyphPaint::Filled);
    drawGlyph(graphics, bounds, "M11.5 17V3.5c1.2 2.6 5.8 3.4 5.8 7.6", GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawNodeArrowIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, "M2.1 12a2.4 2.4 0 1 0 4.8 0a2.4 2.4 0 1 0-4.8 0zM15 7.5 21 12l-6 4.5z", GlyphPaint::Filled);
    drawGlyph(graphics, bounds, "M7.5 12h8M15 7.5 21 12l-6 4.5z", GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawPolyphonicArrowIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, "M2.1 12a2.4 2.4 0 1 0 4.8 0a2.4 2.4 0 1 0-4.8 0z", GlyphPaint::Filled);
    drawGlyph(graphics, bounds, "M7.5 12h9m-5-4.5 4.5 4.5-4.5 4.5M16 7.5l4.5 4.5-4.5 4.5", GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawTraversalArrowIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, "M2.1 12a2.4 2.4 0 1 0 4.8 0a2.4 2.4 0 1 0-4.8 0z", GlyphPaint::Filled);
    drawGlyph(graphics, bounds, "M7.5 12h7.5M15 7.5 21 12l-6 4.5z", GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawFileLabel(juce::Graphics& graphics, const FileLabel& fileLabel)
{
    auto bounds = fileLabel.getLocalBounds().toFloat();

    juce::Colour background = surfaceColour;

    if (fileLabel.selected) {
        background = raisedColour;
    }

    if (fileLabel.grabbed) {
        background = selectedColour;
    }

    graphics.setColour(background);
    graphics.fillRect(bounds);

    if (fileLabel.selected) {
        graphics.setColour(accentColour);
        graphics.fillRect(bounds.withWidth(fileLabelMarkerWidth));
    }

    graphics.setColour(borderColour);
    graphics.drawHorizontalLine(static_cast<int>(bounds.getBottom()) - 1, bounds.getX(), bounds.getRight());
}
