//
// Created by Eli Baumgardner on 11/4/25.
//

#include "CustomLookAndFeel.h"
#include "Buttons/ModulatorButton.h"
#include "Buttons/ResetButton.h"
#include "Node/NodeCanvas.h"
#include "Node/Node.h"
#include "../Util/ValueTreeIdentifiers.h"
#include "Titlebar.h"
#include "CustomTextEditor.h"
#include "Node/NodeArrow.h"
#include "Node/NodeTextEditor.h"
#include "Node/CustomTextCaret.h"
#include "Node/RootNode.h"

CustomLookAndFeel::CustomLookAndFeel()
{
    updateColours();
    nodeDropShadow = juce::DropShadow(dropShadowColour.withAlpha(0.4f),6,juce::Point<int>(3,3));
    barDropShadow  = juce::DropShadow(dropShadowColour.withAlpha(0.4f),6,juce::Point<int>(3,3));
}

void CustomLookAndFeel::setColorIntensityFactor(float factor)
{
    colorIntensityFactor = juce::jlimit(0.0f, 1.0f, factor);
    updateColours();

    if (ComponentContext::canvas)
    {
        if (auto* topLevel = ComponentContext::canvas->getTopLevelComponent())
            topLevel->repaint();
    }
}

juce::Colour CustomLookAndFeel::applyIntensity(juce::Colour base) const
{
    float distance        = std::abs(colorIntensityFactor - 0.5f) * 2.0f;
    float hueShift        = (colorIntensityFactor - 0.5f) * 0.5f;
    float saturationBoost = distance * 0.35f;
    float newSaturation   = juce::jlimit(0.0f, 1.0f, base.getSaturation() + saturationBoost);

    // Lift every colour as the slider moves from neutral, lifting darker colours most
    float darkness        = 1.0f - base.getBrightness();
    float brightnessLift  = distance * (0.10f + darkness * 0.25f);
    float newBrightness   = juce::jlimit(0.0f, 1.0f, base.getBrightness() + brightnessLift);

    return base.withRotatedHue(hueShift)
               .withSaturation(newSaturation)
               .withBrightness(newBrightness);
}

void CustomLookAndFeel::updateColours()
{
    darkColour1 = applyIntensity(baseDarkColour1);
    darkColour2 = applyIntensity(baseDarkColour2);
    lightColour1 = applyIntensity(baseLightColour1);
    lightColour2 = applyIntensity(baseLightColour2);
    lightColour3 = applyIntensity(baseLightColour3);

    canvasColour = darkColour1;
    barColour = darkColour2;
    buttonColour = lightColour2;
    editorColour = lightColour2;
    textColour = lightColour3;
    arrowColour = darkColour2.darker();
    arrowHeadColour = arrowColour;
}

void CustomLookAndFeel::drawEditor(juce::Graphics &g, CustomTextEditor& editor)
{
    auto bounds = editor.getLocalBounds().toFloat();
    g.setColour(buttonColour);
    g.fillRoundedRectangle(bounds, paneCornerRadius);

    editor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    editor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    editor.setColour(juce::TextEditor::textColourId, textColour);
    editor.TextEditor::paint(g);
}

