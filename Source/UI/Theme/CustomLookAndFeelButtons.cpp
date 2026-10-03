#include "CustomLookAndFeel.h"
#include "../Buttons/IconButton.h"
#include "../Editors/FileLabel.h"

enum class TriangleDirection { left, right, up, down };

void CustomLookAndFeel::drawNodeIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState&)
{
    graphics.setColour(buttonColour);
    graphics.fillEllipse(bounds);

    auto glyphBounds = bounds.reduced(bounds.getWidth() * 0.32f);
    graphics.setColour(juce::Colours::black);
    graphics.fillEllipse(glyphBounds);
}

void CustomLookAndFeel::drawTreeIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState&)
{
    graphics.setColour(buttonColour);
    graphics.fillEllipse(bounds);

    juce::Point<float> centre = bounds.getCentre();
    const float outerRadius = bounds.getWidth() * 0.5f;

    const float lineThickness   = 2.0f;
    const float nodeRadius      = outerRadius * 0.24f;
    const float insetMargin     = outerRadius * 0.15f;
    const float placementRadius = outerRadius - nodeRadius - insetMargin;

    juce::Point<float> rootPos (centre.x, centre.y - placementRadius * 0.55f);
    juce::Point<float> leftPos (centre.x - placementRadius * 0.8f, centre.y + placementRadius * 0.55f);
    juce::Point<float> rightPos(centre.x + placementRadius * 0.8f, centre.y + placementRadius * 0.55f);

    graphics.setColour(juce::Colours::black);
    graphics.drawLine(juce::Line<float>(rootPos, leftPos), lineThickness);
    graphics.drawLine(juce::Line<float>(rootPos, rightPos), lineThickness);

    auto nodeCircle = [&](juce::Point<float> pos) {
        graphics.fillEllipse(juce::Rectangle<float>(nodeRadius * 2.0f, nodeRadius * 2.0f).withCentre(pos));
    };
    nodeCircle(rootPos);
    nodeCircle(leftPos);
    nodeCircle(rightPos);
}

static void fillTriangle(juce::Graphics& graphics, juce::Rectangle<float> bounds, TriangleDirection direction)
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
    graphics.fillPath(triangle);
}

static void fillArrowGlyph(juce::Graphics& graphics, juce::Rectangle<float> glyphArea,
                    float shaftThicknessFactor, float headLengthFactor, float headWidthFactor)
{
    juce::Point<float> tail(glyphArea.getX(),     glyphArea.getCentreY());
    juce::Point<float> head(glyphArea.getRight(), glyphArea.getCentreY());

    juce::Path shaft;
    shaft.addLineSegment(juce::Line<float>(tail, head), glyphArea.getHeight() * shaftThicknessFactor);
    graphics.fillPath(shaft);

    const float headLength = glyphArea.getWidth()  * headLengthFactor;
    const float headWidth  = glyphArea.getHeight() * headWidthFactor;

    fillTriangle(graphics, { head.x - headLength, head.y - headWidth, headLength, headWidth * 2.0f }, TriangleDirection::right);
}

void CustomLookAndFeel::drawTraversalIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState&)
{
    graphics.setColour(buttonColour);
    graphics.fillEllipse(bounds);

    auto glyphArea = bounds.reduced(bounds.getWidth() * 0.26f, bounds.getHeight() * 0.38f);

    graphics.setColour(juce::Colours::black);
    fillArrowGlyph(graphics, glyphArea, 0.24f, 0.4f, 0.65f);
}

juce::Colour CustomLookAndFeel::pressableButtonColour(const ButtonState& state) const
{
    if (state.isDown) {
        return buttonColour.darker();
    }

    if (state.isHovered) {
        return buttonColour.brighter();
    }

    return buttonColour;
}

constexpr float transportGlyphInsetRatio = 0.2f;

void CustomLookAndFeel::drawPlayIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    const auto area = bounds.reduced(bounds.getWidth() * transportGlyphInsetRatio);

    graphics.setColour(pressableButtonColour(state));

    if (state.isSelected) {
        fillTriangle(graphics, area, TriangleDirection::right);
        return;
    }

    const float barWidth = area.getWidth() / 5.0f;

    graphics.fillRect(area.withWidth(barWidth).withX(area.getX() + barWidth));
    graphics.fillRect(area.withWidth(barWidth).withX(area.getX() + barWidth * 3.0f));
}

