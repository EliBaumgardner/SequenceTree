//
// Created by Eli Baumgardner on 11/4/25.
//

#include "CustomLookAndFeel.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../Node/Arrow.h"
#include "../Node/Node.h"

juce::Rectangle<float> CustomLookAndFeel::getNodeCircleBounds(juce::Rectangle<float> componentBounds)
{
    static constexpr float shadowDX   = 2.0f;
    static constexpr float shadowBlur = 4.0f;
    float diameter = componentBounds.getWidth() - nodeCirclePad - shadowDX - shadowBlur;
    return juce::Rectangle<float>(diameter, diameter)
               .withPosition(componentBounds.getX() + nodeCirclePad, componentBounds.getY() + nodeCirclePad);
}

void CustomLookAndFeel::drawNode(juce::Graphics& g, const NodeVisual& visual)
{
    const juce::Rectangle<float> componentBounds = visual.bounds;

    static constexpr float shadowDX   = 2.0f;
    static constexpr float shadowDY   = 2.0f;
    static constexpr float shadowBlur = 4.0f;

    auto  circleBounds = getNodeCircleBounds(componentBounds);
    auto circleFill   = circleBounds.reduced(0.5f);
    auto circleSelect = circleBounds.reduced(2.0f);
    auto circleHover  = circleBounds.reduced(0.5f);

    {
        float innerR       = circleBounds.getWidth() * 0.5f;
        float outerR       = innerR + shadowBlur;
        auto  shadowCenter = circleBounds.getCentre() + juce::Point<float>(shadowDX, shadowDY);
        auto  shadowBounds = juce::Rectangle<float>(outerR * 2.0f, outerR * 2.0f).withCentre(shadowCenter);

        juce::ColourGradient gradient(
            juce::Colours::black.withAlpha(0.15f), shadowCenter.x, shadowCenter.y,
            juce::Colours::black.withAlpha(0.0f),  shadowCenter.x + outerR, shadowCenter.y,
            true);
        gradient.addColour(innerR / outerR, juce::Colours::black.withAlpha(0.10f));

        g.setGradientFill(gradient);
        g.fillEllipse(shadowBounds);
    }

    g.setColour(visual.colour);
    g.fillEllipse(circleFill);

    if (! visual.highlights.empty()) {
        constexpr float highlightRingWidth = 1.25f;
        constexpr float highlightRingSpacing = 2.0f;

        int ringIndex = 0;
        for (const auto& highlight : visual.highlights) {
            float inset = highlightRingWidth * 0.5f + static_cast<float>(ringIndex) * highlightRingSpacing;
            auto highlightRing = circleFill.reduced(inset);

            g.setColour(highlight.second);
            g.drawEllipse(highlightRing, highlightRingWidth);
            ++ringIndex;
        }
    }

    if (visual.isHovered) {
        g.drawEllipse(circleHover, 2.0f);
    }

    if (visual.isSelected) {
        juce::Path dottedPath;
        dottedPath.addEllipse(circleSelect);

        juce::PathStrokeType stroke(0.325f);
        float dashLengths[] = { 0.935f, 0.935f };
        stroke.createDashedStroke(dottedPath, dottedPath, dashLengths, 2);

        g.setColour(juce::Colours::black);
        g.strokePath(dottedPath, stroke);
    }
}

void CustomLookAndFeel::drawRootRectangle(juce::Graphics &g, juce::Rectangle<float> bounds)
{
    g.setColour(baseDarkColour2.darker());
    g.fillRect(bounds);
}


namespace {
    juce::Path trimPathToFraction(const juce::Path& source, float t)
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
        if (totalLength <= 0.0f) return {};

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
}

namespace {
    void strokeArrowShaft(juce::Graphics& g, const juce::Path& shaft, bool emphasised, float alpha, juce::Colour colour)
    {
        juce::Path shadowPath    = shaft;
        juce::Path highlightPath = shaft;
        shadowPath   .applyTransform(juce::AffineTransform::translation( 0.5f,  0.5f));
        highlightPath.applyTransform(juce::AffineTransform::translation(-0.5f, -0.5f));

        const juce::PathStrokeType stroke(emphasised ? 3.25f : 2.0f);

        g.setColour(colour.darker(0.4f).withAlpha(0.35f * alpha));
        g.strokePath(shadowPath, stroke);
        g.setColour(colour.brighter(0.4f).withAlpha(0.18f * alpha));
        g.strokePath(highlightPath, stroke);
        g.setColour(colour.withAlpha(alpha));
        g.strokePath(shaft, stroke);
    }

