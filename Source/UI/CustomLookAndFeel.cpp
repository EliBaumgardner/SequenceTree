//
// Created by Eli Baumgardner on 11/4/25.
//

#include "CustomLookAndFeel.h"
#include "Node/NodeCanvas.h"
#include "Node/Node.h"
#include "Titlebar.h"
#include "CustomTextEditor.h"
#include "Node/NodeArrow.h"
#include "Node/NodeTextEditor.h"
#include "Node/CustomTextCaret.h"

CustomLookAndFeel::CustomLookAndFeel()
{
    nodeDropShadow = juce::DropShadow(dropShadowColour.withAlpha(0.4f),6,juce::Point<int>(3,3));
    barDropShadow  = juce::DropShadow(dropShadowColour.withAlpha(0.4f),6,juce::Point<int>(3,3));
}

void CustomLookAndFeel::drawEditor(juce::Graphics &g, CustomTextEditor& editor)
{
    if (auto* dynamicEditor = dynamic_cast<CustomTextEditor*>(&editor)) {
        dynamicEditor->setColour(juce::TextEditor::backgroundColourId, editorColour);
        dynamicEditor->setColour(juce::TextEditor::outlineColourId, editorColour);
        dynamicEditor->setColour(juce::TextEditor::textColourId, textColour); // choose text color
        dynamicEditor->setColour(juce::TextEditor::highlightColourId, juce::Colours::darkgrey); // optional for selection
        dynamicEditor->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
        dynamicEditor->TextEditor::paint(g);
    }
}

void CustomLookAndFeel::drawCanvas(juce::Graphics &g, const NodeCanvas &canvas) { g.fillAll(canvasColour); }

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

void CustomLookAndFeel::drawNode(juce::Graphics& g,const Node& node)
{
    juce::Path nodePath;
    nodePath.addEllipse(node.getLocalBounds().toFloat().reduced(6.0f));
    nodeDropShadow.drawForPath(g, nodePath);

    auto bounds = node.getLocalBounds().toFloat();
    auto circleSelect = bounds.reduced(0.5f);
    auto circleHover = bounds.reduced(4.5f);
    auto circleFill = bounds.reduced(5.5f);

    g.setColour(node.isHighlighted ? node.nodeColour.darker() : node.nodeColour);
    g.fillEllipse(circleFill);

    if (node.isHovered) { g.drawEllipse(circleHover, 2.0f); }

    if (node.isSelected)
    {
        juce::Path dottedPath;
        dottedPath.addEllipse(circleSelect);

        juce::PathStrokeType stroke(0.325f);

        float dashLengths[] = { 2.935f, 2.935f };
        stroke.createDashedStroke(dottedPath, dottedPath, dashLengths, 2);

        g.setColour(juce::Colours::black);
        g.strokePath(dottedPath, stroke);
    }
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

    float arrowLength  = 10.0f;
    float arrowWidth   = 5.0f;
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

    if (a->nodeType == NodeType::Connector || a->nodeType == NodeType::Node)
    {
        float connectorDirX = std::cos(a->incomingAngle);
        float connectorDirY = std::sin(a->incomingAngle);

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
        linePath.startNewSubPath(parentCenterX, parentCenterY);
        linePath.lineTo(arrowEndX, arrowEndY);
    }

    if (isConnectorArrow)
    {
        juce::PathStrokeType stroke(2.0f);
        float dashLengths[] = { 6.0f, 10.0f };
        stroke.createDashedStroke(linePath, linePath, dashLengths, 2);
    }

    g.strokePath(linePath, juce::PathStrokeType(2.0f));

    if (nodeArrow.animT > 0.3f)
    {
        float leftX  = arrowEndX - arrowLength * dirX + arrowWidth * dirY;
        float leftY  = arrowEndY - arrowLength * dirY - arrowWidth * dirX;
        float rightX = arrowEndX - arrowLength * dirX - arrowWidth * dirY;
        float rightY = arrowEndY - arrowLength * dirY + arrowWidth * dirX;

        g.drawLine(arrowEndX, arrowEndY, leftX, leftY, 1.0f);
        g.drawLine(arrowEndX, arrowEndY, rightX, rightY, 1.0f);
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

void CustomLookAndFeel::drawNodeTextEditor(juce::Graphics &g, NodeTextEditor &nodeTextEditor) {
}

juce::CaretComponent* CustomLookAndFeel::createCaretComponent(juce::Component* keyFocusOwner) {
    auto* caret = new CustomTextCaret(keyFocusOwner);
    caret->caretColour = darkColour1;
    caret->caretWidth  = 1.0f;
    return caret;
}