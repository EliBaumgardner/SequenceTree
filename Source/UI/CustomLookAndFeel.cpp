//
// Created by Eli Baumgardner on 11/4/25.
//

#include "CustomLookAndFeel.h"
#include "Node/NodeCanvas.h"
#include "Node/Node.h"
#include "Titlebar.h"
#include "CustomTextEditor.h"
#include "Node/NodeArrow.h"

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

void CustomLookAndFeel::drawNodeArrow(juce::Graphics &g, const NodeArrow& nodeArrow)
{
    auto* a = nodeArrow.parentNode;
    auto* b = nodeArrow.childNode;

    auto aBounds = a->getBounds();
    auto bBounds = b->getBounds();

    float arrowLength = 10.0f;
    float arrowWidth = 5.0f;
    int radius = bBounds.getWidth() / 2;

    g.setColour(arrowColour);

    float x1 = float(aBounds.getCentreX() - nodeArrow.getX());
    float y1 = float(aBounds.getCentreY() - nodeArrow.getY());
    float tx2 = float(bBounds.getCentreX() - nodeArrow.getX());
    float ty2 = float(bBounds.getCentreY() - nodeArrow.getY());

    // Apply rubber-band spring animation: interpolate endpoint toward child
    float x2 = x1 + (tx2 - x1) * nodeArrow.animT;
    float y2 = y1 + (ty2 - y1) * nodeArrow.animT;

    // Calculate direction vector
    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = std::sqrt(dx*dx + dy*dy);

    if (length < 1.0f) return;

    float nx = dx / length;
    float ny = dy / length;

    x2 = x2 - nx * float(radius);
    y2 = y2 - ny * float(radius);

    // Draw main line
    g.drawLine(x1, y1, x2, y2, 2.0f);

    // Arrowhead points — only draw when arrow is mostly extended
    if (nodeArrow.animT > 0.3f)
    {
        float leftX  = x2 - arrowLength * nx + arrowWidth * ny;
        float leftY  = y2 - arrowLength * ny - arrowWidth * nx;
        float rightX = x2 - arrowLength * nx - arrowWidth * ny;
        float rightY = y2 - arrowLength * ny + arrowWidth * nx;

        g.drawLine(x2, y2, leftX, leftY, 1.0f);
        g.drawLine(x2, y2, rightX, rightY, 1.0f);
    }
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