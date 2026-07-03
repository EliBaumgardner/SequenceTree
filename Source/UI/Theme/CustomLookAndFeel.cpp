//
// Created by Eli Baumgardner on 11/4/25.
//

#include "CustomLookAndFeel.h"
#include "../Buttons/ModulatorButton.h"
#include "../Buttons/TraversalFlagButton.h"
#include "../Buttons/ResetButton.h"
#include "../Canvas/NodeCanvas.h"
#include "../Node/Node.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../Titlebar.h"
#include "../BottomBar.h"
#include "../TraversalMenu.h"
#include "../TraversalDisplayMenu.h"
#include "CustomTextEditor.h"
#include "../Node/NodeArrow.h"
#include "../Node/NodeTextEditor.h"
#include "CustomTextCaret.h"
#include "../Node/RootNode.h"
#include "Buttons/ButtonConstants.h"
#include "../Buttons/PaintTool.h"
#include "../Buttons/PaintToolSettings.h"
#include "../Graph/ValueTreeState.h"


CustomLookAndFeel::CustomLookAndFeel()
{
    updateColours();
}

void CustomLookAndFeel::setColorIntensityFactor(float factor)
{
    colorIntensityFactor = juce::jlimit(0.0f, 1.0f, factor);
    updateColours();
}

juce::Colour CustomLookAndFeel::applyIntensity(juce::Colour base) const
{
    float distance        = std::abs(colorIntensityFactor - 0.5f) * 2.0f;
    float hueShift        = (colorIntensityFactor - 0.5f) * 0.5f;
    float saturationBoost = distance * 0.35f;
    float newSaturation   = juce::jlimit(0.0f, 1.0f, base.getSaturation() + saturationBoost);

    float darkness        = 1.0f - base.getBrightness();
    float brightnessLift  = distance * (0.10f + darkness * 0.25f);
    float newBrightness   = juce::jlimit(0.0f, 1.0f, base.getBrightness() + brightnessLift);

    return base.withRotatedHue(hueShift)
               .withSaturation(newSaturation)
               .withBrightness(newBrightness);
}

void CustomLookAndFeel::updateColours()
{
    // darkColour1 = applyIntensity(baseDarkColour1);
    // darkColour2 = applyIntensity(baseDarkColour2);
    // lightColour1 = applyIntensity(baseLightColour1);
    // lightColour2 = applyIntensity(baseLightColour2);
    // lightColour3 = applyIntensity(baseLightColour3);
    //
    // //editorColour = lightColour2;
    // //arrowColour = darkColour2.darker();
    // arrowHeadColour = arrowColour;
}

void CustomLookAndFeel::drawEditor(juce::Graphics &g, CustomTextEditor& editor)
{
    auto bounds = editor.getLocalBounds().toFloat();
    g.setColour(buttonBarColour);
    g.fillRoundedRectangle(bounds, paneCornerRadius);

    editor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    editor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    editor.setColour(juce::TextEditor::textColourId, textColour);
    editor.TextEditor::paint(g);
}

void CustomLookAndFeel::drawCanvas(juce::Graphics &g, const NodeCanvas &canvas)
{

    g.fillAll(canvasColour.brighter());

    if (!canvas.showGrid) {
        return;
    }

    float spacing = canvas.gridSpacing;
    if (spacing < 15.0f) {
        return;
    }

    auto bounds = canvas.getLocalBounds().toFloat();
    float ox;
    float oy;
    if (canvas.gridOriginSet) {
        ox = canvas.gridOrigin.x;
        oy = canvas.gridOrigin.y;
    }
    else {
        ox = bounds.getCentreX();
        oy = bounds.getCentreY();
    }


    const float armLen = 6.0f;
    g.setColour(gridColour);

    float startX = ox - std::ceil((ox - bounds.getX()) / spacing) * spacing;
    float startY = oy - std::ceil((oy - bounds.getY()) / spacing) * spacing;

    for (float x = startX; x <= bounds.getRight(); x += spacing)
    {
        for (float y = startY; y <= bounds.getBottom(); y += spacing)
        {
            g.drawLine(x - armLen, y, x + armLen, y, 0.5f);
            g.drawLine(x, y - armLen, x, y + armLen, 0.5f);
        }
    }
}

