#include "CustomLookAndFeel.h"
#include "../Node/Arrow.h"

static juce::Path trimPathToFraction(const juce::Path& source, float t)
{
    if (t <= 0.0f || source.isEmpty()) {
        return {};
    }

    if (t >= 1.0f) {
        return source;
    }

    float totalLength = 0.0f;
    {
        juce::PathFlatteningIterator it(source);
        while (it.next())
        {
            const float dx = it.x2 - it.x1;
            const float dy = it.y2 - it.y1;
            totalLength += std::sqrt(dx * dx + dy * dy);
        }
    }
    if (totalLength <= 0.0f) {
        return {};
    }

    const float target = totalLength * t;
    juce::Path  out;
    bool        started     = false;
    float       accumulated = 0.0f;

    juce::PathFlatteningIterator it(source);
    while (it.next())
    {
        const float dx     = it.x2 - it.x1;
        const float dy     = it.y2 - it.y1;
        const float segLen = std::sqrt(dx * dx + dy * dy);

        if (! started) { out.startNewSubPath(it.x1, it.y1); started = true; }

        if (accumulated + segLen >= target) {
            const float remain   = target - accumulated;
            float fraction;
        if (segLen > 0.0f) {
            fraction = remain / segLen;
        }
        else {
            fraction = 0.0f;
        }
            out.lineTo(it.x1 + dx * fraction, it.y1 + dy * fraction);
            return out;
        }

        out.lineTo(it.x2, it.y2);
        accumulated += segLen;
    }
    return out;
}

static void strokeArrowShaft(juce::Graphics& g, const juce::Path& shaft, bool emphasised, float alpha, juce::Colour colour)
{
    juce::Path shadowPath    = shaft;
    juce::Path highlightPath = shaft;
    shadowPath   .applyTransform(juce::AffineTransform::translation( 0.5f,  0.5f));
    highlightPath.applyTransform(juce::AffineTransform::translation(-0.5f, -0.5f));

    float strokeWidth = 1.25f;

    if (emphasised) {
        strokeWidth = 2.0f;
    }

    const juce::PathStrokeType stroke(strokeWidth);

    g.setColour(colour.darker(0.4f).withAlpha(0.35f * alpha));
    g.strokePath(shadowPath, stroke);
    g.setColour(colour.brighter(0.4f).withAlpha(0.18f * alpha));
    g.strokePath(highlightPath, stroke);
    g.setColour(colour.withAlpha(alpha));
    g.strokePath(shaft, stroke);
}

static void drawArrowProgress(juce::Graphics& g, const Arrow& arrow, const juce::Path& shaft, juce::Point<float> chord)
{
    static constexpr float baseOffset   = 2.0f;
    static constexpr float trailSpacing = 1.75f;

    int drawnCount = 0;

    for (const auto& entry : arrow.animation.trails)
    {
        const ArrowAnimation::Trail& trail = entry.second;
        if (trail.t <= 0.0f) {
            continue;
        }

        const float offsetDistance = baseOffset + static_cast<float>(drawnCount) * trailSpacing;

        juce::Path offsetLine = shaft;
        offsetLine.applyTransform(juce::AffineTransform::translation(-chord.y * offsetDistance,
                                                                     chord.x * offsetDistance));

        const juce::Path progressPath = trimPathToFraction(offsetLine, trail.t);
        if (! progressPath.isEmpty()) {
            g.setColour(trail.colour);
            g.strokePath(progressPath, juce::PathStrokeType(0.75f,
                                                           juce::PathStrokeType::curved,
                                                           juce::PathStrokeType::butt));
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

static void strokeArrowHead(juce::Graphics& g, juce::Point<float> tip, juce::Point<float> direction,
                     float headLength, float headWidth, float alpha, juce::Colour colour,
                     float thickness)
{
    const juce::Path head = buildArrowHeadPath(tip, direction, headLength, headWidth);

    g.setColour(colour.withAlpha(alpha));
    g.strokePath(head, juce::PathStrokeType(thickness,
                                            juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));
}

static void drawArrowHead(juce::Graphics& g, juce::Point<float> tip, juce::Point<float> direction,
                   float headLength, float headWidth, float alpha, juce::Colour colour)
{
    const juce::Path head = buildArrowHeadPath(tip, direction, headLength, headWidth);

    g.setColour(colour.withAlpha(alpha));
    g.fillPath(head);

    juce::Path headShadow    = head;
    juce::Path headHighlight = head;
    headShadow   .applyTransform(juce::AffineTransform::translation( 0.5f,  0.5f));
    headHighlight.applyTransform(juce::AffineTransform::translation(-0.5f, -0.5f));

    const juce::PathStrokeType headStroke(0.5f);
    g.setColour(colour.darker(0.3f).withAlpha(0.2f));
    g.strokePath(headShadow, headStroke);
    g.setColour(colour.brighter(0.3f).withAlpha(0.1f));
    g.strokePath(headHighlight, headStroke);
}

static void drawArrowLabel(juce::Graphics& g, const Arrow& arrow, const ArrowGeometry& geometry,
                    float headLength, juce::Point<float> origin)
{
    const juce::String labelText = arrow.getDurationLabel();

    if (labelText.isEmpty() || arrow.editingDuration
        || arrow.animation.snapT <= Arrow::labelVisibleThreshold) {
        return;
    }

    const ArrowLabel label = arrow.getLabel(geometry, headLength);

    const juce::Point<float> mid = label.centre - origin;

    const juce::Graphics::ScopedSaveState savedState(g);

    g.addTransform(juce::AffineTransform::rotation(label.angle).translated(mid.x, mid.y));

    g.setFont(juce::Font(8.5f));
    g.setColour(juce::Colours::darkgrey);

    static constexpr float textW = 60.0f;
    static constexpr float textH = 12.0f;

    g.drawText(labelText, -textW * 0.5f, -textH, textW, textH,
               juce::Justification::centredBottom, true);
}

void CustomLookAndFeel::drawArrow(juce::Graphics& g, const Arrow& arrow)
{
    const ArrowGeometry geometry = arrow.getGeometry(arrow.animation.snapT);

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

    strokeArrowShaft(g, shaft, emphasised, alpha, arrowColour);

    if (! arrow.isGhost && ! arrow.animation.trails.empty()) {
        drawArrowProgress(g, arrow, shaft, geometry.chord);
    }

    if (geometry.drawHead) {
        if (arrow.isTraversalArrow()) {
            strokeArrowHead(g, geometry.tip - origin, geometry.direction, headLength, headWidth, alpha,
                            arrowHeadColour, arrowHeadOutlineThickness);
        }
        else {
            drawArrowHead(g, geometry.tip - origin, geometry.direction, headLength, headWidth, alpha, arrowHeadColour);
        }
    }

    drawArrowLabel(g, arrow, geometry, headLength, origin);
}
