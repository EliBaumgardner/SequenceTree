//
// Created by Eli Baumgardner on 11/4/25.
//

#include "CustomLookAndFeel.h"
#include "../Node/Node.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../Node/NodeArrow.h"
#include "../Node/RootNode.h"
#include "../Node/DanglingArrow.h"

void CustomLookAndFeel::drawNodeInteractionEffects(juce::Graphics &g, const Node &node, juce::Rectangle<float> bounds) {

}

void CustomLookAndFeel::drawNode(juce::Graphics& g, const Node& node)
{
    drawNode(g, node, node.getLocalBounds().toFloat());
}

juce::Rectangle<float> CustomLookAndFeel::getNodeCircleBounds(juce::Rectangle<float> componentBounds)
{
    static constexpr float shadowDX   = 2.0f;
    static constexpr float shadowBlur = 4.0f;
    float diameter = componentBounds.getWidth() - nodeCirclePad - shadowDX - shadowBlur;
    return juce::Rectangle<float>(diameter, diameter)
               .withPosition(componentBounds.getX() + nodeCirclePad, componentBounds.getY() + nodeCirclePad);
}

void CustomLookAndFeel::drawNode(juce::Graphics& g, const Node& node, juce::Rectangle<float> componentBounds)
{
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

    float velocity        = (float)(int)node.midiNoteData.getProperty(ValueTreeIdentifiers::MidiVelocity, 100);
    float brightnessFactor = juce::jmap(velocity, 0.0f, 127.0f, 0.4f, 1.6f);

    juce::Colour nodeColour = node.nodeColour;

    g.setColour(nodeColour);
    g.fillEllipse(circleFill);

    if (! node.activeHighlights.empty()) {
        constexpr float highlightRingWidth = 1.25f;
        constexpr float highlightRingSpacing = 2.0f;

        int ringIndex = 0;
        for (const auto& highlight : node.activeHighlights) {
            float inset = highlightRingWidth * 0.5f + static_cast<float>(ringIndex) * highlightRingSpacing;
            auto highlightRing = circleFill.reduced(inset);

            g.setColour(highlight.second);
            g.drawEllipse(highlightRing, highlightRingWidth);
            ++ringIndex;
        }
    }

    if (node.isHovered) {
        g.drawEllipse(circleHover, 2.0f);
    }

    if (node.isSelected) {
        juce::Path dottedPath;
        dottedPath.addEllipse(circleSelect);

        juce::PathStrokeType stroke(0.325f);
        float dashLengths[] = { 0.935f, 0.935f };
        stroke.createDashedStroke(dottedPath, dottedPath, dashLengths, 2);

        g.setColour(juce::Colours::black);
        g.strokePath(dottedPath, stroke);
    }
}

void CustomLookAndFeel::drawRootNode(juce::Graphics& g, const RootNode& node)
{
    auto circleBounds = node.getLocalBounds().toFloat()
                            .withTrimmedLeft((float)RootNode::loopLimitRectangleWidth);
    drawNode(g, node, circleBounds);
}

void CustomLookAndFeel::drawRootNodeRectangle(juce::Graphics &g, const RootRectangle &rootRectangle) {

    juce::Rectangle<float> rootBounds = rootRectangle.getLocalBounds().toFloat();

    g.setColour(baseDarkColour2.darker());
    g.fillRect(rootBounds);

}