void CustomLookAndFeel::drawNodeModeIcon(juce::Graphics& graphics, juce::Rectangle<float> boundsIn, const ButtonState& state)
{
    auto bounds = boundsIn.reduced(outerButtonBoundsReduction);

    graphics.setColour(buttonColour);

    if (state.isSelected) {
        graphics.setColour(buttonColour.darker());
    }

    graphics.fillEllipse(bounds);
}

void CustomLookAndFeel::drawModulatorIcon(juce::Graphics& graphics, juce::Rectangle<float> boundsIn, const ButtonState& state)
{
    juce::Rectangle<float> bounds = boundsIn.reduced(outerButtonBoundsReduction);

    graphics.setColour(buttonColour);

    if (state.isSelected) {
        graphics.setColour(buttonColour.darker());
    }

    graphics.fillRect(bounds);
    graphics.drawRect(bounds, 1.0f);
}

void CustomLookAndFeel::drawTraversalFlagIcon(juce::Graphics& graphics, juce::Rectangle<float> boundsIn, const ButtonState& state)
{
    juce::Rectangle<float> bounds = boundsIn.reduced(outerButtonBoundsReduction);

    graphics.setColour(buttonColour);

    if (state.isSelected) {
        graphics.setColour(buttonColour.darker());
    }

    juce::Path triangle;
    triangle.startNewSubPath(bounds.getCentreX(), bounds.getY());
    triangle.lineTo(bounds.getRight(), bounds.getBottom());
    triangle.lineTo(bounds.getX(), bounds.getBottom());
    triangle.closeSubPath();

    graphics.fillPath(triangle);
    graphics.strokePath(triangle, juce::PathStrokeType(1.0f));
}

void CustomLookAndFeel::drawDisplayArrowIcon(juce::Graphics& graphics, juce::Rectangle<float> boundsIn, const ButtonState& state)
{
    auto bounds = boundsIn.reduced(outerButtonBoundsReduction);

    graphics.setColour(buttonColour);

    if (state.isSelected) {
        graphics.setColour(buttonColour.darker());
    }

    fillTriangle(graphics, bounds.withSizeKeepingCentre(bounds.getWidth(), bounds.getHeight() * 0.9f), TriangleDirection::down);
}

void CustomLookAndFeel::drawIncrementIcon(juce::Graphics& graphics, juce::Rectangle<float> boundsIn, bool pointsUp)
{
    graphics.setColour(juce::Colours::black);

    TriangleDirection direction = TriangleDirection::down;

    if (pointsUp) {
        direction = TriangleDirection::up;
    }

    fillTriangle(graphics, boundsIn.reduced(boundsIn.getWidth() * incrementIconWidthInset, boundsIn.getHeight() * incrementIconHeightInset), direction);
}

void CustomLookAndFeel::drawTextButton(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state,
                                       float fontHeight)
{
    auto area = bounds.reduced(outerButtonBoundsReduction);

    graphics.setColour(pressableButtonColour(state));

    if (state.isSelected) {
        graphics.setColour(buttonColour.darker());
    }

    graphics.fillRoundedRectangle(area, paneCornerRadius);

    graphics.setColour(juce::Colours::black.withAlpha(0.5f));
    graphics.drawRoundedRectangle(area, paneCornerRadius, 1.0f);

    graphics.setColour(juce::Colours::black.withAlpha(0.8f));
    graphics.setFont(juce::Font(juce::FontOptions(fontHeight)));
    graphics.drawFittedText(state.text, area.reduced(innerButtonBoundsReduction, 0.0f).toNearestInt(), juce::Justification::centred, 1);
}

constexpr float squareGlyphInsetRatio     = 0.18f;
constexpr float squareGlyphOutlineRatio   = 0.06f;
constexpr float squareGlyphBarInsetRatio  = 0.28f;
constexpr float squareGlyphBarWidthRatio  = 0.12f;

