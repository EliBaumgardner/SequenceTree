#include "CustomLookAndFeel.h"
#include "../Node/Arrow.h"

static juce::Path trimPathToFraction(const juce::Path& source, float fraction)
{
    if (fraction <= 0.0f || source.isEmpty()) {
        return {};
    }

    if (fraction >= 1.0f) {
        return source;
    }

    juce::PathFlatteningIterator measuring(source);
    juce::PathFlatteningIterator tracing(source);
    juce::Path                   trimmed;
    float                        totalLength = 0.0f;
    float                        accumulated = 0.0f;
    bool                         started     = false;

    while (measuring.next()) {
        const float segmentX = measuring.x2 - measuring.x1;
        const float segmentY = measuring.y2 - measuring.y1;

        totalLength += std::sqrt(segmentX * segmentX + segmentY * segmentY);
    }

    if (totalLength <= 0.0f) {
        return {};
    }

    const float targetLength = totalLength * fraction;

    while (tracing.next()) {
        const float segmentX      = tracing.x2 - tracing.x1;
        const float segmentY      = tracing.y2 - tracing.y1;
        const float segmentLength = std::sqrt(segmentX * segmentX + segmentY * segmentY);

        if (!started) {
            trimmed.startNewSubPath(tracing.x1, tracing.y1);

            started = true;
        }

        if (accumulated + segmentLength >= targetLength) {
            float alongSegment = 0.0f;

            if (segmentLength > 0.0f) {
                alongSegment = (targetLength - accumulated) / segmentLength;
            }

            trimmed.lineTo(tracing.x1 + segmentX * alongSegment, tracing.y1 + segmentY * alongSegment);

            return trimmed;
        }

        trimmed.lineTo(tracing.x2, tracing.y2);

        accumulated += segmentLength;
    }

    return trimmed;
}

static void strokeArrowShaft(juce::Graphics& graphics, const juce::Path& shaft, bool emphasised, float alpha, juce::Colour colour)
{
    juce::Path shadowPath    = shaft;
    juce::Path highlightPath = shaft;
    shadowPath.applyTransform(juce::AffineTransform::translation( 0.5f,  0.5f));
    highlightPath.applyTransform(juce::AffineTransform::translation(-0.5f, -0.5f));

    float strokeWidth = 1.25f;

    if (emphasised) {
        strokeWidth = 2.0f;
    }

    const juce::PathStrokeType stroke(strokeWidth);

    graphics.setColour(colour.darker(0.4f).withAlpha(0.35f * alpha));
    graphics.strokePath(shadowPath, stroke);
    graphics.setColour(colour.brighter(0.4f).withAlpha(0.18f * alpha));
    graphics.strokePath(highlightPath, stroke);
    graphics.setColour(colour.withAlpha(alpha));
    graphics.strokePath(shaft, stroke);
}

static void drawArrowProgress(juce::Graphics& graphics, const Arrow& arrow, const juce::Path& shaft, juce::Point<float> chord)
{
    static constexpr float baseOffset   = 2.0f;
    static constexpr float trailSpacing = 1.75f;

    int drawnCount = 0;

    for (const auto& entry : arrow.animation.trails) {
        const ArrowAnimation::Trail& trail = entry.second;

        if (trail.progress <= 0.0f) {
            continue;
        }

        const float offsetDistance = baseOffset + static_cast<float>(drawnCount) * trailSpacing;

        juce::Path offsetLine = shaft;
        offsetLine.applyTransform(juce::AffineTransform::translation(-chord.y * offsetDistance, chord.x * offsetDistance));

        juce::Path progressPath = trimPathToFraction(offsetLine, trail.progress);
        if (! progressPath.isEmpty()) {
            const juce::PathStrokeType progressStroke(0.75f, juce::PathStrokeType::curved, juce::PathStrokeType::butt);

            graphics.setColour(trail.colour);

            if (trail.source == TrailSource::Preview) {
                float dashLengths[] = { 4.0f, 3.0f };
                progressStroke.createDashedStroke(progressPath, progressPath, dashLengths, 2);
                graphics.fillPath(progressPath);
            }
            else {
                graphics.strokePath(progressPath, progressStroke);
            }
        }
        ++drawnCount;
    }
}

static juce::Path buildArrowHeadPath(juce::Point<float> tip, juce::Point<float> direction,
                              float headLength, float headWidth)
{
    const juce::Point<float> base = tip - direction * headLength;
    const juce::Point<float> side { -direction.y * headWidth, direction.x * headWidth };

    juce::Path head;
    head.startNewSubPath(base - side);
    head.lineTo(tip);
    head.lineTo(base + side);
    head.closeSubPath();

    return head;
}