void CustomLookAndFeel::drawTitleBar(juce::Graphics &g, const Titlebar &titleBar)
{
    auto bounds = titleBar.getLocalBounds().toFloat();

    juce::ColourGradient gradient(barColour.brighter(0.06f), 0, bounds.getY(),
                                   barColour.darker(0.04f),  0, bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRect(bounds);

    g.setColour(barColour.brighter(0.12f));
    g.drawHorizontalLine((int)bounds.getY(), bounds.getX(), bounds.getRight());

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawHorizontalLine((int)bounds.getBottom() - 1, bounds.getX(), bounds.getRight());
}

void CustomLookAndFeel::drawBottomBar(juce::Graphics &g, const BottomBar &bottomBar)
{
    auto bounds = bottomBar.getLocalBounds().toFloat();

    juce::ColourGradient gradient(barColour.darker(0.04f),  0, bounds.getY(),
                                   barColour.brighter(0.06f), 0, bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRect(bounds);

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawHorizontalLine((int)bounds.getY(), bounds.getX(), bounds.getRight());

    g.setColour(barColour.brighter(0.12f));
    g.drawHorizontalLine((int)bounds.getBottom() - 1, bounds.getX(), bounds.getRight());
}

void CustomLookAndFeel::drawTraversalMenu(juce::Graphics &g, const TraversalMenu &traversalMenu)
{
    auto bounds = traversalMenu.getLocalBounds().toFloat();
    g.setColour(baseDarkColour2);
    g.fillRect(bounds);

    auto barHeight = std::floor(bounds.getHeight() * 0.05f);
    auto barBounds = bounds.withHeight(barHeight)
                           .withTrimmedLeft((float) TraversalMenu::resizerWidth);

    juce::ColourGradient gradient(barColour.brighter(0.06f), 0, barBounds.getY(),
                                   barColour.darker(0.04f),  0, barBounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRect(barBounds);

    g.setColour(barColour.brighter(0.12f));
    g.drawHorizontalLine((int)barBounds.getY(), barBounds.getX(), barBounds.getRight());

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawHorizontalLine((int)barBounds.getBottom() - 1, barBounds.getX(), barBounds.getRight());
}

void CustomLookAndFeel::drawTraversalMenuResizer(juce::Graphics &g, juce::Rectangle<int> bounds, bool isMouseOver, bool isDragging)
{
    juce::Colour fill = baseDarkColour1;

    if (isDragging) {
        fill = baseLightColour2;
    }
    else if (isMouseOver) {
        fill = baseDarkColour1.brighter(0.25f);
    }

    g.setColour(fill);
    g.fillRect(bounds);

    auto gripBounds = bounds.toFloat().reduced(bounds.getWidth() * 0.3f, bounds.getHeight() * 0.35f);
    g.setColour(baseLightColour1.withAlpha(0.5f));

    const float dotSpacing = 4.0f;
    for (float y = gripBounds.getY(); y < gripBounds.getBottom(); y += dotSpacing) {
        g.fillRect(gripBounds.getX(), y, gripBounds.getWidth(), 1.0f);
    }
}

void CustomLookAndFeel::drawButtonPane(juce::Graphics &g, const ButtonPane& selectionBar)
{
    auto bounds = selectionBar.getLocalBounds().reduced(outerButtonBoundsReduction).toFloat();
    g.setColour(buttonBarColour);
    g.fillRoundedRectangle(bounds, paneCornerRadius);
}

void CustomLookAndFeel::drawTempoDisplay(juce::Graphics &g, const TempoDisplay &display) {
}


void CustomLookAndFeel::drawDisplayMenu(juce::Graphics &g, const DisplayMenu& displaySelector)
{
    auto bounds = displaySelector.getLocalBounds().toFloat().reduced(outerButtonBoundsReduction);
    g.setColour(buttonBarColour);
    g.fillRoundedRectangle(bounds, paneCornerRadius);
}

void CustomLookAndFeel::drawTraversalDisplayMenu(juce::Graphics &g, const TraversalDisplayMenu& displaySelector)
{
    auto bounds = displaySelector.getLocalBounds().toFloat().reduced(outerButtonBoundsReduction);
    g.setColour(buttonBarColour);
    g.fillRoundedRectangle(bounds, paneCornerRadius);
}

void CustomLookAndFeel::drawDisplayButton(juce::Graphics &g, const DisplayButton &displayButton)
{
    auto bounds = displayButton.getLocalBounds().toFloat().reduced(outerButtonBoundsReduction);

    if (displayButton.isSelected) {
        g.setColour(buttonColour.darker());
    }
    else {
        g.setColour(buttonColour);
    }

    float triangleHeight = bounds.getHeight() * 0.9f;
    float centerY = bounds.getCentreY();
    float topY = centerY - triangleHeight / 2.0f;
    float bottomY = centerY + triangleHeight / 2.0f;

    juce::Path vPath;
    vPath.startNewSubPath(bounds.getX(), topY);
    vPath.lineTo(bounds.getCentreX(), bottomY);
    vPath.lineTo(bounds.getRight(), topY);
    vPath.closeSubPath();

    g.fillPath(vPath);
}

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

    float maxPulseExpansion = circleFill.getX() - componentBounds.getX();
    float pulseExpansion    = maxPulseExpansion * std::sin(node.pulsePhase * juce::MathConstants<float>::pi);
    auto  pulsedFill        = circleFill.expanded(pulseExpansion);

    if (node.isHighlighted) {
        g.setColour(nodeColour.darker());
    }
    else {
        g.setColour(nodeColour);
    }

    g.fillEllipse(pulsedFill);

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
        float midX = (parentX + childX) * 0.5f;
        float midY = (parentY + childY) * 0.5f;

        float deltaX = float(arrowEndX - parentX);
        float deltaY = float(arrowEndY - parentY);

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

        g.setFont(juce::Font(10.0f));
        g.setColour(juce::Colours::darkgrey);

        float textW = 60.0f;
        float textH = 12.0f;

        g.drawText(labelText, -textW * 0.5f, -(textH * 0.5f + 2.0f), textW, textH,
                   juce::Justification::centred, true);
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

    float arrowLength  = 12.0f;
    float arrowWidth   = 6.0f;
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

    if (isRootTargetArrow || nodeArrow.isGhost) {
        juce::PathStrokeType stroke(2.0f);
        float dashLengths[] = { 6.0f, 10.0f };
        stroke.createDashedStroke(linePath, linePath, dashLengths, 2);
    }

    juce::PathStrokeType lineStroke(2.0f);
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

    if (! nodeArrow.isGhost && nodeArrow.progressT > 0.0f) {
        const juce::Colour progressColour = nodeArrow.startNode->nodeColour.brighter(0.3f);

        float linePathLength = 0.0f;
        {
            juce::PathFlatteningIterator iterator(linePath);
            while (iterator.next())
            {
                const float segmentDeltaX = iterator.x2 - iterator.x1;
                const float segmentDeltaY = iterator.y2 - iterator.y1;
                linePathLength += std::sqrt(segmentDeltaX * segmentDeltaX + segmentDeltaY * segmentDeltaY);
            }
        }

        const float bodyLength        = juce::jmax(0.0f, linePathLength - arrowLength);
        float bodyFraction;
        if (linePathLength > 0.0f) {
            bodyFraction = bodyLength / linePathLength;
        }
        else {
            bodyFraction = 1.0f;
        }
        const float bodyProgressT     = juce::jmin(nodeArrow.progressT, bodyFraction);

        if (bodyProgressT > 0.0f) {
            juce::Path bodyProgressPath = trimPathToFraction(linePath, bodyProgressT);
            if (! bodyProgressPath.isEmpty()) {
                const juce::PathStrokeType bodyStroke(3.0f,
                                                      juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::butt);
                const juce::PathStrokeType bodyGlowStroke(5.5f,
                                                          juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::butt);

                g.setColour(progressColour.withAlpha(0.25f));
                g.strokePath(bodyProgressPath, bodyGlowStroke);

                g.setColour(progressColour);
                g.strokePath(bodyProgressPath, bodyStroke);
            }
        }

        if (nodeArrow.progressT > bodyFraction) {
            const float headFraction       = juce::jmax(1.0e-6f, 1.0f - bodyFraction);
            const float headProgressT      = juce::jlimit(0.0f, 1.0f,
                                                          (nodeArrow.progressT - bodyFraction) / headFraction);

            const float headPerpendicularX =  dirY;
            const float headPerpendicularY = -dirX;

            const float headBaseX          = arrowEndX - arrowLength * dirX;
            const float headBaseY          = arrowEndY - arrowLength * dirY;
            const float headFrontX         = headBaseX + headProgressT * arrowLength * dirX;
            const float headFrontY         = headBaseY + headProgressT * arrowLength * dirY;
            const float headFrontHalfWidth = arrowWidth * (1.0f - headProgressT);

            const float baseLeftX  = headBaseX + arrowWidth * headPerpendicularX;
            const float baseLeftY  = headBaseY + arrowWidth * headPerpendicularY;
            const float baseRightX = headBaseX - arrowWidth * headPerpendicularX;
            const float baseRightY = headBaseY - arrowWidth * headPerpendicularY;
            const float frontLeftX  = headFrontX + headFrontHalfWidth * headPerpendicularX;
            const float frontLeftY  = headFrontY + headFrontHalfWidth * headPerpendicularY;
            const float frontRightX = headFrontX - headFrontHalfWidth * headPerpendicularX;
            const float frontRightY = headFrontY - headFrontHalfWidth * headPerpendicularY;

            juce::Path headProgressPath;
            headProgressPath.startNewSubPath(baseLeftX, baseLeftY);
            headProgressPath.lineTo(frontLeftX, frontLeftY);
            headProgressPath.lineTo(frontRightX, frontRightY);
            headProgressPath.lineTo(baseRightX, baseRightY);
            headProgressPath.closeSubPath();

            const juce::PathStrokeType headGlowStroke(2.5f,
                                                      juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded);

            g.setColour(progressColour.withAlpha(0.25f));
            g.strokePath(headProgressPath, headGlowStroke);

            g.setColour(progressColour);
            g.fillPath(headProgressPath);
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

void CustomLookAndFeel::drawPlayButton(juce::Graphics &g, bool isMouseOver, bool isButtonDown, const PlayButton &button)
{
    auto area = button.getLocalBounds().reduced(5);

    if (isMouseOver) {
        g.setColour(buttonColour.brighter());

    }
    else if (isButtonDown) {
        g.setColour(buttonColour.darker());
    }
    else {
        g.setColour(buttonColour);
    }

    if (button.isOn) {
        juce::Path playButton;
        playButton.startNewSubPath((float)area.getX(), (float)area.getY());
        playButton.lineTo((float)area.getRight(), (float)area.getCentreY());
        playButton.lineTo((float)area.getX(), (float)area.getBottom());
        playButton.closeSubPath();
        g.fillPath(playButton);
    }
    else {
        int barWidth = area.getWidth() / 5;
        int gap = barWidth;
        int barHeight = area.getHeight();
        int x1 = area.getX() + barWidth;
        int x2 = x1 + barWidth + gap;

        juce::Rectangle<int> leftBar(x1, area.getY(), barWidth, barHeight);
        juce::Rectangle<int> rightBar(x2, area.getY(), barWidth, barHeight);

        g.fillRect(leftBar);
        g.fillRect(rightBar);
    }
}

void CustomLookAndFeel::drawSyncButton(juce::Graphics &g, bool isMouseOver, bool isButtonDown, const SyncButton &button)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(outerButtonBoundsReduction);
    if (isMouseOver) {
        g.setColour(buttonColour.brighter());
    }
    else {
        g.setColour(buttonColour);
    }
    g.drawEllipse(bounds, 1.0f);
}

void CustomLookAndFeel::drawNodeButton(juce::Graphics &g, const NodeButton& nodeButton)
{
    auto bounds = nodeButton.getLocalBounds().toFloat().reduced(outerButtonBoundsReduction);

    if (nodeButton.isSelected) {
        g.setColour(buttonColour.darker());
    }
    else {
        g.setColour(buttonColour);
    }
    g.fillEllipse(bounds);
}

void CustomLookAndFeel::drawModulatorButton(juce::Graphics &g, const ModulatorButton &modulatorButton) {
    juce::Rectangle<float> bounds = modulatorButton.getLocalBounds().toFloat().reduced(outerButtonBoundsReduction);

    if (modulatorButton.isSelected) {
        g.setColour(buttonColour.darker());
    }
    else{
        g.setColour(buttonColour);
    }

    g.fillRect(bounds);
    g.drawRect(bounds, 1.0f);
}

void CustomLookAndFeel::drawTraversalFlagButton(juce::Graphics &g, const TraversalFlagButton &traversalFlagButton)
{
    juce::Rectangle<float> bounds = traversalFlagButton.getLocalBounds().toFloat().reduced(outerButtonBoundsReduction);

    if (traversalFlagButton.isSelected) {
        g.setColour(buttonColour.darker());
    }
    else {
        g.setColour(buttonColour);
    }

    juce::Path triangle;
    triangle.startNewSubPath(bounds.getCentreX(), bounds.getY());
    triangle.lineTo(bounds.getRight(), bounds.getBottom());
    triangle.lineTo(bounds.getX(), bounds.getBottom());
    triangle.closeSubPath();

    g.fillPath(triangle);
    g.strokePath(triangle, juce::PathStrokeType(1.0f));
}

void CustomLookAndFeel::drawUndoButton(juce::Graphics &g, const UndoButton &undoButton, bool isButtonDown)
{
    auto area = undoButton.getLocalBounds().reduced(outerButtonBoundsReduction);

    if (isButtonDown) {
        g.setColour(buttonColour.darker());
    }
    else if (undoButton.isHovered) {
        g.setColour(buttonColour.brighter());
    }
    else {
        g.setColour(buttonColour);
    }

    juce::Path path;
    path.startNewSubPath((float)area.getRight(), (float)area.getY());
    path.lineTo((float)area.getX(), (float)area.getCentreY());
    path.lineTo((float)area.getRight(), (float)area.getBottom());
    path.closeSubPath();
    g.fillPath(path);
}

void CustomLookAndFeel::drawRedoButton(juce::Graphics &g, const RedoButton &redoButton, bool isButtonDown)
{
    auto area = redoButton.getLocalBounds().reduced(outerButtonBoundsReduction);

    if (isButtonDown) {
        g.setColour(buttonColour.darker());
    }
    else if (redoButton.isHovered) {
        g.setColour(buttonColour.brighter());
    }
    else {
        g.setColour(buttonColour);
    }

    juce::Path path;
    path.startNewSubPath((float)area.getX(), (float)area.getY());
    path.lineTo((float)area.getRight(), (float)area.getCentreY());
    path.lineTo((float)area.getX(), (float)area.getBottom());
    path.closeSubPath();
    g.fillPath(path);
}

void CustomLookAndFeel::drawUndoRedoPane(juce::Graphics &g,const UndoRedoPane& undoRedoPane)
{
    auto bounds = undoRedoPane.getLocalBounds().reduced(outerButtonBoundsReduction).toFloat();
    g.setColour(buttonBarColour);
    g.fillRoundedRectangle(bounds, paneCornerRadius);
}

void CustomLookAndFeel::drawResetButton(juce::Graphics &g, const ResetButton &resetButton, bool isButtonDown)
{
    auto area = resetButton.getLocalBounds().toFloat().reduced(outerButtonBoundsReduction);

    if (isButtonDown) {
        g.setColour(buttonColour.darker());
    }
    else if (resetButton.isHovered) {
        g.setColour(buttonColour.brighter());
    }
    else {
        g.setColour(buttonColour);
    }

    g.fillRect(area);
}

void CustomLookAndFeel::drawPaintTool(juce::Graphics &g, const PaintTool &paintTool) {

    auto area = paintTool.getLocalBounds().toFloat().reduced(outerButtonBoundsReduction);
    g.setColour(buttonColour);
    g.fillRect(area);

    int wandBoundsReduction = 2;
    auto wandArea = area.reduced(wandBoundsReduction);

    g.setColour(juce::Colours::black);

    const float handleThickness = wandArea.getHeight() * 0.16f;
    const float circleDiameter  = wandArea.getHeight() * 0.4f;
    const float circleRadius    = circleDiameter * 0.5f;

    // Inset both ends by the circle radius so the whole wand stays inside the bounds.
    juce::Point<float> tipCentre   (wandArea.getRight() - circleRadius, wandArea.getY() + circleRadius);
    juce::Point<float> handleStart (wandArea.getX() + circleRadius,     wandArea.getBottom() - circleRadius);

    juce::Line<float> handleLine(handleStart, tipCentre);
    juce::Point<float> handleEnd = handleLine.getPointAlongLineProportionally(1.0f - circleRadius / handleLine.getLength());

    juce::Path handle;
    handle.addLineSegment(juce::Line<float>(handleStart, handleEnd), handleThickness);
    g.fillPath(handle);


    auto tipBounds = juce::Rectangle<float>(circleDiameter, circleDiameter).withCentre(tipCentre);
    g.fillEllipse(tipBounds);
}

void CustomLookAndFeel::drawPaintToolSettings(juce::Graphics &g, const PaintToolSettings &paintToolSettings) {

    auto bounds = paintToolSettings.getLocalBounds();

    g.setColour(buttonColour);
    g.fillRect(bounds);
}

void CustomLookAndFeel::drawNodeTextEditor(juce::Graphics &g, NodeTextEditor &nodeTextEditor) {
}

juce::CaretComponent* CustomLookAndFeel::createCaretComponent(juce::Component* keyFocusOwner) {
    auto* caret = new CustomTextCaret(keyFocusOwner);
    caret->caretColour = baseDarkColour1;
    caret->caretWidth  = 1.0f;
    return caret;
}