void CustomLookAndFeel::drawAddIcon(juce::Graphics& graphics, juce::Rectangle<float> boundsIn, const ButtonState& state)
{
    const float side      = juce::jmin(boundsIn.getWidth(), boundsIn.getHeight()) * (1.0f - squareGlyphInsetRatio * 2.0f);
    const auto  square    = boundsIn.withSizeKeepingCentre(side, side);
    const auto  bar       = square.reduced(side * squareGlyphBarInsetRatio);
    const float thickness = juce::jmax(1.0f, side * squareGlyphBarWidthRatio);

    graphics.setColour(pressableButtonColour(state));

    graphics.drawRect(square, juce::jmax(1.0f, side * squareGlyphOutlineRatio));
    graphics.fillRect(bar.withSizeKeepingCentre(bar.getWidth(), thickness));
    graphics.fillRect(bar.withSizeKeepingCentre(thickness, bar.getHeight()));
}

void CustomLookAndFeel::drawRemoveIcon(juce::Graphics& graphics, juce::Rectangle<float> boundsIn, const ButtonState& state)
{
    const float side      = juce::jmin(boundsIn.getWidth(), boundsIn.getHeight()) * (1.0f - squareGlyphInsetRatio * 2.0f);
    const auto  square    = boundsIn.withSizeKeepingCentre(side, side);
    const auto  bar       = square.reduced(side * squareGlyphBarInsetRatio);
    const float thickness = juce::jmax(1.0f, side * squareGlyphBarWidthRatio);

    graphics.setColour(pressableButtonColour(state));

    graphics.drawRect(square, juce::jmax(1.0f, side * squareGlyphOutlineRatio));
    graphics.fillRect(bar.withSizeKeepingCentre(bar.getWidth(), thickness));
}

void CustomLookAndFeel::drawUndoIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    auto area = bounds.reduced(outerButtonBoundsReduction);

    graphics.setColour(pressableButtonColour(state));

    fillTriangle(graphics, area, TriangleDirection::left);
}

void CustomLookAndFeel::drawRedoIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    auto area = bounds.reduced(outerButtonBoundsReduction);

    graphics.setColour(pressableButtonColour(state));

    fillTriangle(graphics, area, TriangleDirection::right);
}

void CustomLookAndFeel::drawResetIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    auto area = bounds.reduced(bounds.getWidth() * transportGlyphInsetRatio);

    const float gap           = area.getWidth() * 0.1f;
    const float triangleWidth = (area.getWidth() - gap) * 0.5f;

    graphics.setColour(pressableButtonColour(state));

    fillTriangle(graphics, area.removeFromLeft (triangleWidth), TriangleDirection::left);
    fillTriangle(graphics, area.removeFromRight(triangleWidth), TriangleDirection::left);
}

void CustomLookAndFeel::drawSyncIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    const auto  area       = bounds.reduced(bounds.getWidth() * transportGlyphInsetRatio);
    const float diameter   = juce::jmin(area.getWidth(), area.getHeight());
    const auto  circle     = area.withSizeKeepingCentre(diameter, diameter);
    const auto  glyphArea  = circle.reduced(diameter * 0.14f, diameter * 0.3f);
    const float headLength = glyphArea.getWidth() * 0.28f;
    const float centreY    = glyphArea.getCentreY();
    juce::Colour circleColour = pressableButtonColour(state);
    juce::Path   shaft;

    if (state.isSelected) {
        circleColour = circleColour.brighter(0.6f);
    }

    graphics.setColour(circleColour);
    graphics.fillEllipse(circle);

    graphics.setColour(juce::Colours::black);

    shaft.addLineSegment({ glyphArea.getX() + headLength, centreY, glyphArea.getRight() - headLength, centreY }, glyphArea.getHeight() * 0.25f);
    graphics.fillPath(shaft);

    fillTriangle(graphics, glyphArea.withWidth(headLength), TriangleDirection::left);
    fillTriangle(graphics, glyphArea.withTrimmedLeft(glyphArea.getWidth() - headLength), TriangleDirection::right);
}

void CustomLookAndFeel::drawPaintToolIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    auto area = bounds.reduced(outerButtonBoundsReduction);
    if (state.isSelected) {
        graphics.setColour(buttonColour.brighter(0.3f));
    }
    else {
        graphics.setColour(buttonColour);
    }

    graphics.fillEllipse(area);

    auto wandArea = area.reduced(area.getWidth() * 0.2f);

    graphics.setColour(juce::Colours::black);

    const float handleThickness = wandArea.getHeight() * 0.16f;
    const float circleDiameter  = wandArea.getHeight() * 0.4f;
    const float circleRadius    = circleDiameter * 0.5f;

    juce::Point<float> tipCentre   (wandArea.getRight() - circleRadius, wandArea.getY() + circleRadius);
    juce::Point<float> handleStart (wandArea.getX() + circleRadius,     wandArea.getBottom() - circleRadius);

    juce::Line<float> handleLine(handleStart, tipCentre);
    juce::Point<float> handleEnd = handleLine.getPointAlongLineProportionally(1.0f - circleRadius / handleLine.getLength());

    juce::Path handle;
    handle.addLineSegment(juce::Line<float>(handleStart, handleEnd), handleThickness);
    graphics.fillPath(handle);

    auto tipBounds = juce::Rectangle<float>(circleDiameter, circleDiameter).withCentre(tipCentre);
    graphics.fillEllipse(tipBounds);
}

