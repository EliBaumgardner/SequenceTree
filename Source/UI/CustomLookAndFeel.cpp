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
    nodeDropShadow = juce::DropShadow(dropShadowColour.withAlpha(0.4f),6,juce::Point<int>(3,3));
    barDropShadow  = juce::DropShadow(dropShadowColour.withAlpha(0.4f),6,juce::Point<int>(3,3));
}

void CustomLookAndFeel::drawEditor(juce::Graphics &g, CustomTextEditor& editor)
{
    editor.TextEditor::paint(g);
}

void CustomLookAndFeel::drawCanvas(juce::Graphics &g, const NodeCanvas &canvas)
{
    g.fillAll(canvasColour);

    if (!canvas.showGrid) return;

    float spacing = canvas.gridSpacing;
    if (spacing < 15.0f) return;

    auto bounds = canvas.getLocalBounds().toFloat();
    float ox = canvas.gridOrigin.x;
    float oy = canvas.gridOrigin.y;

    g.setColour(juce::Colour(0x22000000));

    float startX = ox - std::ceil((ox - bounds.getX()) / spacing) * spacing;
    for (float x = startX; x <= bounds.getRight(); x += spacing)
        g.drawVerticalLine(int(x), bounds.getY(), bounds.getBottom());

    float startY = oy - std::ceil((oy - bounds.getY()) / spacing) * spacing;
    for (float y = startY; y <= bounds.getBottom(); y += spacing)
        g.drawHorizontalLine(int(y), bounds.getX(), bounds.getRight());

    // Highlight the parent origin point
    g.setColour(juce::Colour(0x55000000));
    g.drawEllipse(ox - 4.0f, oy - 4.0f, 8.0f, 8.0f, 1.5f);
}

void CustomLookAndFeel::drawTitleBar(juce::Graphics &g, const Titlebar &titleBar)
{
    auto bounds = titleBar.getLocalBounds().toFloat().withTrimmedBottom(2.0f);

    juce::Path rectPath;
    rectPath.addRectangle(bounds);

    barDropShadow.drawForPath(g, rectPath);

    g.setColour(barColour);
    g.fillRect(bounds);
}

void CustomLookAndFeel::drawButtonPane(juce::Graphics &g, const ButtonPane& selectionBar)
{
    auto bounds = selectionBar.getLocalBounds().reduced(2.0f).toFloat();
    g.setColour(buttonColour);
    g.fillRect(bounds);
}

void CustomLookAndFeel::drawTempoDisplay(juce::Graphics &g, const TempoDisplay &display) {
    g.setColour(buttonColour);
    g.fillAll();
}


void CustomLookAndFeel::drawDisplayMenu(juce::Graphics &g, const DisplayMenu& displaySelector)
{
    auto bounds = displaySelector.getLocalBounds().toFloat().reduced(buttonBoundsReduction);
    g.setColour(buttonColour);
    g.fillRect(bounds);

}