    void drawArrowProgress(juce::Graphics& g, const Arrow& arrow, const juce::Path& shaft, juce::Point<float> chord)
    {
        static constexpr float baseOffset   = 3.5f;
        static constexpr float trackSpacing = 3.0f;

        int drawnCount = 0;

        for (const auto& entry : arrow.progress.tracks)
        {
            const ArrowProgress::Track& track = entry.second;
            if (track.t <= 0.0f) {
                continue;
            }

            const float offsetDistance = baseOffset + (float)drawnCount * trackSpacing;

            juce::Path offsetLine = shaft;
            offsetLine.applyTransform(juce::AffineTransform::translation(-chord.y * offsetDistance,
                                                                         chord.x * offsetDistance));

            const juce::Path progressPath = trimPathToFraction(offsetLine, track.t);
            if (! progressPath.isEmpty()) {
                g.setColour(track.colour);
                g.strokePath(progressPath, juce::PathStrokeType(1.25f,
                                                               juce::PathStrokeType::curved,
                                                               juce::PathStrokeType::butt));
            }
            ++drawnCount;
        }
    }

    void drawArrowHead(juce::Graphics& g, juce::Point<float> tip, juce::Point<float> direction,
                       float headLength, float headWidth, float alpha, juce::Colour colour)
    {
        const juce::Point<float> base = tip - direction * headLength;
        const juce::Point<float> side { -direction.y * headWidth, direction.x * headWidth };

        juce::Path head;
        head.startNewSubPath(base - side);
        head.lineTo(tip);
        head.lineTo(base + side);
        head.closeSubPath();

        g.setColour(colour.withAlpha(alpha));
        g.fillPath(head);

        juce::Path headShadow    = head;
        juce::Path headHighlight = head;
        headShadow   .applyTransform(juce::AffineTransform::translation( 0.5f,  0.5f));
        headHighlight.applyTransform(juce::AffineTransform::translation(-0.5f, -0.5f));

        const juce::PathStrokeType headStroke(0.75f);
        g.setColour(colour.darker(0.3f).withAlpha(0.2f));
        g.strokePath(headShadow, headStroke);
        g.setColour(colour.brighter(0.3f).withAlpha(0.1f));
        g.strokePath(headHighlight, headStroke);
    }

    void drawArrowLabel(juce::Graphics& g, const Arrow& arrow, const ArrowGeometry& geometry,
                        float headLength, juce::Point<float> origin)
    {
        const juce::String labelText = arrow.getDurationLabel();

        if (labelText.isEmpty() || arrow.animT <= Arrow::labelVisibleThreshold) {
            return;
        }

        const juce::Point<float> centre = geometry.centre - origin;
        const juce::Point<float> tip    = geometry.tip    - origin;
        const juce::Point<float> delta  = tip - centre;

        juce::Point<float> shaftStart = centre;
        juce::Point<float> shaftEnd   = tip;

        const float length = delta.getDistanceFromOrigin();
        if (length > 0.0f) {
            const juce::Point<float> unit = delta / length;
            shaftStart += unit * (arrow.startNode->getHeight() * 0.5f);
            shaftEnd   -= unit * headLength;
        }

        const juce::Point<float> mid = (shaftStart + shaftEnd) * 0.5f;

        float angle = std::atan2(delta.y, delta.x);

        const float halfPi = juce::MathConstants<float>::halfPi;

        while (angle > halfPi) {
            angle -= juce::MathConstants<float>::pi;
        }

        while (angle <= -halfPi) {
            angle += juce::MathConstants<float>::pi;
        }

        static constexpr float verticalArrowTextThreshold = 0.2f;
        if (std::abs(delta.x) < std::abs(delta.y) * verticalArrowTextThreshold) {
            angle = 0.0f;
        }

        const juce::Graphics::ScopedSaveState savedState(g);

        g.addTransform(juce::AffineTransform::rotation(angle).translated(mid.x, mid.y));

        g.setFont(juce::Font(8.5f));
        g.setColour(juce::Colours::darkgrey);

        static constexpr float textW = 60.0f;
        static constexpr float textH = 12.0f;

        g.drawText(labelText, -textW * 0.5f, -textH, textW, textH,
                   juce::Justification::centredBottom, true);
    }
}

void CustomLookAndFeel::drawArrow(juce::Graphics& g, const Arrow& arrow)
{
    ArrowGeometry geometry = arrow.getGeometry(arrow.animT);

    if (! geometry.valid) {
        return;
    }

    const bool  emphasised = arrow.hovered || arrow.selected;
    const float headLength = emphasised ? 15.0f : 12.0f;
    const float headWidth  = emphasised ? 7.5f  : 6.0f;
    const float alpha      = arrow.isGhost ? 0.5f : 1.0f;

    const juce::Point<float> origin { (float)arrow.getX(), (float)arrow.getY() };

    juce::Path shaft = arrow.buildShaftPath(geometry, headLength, origin);

    if (arrow.isDashed()) {
        juce::PathStrokeType dashStroke(2.0f);
        float dashLengths[] = { 6.0f, 10.0f };
        dashStroke.createDashedStroke(shaft, shaft, dashLengths, 2);
    }

    strokeArrowShaft(g, shaft, emphasised, alpha, arrowColour);

    if (! arrow.isGhost && arrow.progress.hasTracks()) {
        drawArrowProgress(g, arrow, shaft, geometry.chord);
    }

    if (geometry.drawHead) {
        drawArrowHead(g, geometry.tip - origin, geometry.direction, headLength, headWidth, alpha, arrowHeadColour);
    }

    drawArrowLabel(g, arrow, geometry, headLength, origin);
}