void CustomLookAndFeel::drawArrowToolIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    auto area = bounds.reduced(outerButtonBoundsReduction);

    graphics.setColour(pressableButtonColour(state));
    graphics.fillRect(area);

    auto glyphArea = area.reduced(area.getWidth() * 0.22f, area.getHeight() * 0.34f);

    graphics.setColour(juce::Colours::black);
    fillArrowGlyph(graphics, glyphArea, 0.16f, 0.32f, 0.5f);
}

void CustomLookAndFeel::drawSpanToolIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    auto area = bounds.reduced(outerButtonBoundsReduction);

    if (state.isSelected) {
        graphics.setColour(buttonColour.brighter(0.3f));
    }
    else {
        graphics.setColour(buttonColour);
    }

    graphics.fillRect(area);

    auto glyphArea = area.reduced(area.getWidth() * 0.22f, area.getHeight() * 0.3f);

    graphics.setColour(juce::Colours::black);
    graphics.drawRect(glyphArea, 1.0f);

    const float endpointDiameter = glyphArea.getHeight() * 0.44f;
    const auto  endpointBounds   = juce::Rectangle<float>(endpointDiameter, endpointDiameter);

    graphics.fillEllipse(endpointBounds.withCentre({ glyphArea.getX(),     glyphArea.getCentreY() }));
    graphics.fillEllipse(endpointBounds.withCentre({ glyphArea.getRight(), glyphArea.getCentreY() }));
}

void CustomLookAndFeel::drawQuaverToolIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    const auto   area         = bounds.reduced(bounds.getWidth() * transportGlyphInsetRatio);
    const float  diameter     = juce::jmin(area.getWidth(), area.getHeight());
    const auto   circle       = area.withSizeKeepingCentre(diameter, diameter);
    const float  side         = diameter * 0.6f;
    const auto   square       = circle.withSizeKeepingCentre(side, side);
    const float  headX        = square.getX() + side * 0.34f;
    const float  headY        = square.getBottom() - side * 0.2f;
    const float  stemX        = headX + side * 0.19f;
    const float  stemTopY     = square.getY() + side * 0.04f;
    juce::Colour circleColour = pressableButtonColour(state);
    juce::Path   head;
    juce::Path   stem;
    juce::Path   flag;

    if (state.isSelected) {
        circleColour = circleColour.brighter(0.6f);
    }

    graphics.setColour(circleColour);
    graphics.fillEllipse(circle);

    graphics.setColour(juce::Colours::black);

    head.addEllipse(juce::Rectangle<float>(side * 0.44f, side * 0.3f).withCentre({ headX, headY }));
    head.applyTransform(juce::AffineTransform::rotation(-0.35f, headX, headY));
    graphics.fillPath(head);

    stem.addLineSegment({ stemX, headY - side * 0.04f, stemX, stemTopY }, juce::jmax(1.0f, side * 0.08f));
    graphics.fillPath(stem);

    flag.startNewSubPath(stemX, stemTopY);
    flag.cubicTo(stemX + side * 0.08f, stemTopY + side * 0.18f, stemX + side * 0.36f, stemTopY + side * 0.24f, stemX + side * 0.26f, stemTopY + side * 0.56f);
    flag.cubicTo(stemX + side * 0.28f, stemTopY + side * 0.34f, stemX + side * 0.12f, stemTopY + side * 0.3f, stemX, stemTopY + side * 0.26f);
    flag.closeSubPath();
    graphics.fillPath(flag);
}