void CustomLookAndFeel::drawNodeArrowText(juce::Graphics &g, const NodeArrow &nodeArrow, const juce::TextEditor &editor,TextCords textCord) {

    int parentX = textCord.parentNodeX;
    int parentY = textCord.parentNodeY;
    int childX  = textCord.childNodeX;
    int childY  = textCord.childNodeY;
    int arrowEndX = textCord.newX;
    int arrowEndY = textCord.newY;


    juce::String labelText = editor.getText();
    if (labelText.isNotEmpty() && nodeArrow.animT > 0.8f) {
        float deltaX = float(arrowEndX - parentX);
        float deltaY = float(arrowEndY - parentY);

        const float arrowHeadLength = 12.0f;
        const float parentRadius = float(nodeArrow.startNode->getHeight()) * 0.5f;

        float shaftStartX = float(parentX);
        float shaftStartY = float(parentY);
        float shaftEndX = float(arrowEndX);
        float shaftEndY = float(arrowEndY);
        float length = std::sqrt(deltaX * deltaX + deltaY * deltaY);
        if (length > 0.0f) {
            float ux = deltaX / length;
            float uy = deltaY / length;
            shaftStartX += ux * parentRadius;
            shaftStartY += uy * parentRadius;
            shaftEndX   -= ux * arrowHeadLength;
            shaftEndY   -= uy * arrowHeadLength;
        }

        float midX = (shaftStartX + shaftEndX) * 0.5f;
        float midY = (shaftStartY + shaftEndY) * 0.5f;

        float angle = std::atan2(deltaY, deltaX);

        const float halfPi = juce::MathConstants<float>::halfPi;

        while (angle >  halfPi) {
            angle -= juce::MathConstants<float>::pi;
        }

        while (angle <= -halfPi) {
            angle += juce::MathConstants<float>::pi;
        }

        const float verticalArrowTextThreshold = 0.2f;
        const bool isVertical = std::abs(deltaX) < std::abs(deltaY) * verticalArrowTextThreshold;
        if (isVertical) {
            angle = 0.0f;
        }

        juce::Graphics::ScopedSaveState savedState(g);

        g.addTransform(juce::AffineTransform::rotation(angle).translated(midX, midY));

        g.setFont(juce::Font(8.5f));
        g.setColour(juce::Colours::darkgrey);

        float textW = 60.0f;
        float textH = 12.0f;

        g.drawText(labelText, -textW * 0.5f, -textH, textW, textH,
                   juce::Justification::centredBottom, true);
    }
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

void CustomLookAndFeel::drawNodeArrow(juce::Graphics &g, const NodeArrow& nodeArrow, const juce::TextEditor& editor)
{
    auto* a = nodeArrow.startNode;
    auto* b = nodeArrow.endNode;

    juce::Point<int> parentCentre = a->getNodeCentre();
    juce::Point<int> childCentre  = b->getNodeCentre();

    float arrowLength  = nodeArrow.hovered ? 15.0f : 12.0f;
    float arrowWidth   = nodeArrow.hovered ? 7.5f  : 6.0f;
    int   childRadius  = b->getHeight() / 2;
    int   parentRadius = a->getHeight() / 2;

    g.setColour(arrowColour);
    float ghostAlpha;
    if (nodeArrow.isGhost) {
        ghostAlpha = 0.5f;
    }
    else {
        ghostAlpha = 1.0f;
    }

    float parentCenterX = float(parentCentre.x - nodeArrow.getX());
    float parentCenterY = float(parentCentre.y - nodeArrow.getY());
    float childCenterX  = float(childCentre.x  - nodeArrow.getX());
    float childCenterY  = float(childCentre.y  - nodeArrow.getY());

    float arrowEndX = parentCenterX + (childCenterX - parentCenterX) * nodeArrow.animT;
    float arrowEndY = parentCenterY + (childCenterY - parentCenterY) * nodeArrow.animT;

    float deltaX = arrowEndX - parentCenterX;
    float deltaY = arrowEndY - parentCenterY;
    float length = std::sqrt(deltaX * deltaX + deltaY * deltaY);

    if (length < 1.0f) {
        return;
    }

    float dirX = deltaX / length;
    float dirY = deltaY / length;

    bool childIsTraversalFlag = b->nodeType == NodeType::TraversalFlag;

    if (! childIsTraversalFlag) {
        arrowEndX -= dirX * float(childRadius);
        arrowEndY -= dirY * float(childRadius);
    }
    else {
        float childHalf      = std::min(b->getWidth(), b->getHeight()) * 0.5f;
        float baseHalfHeight = (childHalf - 4.0f) * 0.7f * 0.5f;
        arrowEndX += dirX * baseHalfHeight;
        arrowEndY += dirY * baseHalfHeight;
    }

    bool isRootTargetArrow = b->nodeType == NodeType::Root && !a->isAlternativeNode;
    bool isFlagSourceArrow = a->nodeType == NodeType::TraversalFlag;

    juce::Path linePath;

    {
        float dx = arrowEndX - parentCenterX;
        float dy = arrowEndY - parentCenterY;
        float absDx = std::abs(dx);
        float absDy = std::abs(dy);

        if (absDx < 1.0f || absDy < 1.0f || childIsTraversalFlag) {
            linePath.startNewSubPath(parentCenterX, parentCenterY);
            linePath.lineTo(arrowEndX, arrowEndY);
        }
        else {
            float sign;
            if (dx >= 0.0f) {
                sign = 1.0f;
            }
            else {
                sign = -1.0f;
            }
            float perpX = -dirY * 0.8f * sign;
            float perpY =  dirX * 0.8f * sign;

            float segLen = std::sqrt(dx * dx + dy * dy);
            float offset = segLen * 0.15f;

            float cp1X = parentCenterX + dx * 0.33f + perpX * offset;
            float cp1Y = parentCenterY + dy * 0.33f + perpY * offset;
            float cp2X = parentCenterX + dx * 0.67f - perpX * offset;
            float cp2Y = parentCenterY + dy * 0.67f - perpY * offset;

            linePath.startNewSubPath(parentCenterX, parentCenterY);
            linePath.cubicTo(cp1X, cp1Y, cp2X, cp2Y, arrowEndX, arrowEndY);

            float neckX = arrowEndX - cp2X;
            float neckY = arrowEndY - cp2Y;
            float neckLen = std::sqrt(neckX * neckX + neckY * neckY);
            if (neckLen > 0.0f) {
                dirX = neckX / neckLen;
                dirY = neckY / neckLen;
            }
        }
    }

    if (isRootTargetArrow || isFlagSourceArrow || nodeArrow.isGhost) {
        juce::PathStrokeType stroke(2.0f);
        float dashLengths[] = { 6.0f, 10.0f };
        stroke.createDashedStroke(linePath, linePath, dashLengths, 2);
    }

    juce::PathStrokeType lineStroke(nodeArrow.hovered ? 3.25f : 2.0f);
    auto shadowPath    = linePath;
    auto highlightPath = linePath;
    shadowPath   .applyTransform(juce::AffineTransform::translation( 0.5f,  0.5f));
    highlightPath.applyTransform(juce::AffineTransform::translation(-0.5f, -0.5f));

    g.setColour(arrowColour.darker(0.4f).withAlpha(0.35f * ghostAlpha));
    g.strokePath(shadowPath,    lineStroke);
    g.setColour(arrowColour.brighter(0.4f).withAlpha(0.18f * ghostAlpha));
    g.strokePath(highlightPath, lineStroke);
    g.setColour(arrowColour.withAlpha(ghostAlpha));
    g.strokePath(linePath, lineStroke);

    if (! nodeArrow.isGhost && nodeArrow.progress.hasTracks()) {
        const float shaftDeltaX = arrowEndX - parentCenterX;
        const float shaftDeltaY = arrowEndY - parentCenterY;
        const float shaftLength = std::sqrt(shaftDeltaX * shaftDeltaX + shaftDeltaY * shaftDeltaY);

        if (shaftLength > 0.0f) {
            const float unitX = shaftDeltaX / shaftLength;
            const float unitY = shaftDeltaY / shaftLength;

            const float baseOffset = 3.5f;
            const float trackSpacing = 3.0f;

            int drawnCount = 0;
            for (const auto& entry : nodeArrow.progress.tracks) {
                const ArrowProgress::Track& track = entry.second;
                if (track.t <= 0.0f) {
                    continue;
                }

                const float offsetDistance = baseOffset + static_cast<float>(drawnCount) * trackSpacing;
                const float perpX = -unitY * offsetDistance;
                const float perpY =  unitX * offsetDistance;

                juce::Path offsetLine = linePath;
                offsetLine.applyTransform(juce::AffineTransform::translation(perpX, perpY));

                juce::Path progressPath = trimPathToFraction(offsetLine, track.t);
                if (! progressPath.isEmpty()) {
                    g.setColour(track.colour);
                    g.strokePath(progressPath, juce::PathStrokeType(1.25f,
                                                                    juce::PathStrokeType::curved,
                                                                    juce::PathStrokeType::butt));
                }
                ++drawnCount;
            }
        }
    }

    if (nodeArrow.animT > 0.3f && !childIsTraversalFlag) {
        float leftX  = arrowEndX - arrowLength * dirX + arrowWidth * dirY;
        float leftY  = arrowEndY - arrowLength * dirY - arrowWidth * dirX;
        float rightX = arrowEndX - arrowLength * dirX - arrowWidth * dirY;
        float rightY = arrowEndY - arrowLength * dirY + arrowWidth * dirX;

        juce::Path arrowHead;
        arrowHead.startNewSubPath(leftX, leftY);
        arrowHead.lineTo(arrowEndX, arrowEndY);
        arrowHead.lineTo(rightX, rightY);
        arrowHead.closeSubPath();

        g.setColour(arrowHeadColour.withAlpha(ghostAlpha));
        g.fillPath(arrowHead);

        auto headShadow    = arrowHead;
        auto headHighlight = arrowHead;
        headShadow   .applyTransform(juce::AffineTransform::translation( 0.5f,  0.5f));
        headHighlight.applyTransform(juce::AffineTransform::translation(-0.5f, -0.5f));

        juce::PathStrokeType headStroke(0.75f);
        g.setColour(arrowHeadColour.darker(0.3f).withAlpha(0.2f));
        g.strokePath(headShadow,    headStroke);
        g.setColour(arrowHeadColour.brighter(0.3f).withAlpha(0.1f));
        g.strokePath(headHighlight, headStroke);
    }

    TextCords textCords;
    textCords.parentNodeX = parentCenterX;
    textCords.parentNodeY = parentCenterY;
    textCords.childNodeX = childCenterX;
    textCords.childNodeY = childCenterY;
    textCords.newX = arrowEndX;
    textCords.newY = arrowEndY;

    drawNodeArrowText(g, nodeArrow, editor, textCords);
}

void CustomLookAndFeel::drawDanglingArrow(juce::Graphics &g, const DanglingArrow &danglingArrow)
{
    Node* node = danglingArrow.startNode;
    if (node == nullptr) {
        return;
    }

    juce::Point<int> startCentre = node->getNodeCentre();
    juce::Point<int> tip         = danglingArrow.getTip();

    float startX = float(startCentre.x - danglingArrow.getX());
    float startY = float(startCentre.y - danglingArrow.getY());
    float endX   = float(tip.x - danglingArrow.getX());
    float endY   = float(tip.y - danglingArrow.getY());

    float dx = endX - startX;
    float dy = endY - startY;
    float length = std::sqrt(dx * dx + dy * dy);

    if (length < 1.0f) {
        return;
    }

    float dirX = dx / length;
    float dirY = dy / length;

    float parentRadius = node->getHeight() / 2.0f;
    startX += dirX * parentRadius;
    startY += dirY * parentRadius;

    juce::Path linePath;
    linePath.startNewSubPath(startX, startY);
    linePath.lineTo(endX, endY);

    if (danglingArrow.dashed) {
        juce::PathStrokeType dashStroke(2.0f);
        float dashLengths[] = { 6.0f, 10.0f };
        dashStroke.createDashedStroke(linePath, linePath, dashLengths, 2);
    }

    juce::PathStrokeType lineStroke(danglingArrow.hovered ? 3.25f : 2.0f);
    auto shadowPath    = linePath;
    auto highlightPath = linePath;
    shadowPath   .applyTransform(juce::AffineTransform::translation( 0.5f,  0.5f));
    highlightPath.applyTransform(juce::AffineTransform::translation(-0.5f, -0.5f));

    g.setColour(arrowColour.darker(0.4f).withAlpha(0.35f));
    g.strokePath(shadowPath,    lineStroke);
    g.setColour(arrowColour.brighter(0.4f).withAlpha(0.18f));
    g.strokePath(highlightPath, lineStroke);
    g.setColour(arrowColour);
    g.strokePath(linePath, lineStroke);

    if (danglingArrow.progress.hasTracks()) {
        const float baseOffset = 3.5f;
        const float trackSpacing = 3.0f;

        int drawnCount = 0;
        for (const auto& entry : danglingArrow.progress.tracks) {
            const ArrowProgress::Track& track = entry.second;
            if (track.t <= 0.0f) {
                continue;
            }

            const float offsetDistance = baseOffset + static_cast<float>(drawnCount) * trackSpacing;
            const float perpX = -dirY * offsetDistance;
            const float perpY =  dirX * offsetDistance;

            juce::Path offsetLine = linePath;
            offsetLine.applyTransform(juce::AffineTransform::translation(perpX, perpY));

            juce::Path progressPath = trimPathToFraction(offsetLine, track.t);
            if (! progressPath.isEmpty()) {
                g.setColour(track.colour);
                g.strokePath(progressPath, juce::PathStrokeType(1.25f,
                                                                juce::PathStrokeType::curved,
                                                                juce::PathStrokeType::butt));
            }
            ++drawnCount;
        }
    }

    const float arrowLength = danglingArrow.hovered ? 15.0f : 12.0f;
    const float arrowWidth  = danglingArrow.hovered ? 7.5f  : 6.0f;

    float leftX  = endX - arrowLength * dirX + arrowWidth * dirY;
    float leftY  = endY - arrowLength * dirY - arrowWidth * dirX;
    float rightX = endX - arrowLength * dirX - arrowWidth * dirY;
    float rightY = endY - arrowLength * dirY + arrowWidth * dirX;

    juce::Path arrowHead;
    arrowHead.startNewSubPath(leftX, leftY);
    arrowHead.lineTo(endX, endY);
    arrowHead.lineTo(rightX, rightY);
    arrowHead.closeSubPath();

    g.setColour(arrowHeadColour);
    g.fillPath(arrowHead);

    juce::String labelText = danglingArrow.getDurationLabel();
    if (labelText.isNotEmpty()) {
        float rawStartX = float(startCentre.x - danglingArrow.getX());
        float rawStartY = float(startCentre.y - danglingArrow.getY());

        float deltaX = endX - rawStartX;
        float deltaY = endY - rawStartY;

        const float startRadius = float(node->getHeight()) * 0.5f;

        float shaftStartX = rawStartX;
        float shaftStartY = rawStartY;
        float shaftEndX = endX;
        float shaftEndY = endY;
        float shaftLength = std::sqrt(deltaX * deltaX + deltaY * deltaY);
        if (shaftLength > 0.0f) {
            float ux = deltaX / shaftLength;
            float uy = deltaY / shaftLength;
            shaftStartX += ux * startRadius;
            shaftStartY += uy * startRadius;
            shaftEndX   -= ux * arrowLength;
            shaftEndY   -= uy * arrowLength;
        }

        float midX = (shaftStartX + shaftEndX) * 0.5f;
        float midY = (shaftStartY + shaftEndY) * 0.5f;

        float angle = std::atan2(deltaY, deltaX);

        const float halfPi = juce::MathConstants<float>::halfPi;

        while (angle >  halfPi) {
            angle -= juce::MathConstants<float>::pi;
        }

        while (angle <= -halfPi) {
            angle += juce::MathConstants<float>::pi;
        }

        const float verticalArrowTextThreshold = 0.2f;
        const bool isVertical = std::abs(deltaX) < std::abs(deltaY) * verticalArrowTextThreshold;
        if (isVertical) {
            angle = 0.0f;
        }

        juce::Graphics::ScopedSaveState savedState(g);

        g.addTransform(juce::AffineTransform::rotation(angle).translated(midX, midY));

        g.setFont(juce::Font(8.5f));
        g.setColour(juce::Colours::darkgrey);

        float textW = 60.0f;
        float textH = 12.0f;

        g.drawText(labelText, -textW * 0.5f, -textH, textW, textH,
                   juce::Justification::centredBottom, true);
    }
}
