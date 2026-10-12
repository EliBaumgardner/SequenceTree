#include "CustomLookAndFeel.h"
#include "../Buttons/IconButton.h"
#include "../Editors/FileLabel.h"
#include "../Buttons/ValueSlider.h"

enum class GlyphPaint { Stroked, Dashed, Filled, Tinted };

static void drawGlyph(juce::Graphics& graphics, juce::Rectangle<float> bounds, juce::Path glyph, GlyphPaint paint)
{
    const float          side     = juce::jmin(bounds.getWidth(), bounds.getHeight()) * Theme::glyphSizeRatio;
    const auto           area     = bounds.withSizeKeepingCentre(side, side);
    const float          scale    = side / Theme::glyphGridSize;
    const float          dashes[] = { 2.2f * scale, 2.4f * scale };
    juce::PathStrokeType stroke(side * Theme::glyphStrokeRatio, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

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

void CustomLookAndFeel::drawTreeIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.tree, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawNodeIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.nodeRing, GlyphPaint::Stroked);
    drawGlyph(graphics, bounds, glyphs.nodeDot, GlyphPaint::Filled);
}

void CustomLookAndFeel::drawTraversalIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.traversal, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawSettingsIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.settings, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawPlayIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    if (state.isSelected) {
        drawGlyph(graphics, bounds, glyphs.pause, GlyphPaint::Filled);

        return;
    }

    drawGlyph(graphics, bounds, glyphs.play, GlyphPaint::Filled);
}

void CustomLookAndFeel::drawSyncIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.sync, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawTempoIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.tempo, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawResetIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.resetBar, GlyphPaint::Stroked);
    drawGlyph(graphics, bounds, glyphs.resetTriangle, GlyphPaint::Filled);
}