static juce::Rectangle<float> fillArrowIconTile(juce::Graphics& graphics, juce::Rectangle<float> area,
                                                const ButtonState& state, juce::Colour buttonColour,
                                                float cornerRadius)
{
    juce::Colour tileColour = buttonColour;

    if (state.isSelected) {
        tileColour = buttonColour.brighter(0.3f);
    }

    if (state.isHovered) {
        tileColour = tileColour.brighter(0.15f);
    }

    graphics.setColour(tileColour);
    graphics.fillRoundedRectangle(area, cornerRadius);

    const auto glyphArea = area.reduced(area.getWidth() * 0.18f, area.getHeight() * 0.34f);

    const float nodeDiameter = glyphArea.getHeight();

    graphics.setColour(juce::Colours::black);
    graphics.fillEllipse(juce::Rectangle<float>(nodeDiameter, nodeDiameter)
                      .withCentre({ glyphArea.getX(), glyphArea.getCentreY() }));

    return glyphArea;
}

void CustomLookAndFeel::drawNodeArrowIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    const auto glyphArea = fillArrowIconTile(graphics, bounds.reduced(outerButtonBoundsReduction), state, buttonColour, paneCornerRadius);

    fillArrowGlyph(graphics, glyphArea, 0.16f, 0.32f, 0.5f);
}

void CustomLookAndFeel::drawPolyphonicArrowIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    const auto glyphArea = fillArrowIconTile(graphics, bounds.reduced(outerButtonBoundsReduction), state, buttonColour, paneCornerRadius);

    const float headLength = glyphArea.getWidth()  * 0.32f;
    const float headWidth  = glyphArea.getHeight() * 0.5f;
    const float thickness  = glyphArea.getHeight() * 0.16f;

    const juce::Point<float> frontTip(glyphArea.getRight(),            glyphArea.getCentreY());
    const juce::Point<float> rearTip (frontTip.x - headLength * 0.85f, glyphArea.getCentreY());

    juce::Path shaft;
    shaft.addLineSegment(juce::Line<float>({ glyphArea.getX(), glyphArea.getCentreY() }, rearTip), thickness);
    graphics.fillPath(shaft);

    juce::Path chevrons;

    for (juce::Point<float> tip : { rearTip, frontTip }) {
        chevrons.startNewSubPath(tip.x - headLength, tip.y - headWidth);
        chevrons.lineTo(tip);
        chevrons.lineTo(tip.x - headLength, tip.y + headWidth);
    }

    graphics.strokePath(chevrons, juce::PathStrokeType(thickness, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
}

void CustomLookAndFeel::drawTraversalArrowIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state)
{
    const auto glyphArea = fillArrowIconTile(graphics, bounds.reduced(outerButtonBoundsReduction), state, buttonColour, paneCornerRadius);

    const float headLength = glyphArea.getWidth()  * 0.32f;
    const float headWidth  = glyphArea.getHeight() * 0.5f;
    const float thickness  = glyphArea.getHeight() * 0.16f;

    const juce::Point<float> tip (glyphArea.getRight(), glyphArea.getCentreY());
    const juce::Point<float> base(tip.x - headLength,   glyphArea.getCentreY());

    juce::Path shaft;
    shaft.addLineSegment(juce::Line<float>({ glyphArea.getX(), glyphArea.getCentreY() }, base), thickness);
    graphics.fillPath(shaft);

    juce::Path head;
    head.startNewSubPath(base.x, base.y - headWidth);
    head.lineTo(tip);
    head.lineTo(base.x, base.y + headWidth);
    head.closeSubPath();

    graphics.strokePath(head, juce::PathStrokeType(juce::jmax(1.0f, thickness * 0.6f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void CustomLookAndFeel::drawFileLabel(juce::Graphics& graphics, const FileLabel& fileLabel)
{
    auto bounds = fileLabel.getLocalBounds().toFloat();

    juce::Colour background = baseDarkColour1;

    if (fileLabel.selected) {
        background = baseDarkColour1.brighter(0.18f);
    }

    if (fileLabel.grabbed) {
        background = baseDarkColour1.brighter(0.3f);
    }

    graphics.setColour(background);
    graphics.fillRect(bounds);

    if (fileLabel.selected) {
        graphics.setColour(baseLightColour2);
        graphics.fillRect(bounds.withWidth(fileLabelMarkerWidth));
    }

    graphics.setColour(juce::Colours::black.withAlpha(0.35f));
    graphics.drawHorizontalLine(static_cast<int>(bounds.getBottom()) - 1, bounds.getX(), bounds.getRight());
}