void CustomLookAndFeel::drawCanvas(juce::Graphics &g, const NodeCanvas &canvas)
{
    g.fillAll(canvasColour);

    float spacing = canvas.gridSpacing;
    if (spacing < 15.0f) return;

    auto bounds = canvas.getLocalBounds().toFloat();
    float ox = canvas.gridOriginSet ? canvas.gridOrigin.x : bounds.getCentreX();
    float oy = canvas.gridOriginSet ? canvas.gridOrigin.y : bounds.getCentreY();


    const float armLen = 6.0f;
    g.setColour(darkColour2.darker());

    float startX = ox - std::ceil((ox - bounds.getX()) / spacing) * spacing;
    float startY = oy - std::ceil((oy - bounds.getY()) / spacing) * spacing;

    for (float x = startX; x <= bounds.getRight(); x += spacing)
    {
        for (float y = startY; y <= bounds.getBottom(); y += spacing)
        {
            g.drawLine(x - armLen, y, x + armLen, y, 1.0f);
            g.drawLine(x, y - armLen, x, y + armLen, 1.0f);
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

void CustomLookAndFeel::drawButtonPane(juce::Graphics &g, const ButtonPane& selectionBar)
{
    auto bounds = selectionBar.getLocalBounds().reduced(2.0f).toFloat();
    g.setColour(buttonColour);
    g.fillRoundedRectangle(bounds, paneCornerRadius);
}

void CustomLookAndFeel::drawTempoDisplay(juce::Graphics &g, const TempoDisplay &display) {
}


void CustomLookAndFeel::drawDisplayMenu(juce::Graphics &g, const DisplayMenu& displaySelector)
{
    auto bounds = displaySelector.getLocalBounds().toFloat().reduced(buttonBoundsReduction);
    g.setColour(buttonColour);
    g.fillRoundedRectangle(bounds, paneCornerRadius);
}

void CustomLookAndFeel::drawDisplayButton(juce::Graphics &g, const DisplayButton &displayButton)
{
    auto bounds = displayButton.getLocalBounds().toFloat().reduced(5.0f);

    g.setColour(displayButton.isSelected ? textColour.darker() : textColour);

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

void CustomLookAndFeel::drawNode(juce::Graphics& g, const Node& node, juce::Rectangle<float> componentBounds)
{
    juce::Rectangle<float> bounds      = componentBounds.reduced(1.5f);
    juce::Rectangle<float> circleBounds = bounds.reduced(6.0f);
    juce::Rectangle<float> circleFill   = circleBounds.reduced(0.5f);
    juce::Rectangle<float> circleSelect = bounds.reduced(4.0f);
    juce::Rectangle<float> circleHover  = bounds.reduced(5.5f);

    juce::Path nodePath;
    nodePath.addEllipse(circleBounds);
    nodeDropShadow.drawForPath(g, nodePath);

    float velocity        = (float)(int)node.midiNoteData.getProperty(ValueTreeIdentifiers::MidiVelocity, 100);
    float brightnessFactor = juce::jmap(velocity, 0.0f, 127.0f, 0.4f, 1.6f);

    juce::Colour nodeColour = applyIntensity(node.nodeColour).withMultipliedBrightness(brightnessFactor);

    float pulseExpansion = 4.0f * std::sin(node.pulsePhase * juce::MathConstants<float>::pi);
    auto  pulsedFill     = circleFill.expanded(pulseExpansion);

    g.setColour(node.isHighlighted ? nodeColour.darker() : nodeColour);
    g.fillEllipse(pulsedFill);

    if (node.isHovered)   { g.drawEllipse(circleHover, 2.0f); }

    if (node.isSelected)
    {
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

    g.setColour(darkColour2.darker());
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
    if (labelText.isNotEmpty() && nodeArrow.animT > 0.8f)
    {
        float midX = (parentX + childX) * 0.5f;
        float midY = (parentY + childY) * 0.5f;
        float angle = std::atan2(arrowEndY - parentY, arrowEndX - parentX);

        const float halfPi = juce::MathConstants<float>::halfPi;
        while (angle >  halfPi) angle -= juce::MathConstants<float>::pi;
        while (angle <= -halfPi) angle += juce::MathConstants<float>::pi;

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
        if (t <= 0.0f || source.isEmpty()) return {};
        if (t >= 1.0f) return source;

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

            if (accumulated + segLen >= target)
            {
                const float remain   = target - accumulated;
                const float fraction = segLen > 0.0f ? remain / segLen : 0.0f;
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
    auto* a = nodeArrow.parentNode;
    auto* b = nodeArrow.childNode;

    auto parentCentre = a->getNodeCentre();
    auto childCentre  = b->getNodeCentre();

    float arrowLength  = 12.0f;
    float arrowWidth   = 6.0f;
    int   childRadius  = b->getHeight() / 2;
    int   parentRadius = a->getHeight() / 2;

    g.setColour(arrowColour);
    const float ghostAlpha = nodeArrow.isGhost ? 0.5f : 1.0f;

    float parentCenterX = float(parentCentre.x - nodeArrow.getX());
    float parentCenterY = float(parentCentre.y - nodeArrow.getY());
    float childCenterX  = float(childCentre.x  - nodeArrow.getX());
    float childCenterY  = float(childCentre.y  - nodeArrow.getY());

    float arrowEndX = parentCenterX + (childCenterX - parentCenterX) * nodeArrow.animT;
    float arrowEndY = parentCenterY + (childCenterY - parentCenterY) * nodeArrow.animT;

    float deltaX = arrowEndX - parentCenterX;
    float deltaY = arrowEndY - parentCenterY;
    float length = std::sqrt(deltaX * deltaX + deltaY * deltaY);

    if (length < 1.0f) return;

    float dirX = deltaX / length;
    float dirY = deltaY / length;

    arrowEndX -= dirX * float(childRadius);
    arrowEndY -= dirY * float(childRadius);

    bool isConnectorArrow  = a->nodeType == NodeType::Connector
                          || b->nodeType == NodeType::Connector;
    bool isRootTargetArrow = b->nodeType == NodeType::Root;

    juce::Path linePath;

    if (a->nodeType == NodeType::Connector)
    {
        float connectorDirX = std::cos(a->incomingAngle);
        float connectorDirY = std::sin(a->incomingAngle);

        {
            float roughX = arrowEndX - parentCenterX;
            float roughY = arrowEndY - parentCenterY;
            float roughLen = std::sqrt(roughX * roughX + roughY * roughY);
            if (roughLen > 1.0f && connectorDirX * (roughX / roughLen) + connectorDirY * (roughY / roughLen) < 0.0f)
            {
                connectorDirX = roughX / roughLen;
                connectorDirY = roughY / roughLen;
            }
        }

        float startX = parentCenterX + float(parentRadius) * connectorDirX;
        float startY = parentCenterY + float(parentRadius) * connectorDirY;

        float toEndX   = arrowEndX - startX;
        float toEndY   = arrowEndY - startY;
        float toEndLen = std::sqrt(toEndX * toEndX + toEndY * toEndY);

        if (toEndLen < 1.0f) return;

        float toEndDirX     = toEndX / toEndLen;
        float toEndDirY     = toEndY / toEndLen;
        float tangentLength = toEndLen * 0.4f;

        float cp1X = startX + tangentLength * connectorDirX;
        float cp1Y = startY + tangentLength * connectorDirY;
        float cp2X = arrowEndX - tangentLength * toEndDirX;
        float cp2Y = arrowEndY - tangentLength * toEndDirY;

        linePath.startNewSubPath(startX, startY);
        linePath.cubicTo(cp1X, cp1Y, cp2X, cp2Y, arrowEndX, arrowEndY);

        dirX = toEndDirX;
        dirY = toEndDirY;
    }
    else
    {
        float dx = arrowEndX - parentCenterX;
        float dy = arrowEndY - parentCenterY;
        float absDx = std::abs(dx);
        float absDy = std::abs(dy);

        // Straight line when nodes are aligned horizontally or vertically
        if (absDx < 1.0f || absDy < 1.0f)
        {
            linePath.startNewSubPath(parentCenterX, parentCenterY);
            linePath.lineTo(arrowEndX, arrowEndY);
        }
        else
        {
            // Gentle S-curve: a straight arrow, wiggled slightly.
            // Perpendicular to the main direction, flipped when going left
            // so the curve bows the same visual direction regardless of dx sign.
            float sign  = (dx >= 0.0f) ? 1.0f : -1.0f;
            float perpX = -dirY * 0.8f * sign;
            float perpY =  dirX * 0.8f * sign;

            // Subtle perpendicular offset for the S-shape
            float segLen = std::sqrt(dx * dx + dy * dy);
            float offset = segLen * 0.15f;

            // Control points at 1/3 and 2/3 along the line,
            // pushed to opposite sides of the straight path
            float cp1X = parentCenterX + dx * 0.33f + perpX * offset;
            float cp1Y = parentCenterY + dy * 0.33f + perpY * offset;
            float cp2X = parentCenterX + dx * 0.67f - perpX * offset;
            float cp2Y = parentCenterY + dy * 0.67f - perpY * offset;

            linePath.startNewSubPath(parentCenterX, parentCenterY);
            linePath.cubicTo(cp1X, cp1Y, cp2X, cp2Y, arrowEndX, arrowEndY);

            // Arrowhead follows the "neck" — the tangent where the curve meets the tip
            float neckX = arrowEndX - cp2X;
            float neckY = arrowEndY - cp2Y;
            float neckLen = std::sqrt(neckX * neckX + neckY * neckY);
            if (neckLen > 0.0f)
            {
                dirX = neckX / neckLen;
                dirY = neckY / neckLen;
            }
        }
    }

    if (isConnectorArrow || isRootTargetArrow || nodeArrow.isGhost)
    {
        juce::PathStrokeType stroke(2.0f);
        float dashLengths[] = { 6.0f, 10.0f };
        stroke.createDashedStroke(linePath, linePath, dashLengths, 2);
    }

    // Subtle engraved look: thin shadow/highlight offsets around the main stroke
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

    if (! nodeArrow.isGhost && nodeArrow.progressT > 0.0f)
    {
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
        const float bodyFraction      = (linePathLength > 0.0f) ? bodyLength / linePathLength : 1.0f;
        const float bodyProgressT     = juce::jmin(nodeArrow.progressT, bodyFraction);
        const juce::Colour progressColour = lightColour1;

        if (bodyProgressT > 0.0f)
        {
            juce::Path bodyProgressPath = trimPathToFraction(linePath, bodyProgressT);
            if (! bodyProgressPath.isEmpty())
            {
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

        if (nodeArrow.progressT > bodyFraction)
        {
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

    if (nodeArrow.animT > 0.3f)
    {
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
    auto bgArea = button.getLocalBounds().toFloat().reduced(2.0f);
    if (isMouseOver)
    {
        g.setColour(buttonColour.withAlpha(0.45f));
        g.fillEllipse(bgArea);
    }

    auto area = button.getLocalBounds().reduced(5);
    g.setColour(isButtonDown ? lightColour3.darker() : (isMouseOver ? lightColour3 : buttonColour));

    if (button.isOn){
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
    auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    if (isMouseOver)
    {
        g.setColour(textColour.withAlpha(0.15f));
        g.fillEllipse(bounds);
    }
    g.setColour(textColour);
    g.drawEllipse(bounds, 1.0f);
}

void CustomLookAndFeel::drawNodeButton(juce::Graphics &g, const NodeButton& nodeButton)
{
    auto bounds = nodeButton.getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(nodeButton.isSelected ? lightColour3.darker() : lightColour3);
    g.fillEllipse(bounds);
}

void CustomLookAndFeel::drawTraverserButton(juce::Graphics& g, const ConnectorButton& traverserButton)
{
    auto bounds = traverserButton.getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(traverserButton.isSelected ? lightColour3.darker() : lightColour3);

    juce::Path triangle;
    triangle.startNewSubPath(bounds.getCentreX(), bounds.getY());
    triangle.lineTo(bounds.getRight(), bounds.getBottom());
    triangle.lineTo(bounds.getX(), bounds.getBottom());
    triangle.closeSubPath();

    g.fillPath(triangle);
}

void CustomLookAndFeel::drawModulatorButton(juce::Graphics &g, const ModulatorButton &modulatorButton) {
    auto bounds = modulatorButton.getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(modulatorButton.isSelected ? lightColour3.darker() : lightColour3);

    auto rect = bounds;

    g.fillRect(rect);
    g.drawRect(rect, 1.0f);
}


void CustomLookAndFeel::drawUndoButton(juce::Graphics &g, const UndoButton &undoButton, bool isButtonDown)
{
    auto area = undoButton.getLocalBounds().reduced(5);

    if (isButtonDown)          { g.setColour(lightColour3.darker()); }
    else if (undoButton.isHovered) { g.setColour(lightColour3.brighter(0.2f)); }
    else                       { g.setColour(lightColour3);   }

    juce::Path path;
    path.startNewSubPath((float)area.getRight(), (float)area.getY());
    path.lineTo((float)area.getX(), (float)area.getCentreY());
    path.lineTo((float)area.getRight(), (float)area.getBottom());
    path.closeSubPath();
    g.fillPath(path);
}

void CustomLookAndFeel::drawRedoButton(juce::Graphics &g, const RedoButton &redoButton, bool isButtonDown)
{
    auto area = redoButton.getLocalBounds().reduced(5);

    if (isButtonDown)          { g.setColour(lightColour3.darker()); }
    else if (redoButton.isHovered) { g.setColour(lightColour3.brighter(0.2f)); }
    else                       { g.setColour(lightColour3);   }

    juce::Path path;
    path.startNewSubPath((float)area.getX(), (float)area.getY());
    path.lineTo((float)area.getRight(), (float)area.getCentreY());
    path.lineTo((float)area.getX(), (float)area.getBottom());
    path.closeSubPath();
    g.fillPath(path);
}

void CustomLookAndFeel::drawUndoRedoPane(juce::Graphics &g,const UndoRedoPane& undoRedoPane)
{
    auto bounds = undoRedoPane.getLocalBounds().reduced(2.0f).toFloat();
    g.setColour(buttonColour);
    g.fillRoundedRectangle(bounds, paneCornerRadius);
}

void CustomLookAndFeel::drawResetButton(juce::Graphics &g, const ResetButton &resetButton, bool isButtonDown)
{
    auto bgArea = resetButton.getLocalBounds().toFloat().reduced(2.0f);
    if (resetButton.isHovered)
    {
        g.setColour(buttonColour.withAlpha(0.45f));
        g.fillEllipse(bgArea);
    }

    auto area = resetButton.getLocalBounds().toFloat().reduced(5.0f);
    g.setColour(isButtonDown ? lightColour3.darker() : (resetButton.isHovered ? lightColour3.brighter(0.15f) : lightColour3));
    g.fillRect(area);
}

void CustomLookAndFeel::drawNodeTextEditor(juce::Graphics &g, NodeTextEditor &nodeTextEditor) {
}

juce::CaretComponent* CustomLookAndFeel::createCaretComponent(juce::Component* keyFocusOwner) {
    auto* caret = new CustomTextCaret(keyFocusOwner);
    caret->caretColour = darkColour1;
    caret->caretWidth  = 1.0f;
    return caret;
}