static void strokeArrowHead(juce::Graphics& graphics, juce::Point<float> tip, juce::Point<float> direction,
                     float headLength, float headWidth, float alpha, juce::Colour colour,
                     float thickness)
{
    const juce::Path head = buildArrowHeadPath(tip, direction, headLength, headWidth);

    graphics.setColour(colour.withAlpha(alpha));
    graphics.strokePath(head, juce::PathStrokeType(thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

static void drawArrowHead(juce::Graphics& graphics, juce::Point<float> tip, juce::Point<float> direction,
                   float headLength, float headWidth, float alpha, juce::Colour colour)
{
    const juce::Path head = buildArrowHeadPath(tip, direction, headLength, headWidth);

    graphics.setColour(colour.withAlpha(alpha));
    graphics.fillPath(head);

    juce::Path headShadow    = head;
    juce::Path headHighlight = head;
    headShadow.applyTransform(juce::AffineTransform::translation( 0.5f,  0.5f));
    headHighlight.applyTransform(juce::AffineTransform::translation(-0.5f, -0.5f));

    const juce::PathStrokeType headStroke(0.5f);
    graphics.setColour(colour.darker(0.3f).withAlpha(0.2f));
    graphics.strokePath(headShadow, headStroke);
    graphics.setColour(colour.brighter(0.3f).withAlpha(0.1f));
    graphics.strokePath(headHighlight, headStroke);
}

static void drawArrowLabel(juce::Graphics& graphics, const Arrow& arrow, const ArrowGeometry& geometry,
                    float headLength, juce::Point<float> origin)
{
    const juce::String labelText = arrow.getDurationLabel();

    if (labelText.isEmpty() || arrow.editingDuration
        || arrow.animation.snapProgress <= Arrow::labelVisibleThreshold) {
        return;
    }

    const ArrowLabel label = arrow.getLabel(geometry, headLength);

    const juce::Point<float> mid = label.centre - origin;

    const juce::Graphics::ScopedSaveState savedState(graphics);

    graphics.addTransform(juce::AffineTransform::rotation(label.angle).translated(mid.x, mid.y));

    graphics.setFont(juce::Font(8.5f));
    graphics.setColour(juce::Colours::darkgrey);

    static constexpr float textW = 60.0f;
    static constexpr float textH = 12.0f;

    graphics.drawText(labelText, -textW * 0.5f, -textH, textW, textH, juce::Justification::centredBottom, true);
}

void CustomLookAndFeel::drawArrow(juce::Graphics& graphics, const Arrow& arrow)
{
    const ArrowGeometry geometry = arrow.getGeometry(arrow.animation.snapProgress);

    if (! geometry.valid) {
        return;
    }

    const bool  emphasised = arrow.hovered || arrow.selected;
    float headLength = Arrow::arrowHeadLength;
    float headWidth  = 4.25f;
    float alpha      = 1.0f;

    if (emphasised) {
        headLength = Arrow::arrowHeadLengthHover;
        headWidth  = 5.25f;
    }

    if (arrow.isGhost) {
        alpha = 0.5f;
    }

    const juce::Point<float> origin { static_cast<float>(arrow.getX()), static_cast<float>(arrow.getY()) };

    juce::Path shaft = arrow.buildShaftPath(geometry, headLength, origin);

    if (arrow.isDashed()) {
        juce::PathStrokeType dashStroke(1.25f);
        float dashLengths[] = { 6.0f, 10.0f };
        dashStroke.createDashedStroke(shaft, shaft, dashLengths, 2);
    }

    strokeArrowShaft(graphics, shaft, emphasised, alpha, arrowColour);

    if (! arrow.isGhost && ! arrow.animation.trails.empty()) {
        drawArrowProgress(graphics, arrow, shaft, geometry.chord);
    }

    if (geometry.drawHead) {
        if (arrow.isTraversalArrow()) {
            strokeArrowHead(graphics, geometry.tip - origin, geometry.direction, headLength, headWidth, alpha, arrowHeadColour, arrowHeadOutlineThickness);
        }
        else {
            drawArrowHead(graphics, geometry.tip - origin, geometry.direction, headLength, headWidth, alpha, arrowHeadColour);
        }
    }

    drawArrowLabel(graphics, arrow, geometry, headLength, origin);
}