void CustomLookAndFeel::drawResetPatchIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.resetPatch, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawUndoIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.undo, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawRedoIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.redo, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawNodeModeIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    if (state.isSelected) {
        drawGlyph(graphics, bounds, glyphs.nodeMode, GlyphPaint::Tinted);
    }

    drawGlyph(graphics, bounds, glyphs.nodeMode, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawModulatorIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    if (state.isSelected) {
        drawGlyph(graphics, bounds, glyphs.modulator, GlyphPaint::Tinted);
    }

    drawGlyph(graphics, bounds, glyphs.modulator, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawTraversalFlagIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    if (state.isSelected) {
        drawGlyph(graphics, bounds, glyphs.traversalFlag, GlyphPaint::Tinted);
    }

    drawGlyph(graphics, bounds, glyphs.traversalFlag, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawDisplayArrowIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.chevronDown, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawCollapseIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.chevronUp, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawEyeIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.eye, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawAddIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.add, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawRemoveIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.remove, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawImportIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.import, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawPaintToolIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.paintTip, GlyphPaint::Tinted);
    drawGlyph(graphics, bounds, glyphs.paintTip, GlyphPaint::Stroked);
    drawGlyph(graphics, bounds, glyphs.paintHandle, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawSpanToolIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.spanOutline, GlyphPaint::Dashed);
    drawGlyph(graphics, bounds, glyphs.spanEnds, GlyphPaint::Filled);
    drawGlyph(graphics, bounds, glyphs.spanBridge, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawQuaverToolIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.quaverHead, GlyphPaint::Filled);
    drawGlyph(graphics, bounds, glyphs.quaverStem, GlyphPaint::Stroked);
}

static void drawTitledButton(const CustomLookAndFeel& lookAndFeel, juce::Graphics& graphics, juce::Rectangle<float> bounds,
                             const ButtonState& state, const juce::Path& glyph, float fontHeight)
{
    const auto       area       = bounds.reduced(lookAndFeel.borderThickness * 0.5f);
    const juce::Font labelFont  = lookAndFeel.font(Theme::FontStyle::Regular, fontHeight);
    const float      glyphWidth = area.getHeight() * Theme::glyphSizeRatio;
    const float      gap        = fontHeight * 0.5f;
    const float      groupWidth = glyphWidth + gap + juce::GlyphArrangement::getStringWidth(labelFont, state.text);
    auto             content    = area.withSizeKeepingCentre(juce::jmin(groupWidth, area.getWidth()), area.getHeight());
    const auto       glyphSlot  = content.removeFromLeft(glyphWidth);
    juce::Colour     fill       = lookAndFeel.raisedColour;

    if (state.isHovered || state.isDown || state.isSelected) {
        fill = lookAndFeel.selectedColour;
    }

    graphics.setColour(fill);
    graphics.fillRoundedRectangle(area, lookAndFeel.paneCornerRadius);

    graphics.setColour(lookAndFeel.borderStrongColour);
    graphics.drawRoundedRectangle(area, lookAndFeel.paneCornerRadius, lookAndFeel.borderThickness);

    graphics.setColour(lookAndFeel.textColour);

    drawGlyph(graphics, glyphSlot.withSizeKeepingCentre(area.getHeight(), area.getHeight()), glyph, GlyphPaint::Stroked);

    content.removeFromLeft(gap);

    graphics.setFont(labelFont);
    graphics.drawFittedText(state.text, content.toNearestInt(), juce::Justification::centredLeft, 1);
}

void CustomLookAndFeel::drawHostButton(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    drawTitledButton(*this, graphics, bounds, state, glyphs.host, labelFontHeight);
}

void CustomLookAndFeel::drawDanglingArrowButton(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    drawTitledButton(*this, graphics, bounds, state, glyphs.danglingArrow, labelFontHeight);
}

void CustomLookAndFeel::drawPencilButton(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    drawTitledButton(*this, graphics, bounds, state, glyphs.pencil, labelFontHeight);
}

void CustomLookAndFeel::drawPublishButton(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    drawTitledButton(*this, graphics, bounds, state, glyphs.publish, labelFontHeight);
}

void CustomLookAndFeel::drawOrnamentButton(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.ornament, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawPencilIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.pencil, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawEraserIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    graphics.setColour(pressableButtonColour(state));

    drawGlyph(graphics, bounds, glyphs.eraser, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawAxisButton(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    ButtonState tileState = state;

    tileState.text = {};

    drawButtonTile(graphics, bounds, tileState);

    graphics.setColour(pressableButtonColour(state));
    graphics.setFont(font(FontStyle::SemiBold, textHeight));
    graphics.drawFittedText(state.text, bounds.toNearestInt(), juce::Justification::centred, 1);
}

void CustomLookAndFeel::drawRulesButton(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state,
                                        float fontHeight)
{
    drawTitledButton(*this, graphics, bounds, state, glyphs.rules, fontHeight);
}

void CustomLookAndFeel::drawIncrementIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, bool pointsUp)
{
    const float glyphSide = bounds.getHeight() * incrementGlyphScale;
    juce::Path  chevron   = glyphs.chevronDown;

    if (pointsUp) {
        chevron = glyphs.chevronUp;
    }

    graphics.setColour(dimTextColour);

    drawGlyph(graphics, bounds.withSizeKeepingCentre(glyphSide, glyphSide), chevron, GlyphPaint::Stroked);
}

void CustomLookAndFeel::drawValueSlider(juce::Graphics& graphics, const ValueSlider& slider)
{
    const auto   track      = slider.getLocalBounds().toFloat().reduced(popupMenuItemInset, popupMenuItemGap);
    const double value      = slider.range.snapToLegalValue(static_cast<double>(slider.boundValue.getValue()));
    const float  proportion = static_cast<float>(slider.range.convertTo0to1(value));
    const float  handleX    = track.getX() + track.getWidth() * proportion;
    const auto   textArea   = track.reduced(popupMenuTextInset, 0.0f).toNearestInt();
    juce::Colour border     = borderColour;
    juce::Path   trackShape;

    if (slider.isMouseOverOrDragging()) {
        border = borderStrongColour;
    }

    trackShape.addRoundedRectangle(track, paneCornerRadius);

    graphics.setColour(surfaceColour);
    graphics.fillPath(trackShape);

    {
        const juce::Graphics::ScopedSaveState savedState(graphics);

        graphics.reduceClipRegion(trackShape);
        graphics.setColour(accentSoftColour);
        graphics.fillRect(track.withRight(handleX));
        graphics.setColour(accentColour);
        graphics.fillRect(juce::Rectangle<float>(valueSliderHandleWidth, track.getHeight()).withCentre({ handleX, track.getCentreY() }));
    }

    graphics.setColour(border);
    graphics.strokePath(trackShape, juce::PathStrokeType(borderThickness));

    graphics.setFont(getPopupMenuFont());
    graphics.setColour(captionColour);
    graphics.drawFittedText(slider.label, textArea, juce::Justification::centredLeft, 1);
    graphics.setColour(textColour);
    graphics.drawFittedText(juce::String(value, 2) + slider.suffix, textArea, juce::Justification::centredRight, 1);
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

    const juce::Font labelFont   = font(FontStyle::Regular, textHeight);
    const float      glyphInset  = bounds.getHeight() * (1.0f - Theme::glyphSizeRatio) * 0.5f;
    const float      labelIndent = bounds.getHeight() * 0.85f - glyphInset;
    const float      groupWidth  = labelIndent + juce::GlyphArrangement::getStringWidth(labelFont, state.text);
    const auto       content     = bounds.withSizeKeepingCentre(juce::jmin(groupWidth, bounds.getWidth()), bounds.getHeight());

    graphics.setColour(pressableButtonColour(state));
    graphics.setFont(labelFont);
    graphics.drawFittedText(state.text, content.withTrimmedLeft(labelIndent).toNearestInt(), juce::Justification::centredLeft, 1);

    return content.withWidth(bounds.getHeight()).translated(-glyphInset, 0.0f);
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