void CustomLookAndFeel::drawDisplayButton(juce::Graphics &g, const DisplayButton &displayButton)
{
    g.setColour(displayButton.isSelected ? lightColour3.darker() : lightColour3);

    auto bounds = displayButton.getLocalBounds().toFloat().reduced(5.0f);

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

void CustomLookAndFeel::drawNode(juce::Graphics& g,const Node& node)
{
    juce::Rectangle<float> bounds = node.getLocalBounds().toFloat().reduced(1.5);
    juce::Rectangle<float> circleBounds = bounds.reduced(6.0f);
    juce::Rectangle<float> circleFill = circleBounds.reduced(0.5f);

    juce::Rectangle<float> circleSelect = bounds.reduced(4.0f);
    juce::Rectangle<float> circleHover = bounds.reduced(5.5f);

    juce::Path nodePath;
    nodePath.addEllipse(circleBounds);
    nodeDropShadow.drawForPath(g, nodePath);

    float velocity = (float)(int)node.midiNoteData.getProperty(ValueTreeIdentifiers::MidiVelocity, 100);
    float brightnessFactor = juce::jmap(velocity, 0.0f, 127.0f, 0.4f, 1.6f);

    juce::Colour nodeColour = node.nodeColour.withMultipliedBrightness(brightnessFactor);

    float pulseExpansion = 4.0f * std::sin(node.pulsePhase * juce::MathConstants<float>::pi);
    auto pulsedFill = circleFill.expanded(pulseExpansion);

    g.setColour(node.isHighlighted ? nodeColour.darker() : nodeColour);

    g.fillEllipse(pulsedFill);

    if (node.isHovered) { g.drawEllipse(circleHover, 2.0f); }

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

void CustomLookAndFeel::drawRootNode(juce::Graphics& g,const RootNode& node) {

    drawNode(g, node);
}

void CustomLookAndFeel::drawRootNodeRectangle(juce::Graphics &g, const RootRectangle &rootRectangle) {

    juce::Rectangle<float> rootBounds = rootRectangle.getLocalBounds().toFloat();

    g.setColour(darkColour2);
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

void CustomLookAndFeel::drawNodeArrow(juce::Graphics &g, const NodeArrow& nodeArrow, const juce::TextEditor& editor)
{
    auto* a = nodeArrow.parentNode;
    auto* b = nodeArrow.childNode;

    auto parentNodeBounds = a->getBounds();
    auto childNodeBounds  = b->getBounds();

    float arrowLength  = 12.0f;
    float arrowWidth   = 6.0f;
    int   childRadius  = childNodeBounds.getWidth()  / 2;
    int   parentRadius = parentNodeBounds.getWidth() / 2;

    g.setColour(arrowColour);

    float parentCenterX = float(parentNodeBounds.getCentreX() - nodeArrow.getX());
    float parentCenterY = float(parentNodeBounds.getCentreY() - nodeArrow.getY());
    float childCenterX  = float(childNodeBounds.getCentreX()  - nodeArrow.getX());
    float childCenterY  = float(childNodeBounds.getCentreY()  - nodeArrow.getY());

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

    bool isConnectorArrow = a->nodeType == NodeType::Connector
                         || b->nodeType == NodeType::Connector;

    juce::Path linePath;

    if (a->nodeType == NodeType::Connector)
    {
        float connectorDirX = std::cos(a->incomingAngle);
        float connectorDirY = std::sin(a->incomingAngle);

        // If exit direction points away from child, fall back to direct direction.
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

    if (isConnectorArrow)
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
    g.setColour(arrowColour.darker(0.4f).withAlpha(0.35f));
    g.strokePath(shadowPath,    lineStroke);
    g.setColour(arrowColour.brighter(0.4f).withAlpha(0.18f));
    g.strokePath(highlightPath, lineStroke);
    g.setColour(arrowColour);
    g.strokePath(linePath, lineStroke);

    if (nodeArrow.animT > 0.3f)
    {
        float leftX  = arrowEndX - arrowLength * dirX + arrowWidth * dirY;
        float leftY  = arrowEndY - arrowLength * dirY - arrowWidth * dirX;
        float rightX = arrowEndX - arrowLength * dirX - arrowWidth * dirY;
        float rightY = arrowEndY - arrowLength * dirY + arrowWidth * dirX;

        // Engraved arrowhead: fill main, then shadow/highlight strokes around the outline
        juce::Path arrowHead;
        arrowHead.startNewSubPath(leftX, leftY);
        arrowHead.lineTo(arrowEndX, arrowEndY);
        arrowHead.lineTo(rightX, rightY);
        arrowHead.closeSubPath();

        g.setColour(arrowHeadColour);
        g.fillPath(arrowHead);

        auto headShadow    = arrowHead;
        auto headHighlight = arrowHead;
        headShadow   .applyTransform(juce::AffineTransform::translation( 0.5f,  0.5f));
        headHighlight.applyTransform(juce::AffineTransform::translation(-0.5f, -0.5f));

        juce::PathStrokeType headStroke(1.0f);
        g.setColour(arrowHeadColour.darker(0.5f).withAlpha(0.4f));
        g.strokePath(headShadow,    headStroke);
        g.setColour(arrowHeadColour.brighter(0.4f).withAlpha(0.2f));
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
    g.setColour(buttonColour);

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
    juce::Rectangle<float> bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(lightColour3);
    g.fillEllipse(bounds);
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

    if (isButtonDown) { g.setColour(lightColour3.darker()); }
    else              { g.setColour(lightColour3);   }

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

    if (isButtonDown) { g.setColour(lightColour3.darker()); }
    else              { g.setColour(lightColour3);   }

    juce::Path playButton;
    playButton.startNewSubPath((float)area.getX(), (float)area.getY());
    playButton.lineTo((float)area.getRight(), (float)area.getCentreY());
    playButton.lineTo((float)area.getX(), (float)area.getBottom());
    playButton.closeSubPath();
    g.fillPath(playButton);
}

void CustomLookAndFeel::drawUndoRedoPane(juce::Graphics &g,const UndoRedoPane& undoRedoPane)
{
    auto bounds = undoRedoPane.getLocalBounds().reduced(2.0f).toFloat();
    g.setColour(buttonColour);
    g.fillRect(bounds);
}

void CustomLookAndFeel::drawResetButton(juce::Graphics &g, const ResetButton &resetButton, bool isButtonDown)
{
    auto area = resetButton.getLocalBounds().toFloat().reduced(5.0f);

    g.setColour(isButtonDown ? lightColour3.darker() : lightColour3);
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