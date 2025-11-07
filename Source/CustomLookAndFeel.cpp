//
// Created by Eli Baumgardner on 11/4/25.
//

#include "CustomLookAndFeel.h"
#include "Node/NodeCanvas.h"
#include "Node/Node.h"
#include "UI/TitleBar.h"
#include "UI/SelectionBar.h"
#include "UI/DynamicEditor.h"

CustomLookAndFeel::CustomLookAndFeel()
{
    nodeDropShadow         = juce::DropShadow(dropShadowColour.withAlpha(0.4f),6,juce::Point<int>(3,3));
    barDropShadow = juce::DropShadow( dropShadowColour.withAlpha(0.4f),6,juce::Point<int>(3,3));
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

void CustomLookAndFeel::drawNodeArrow(juce::Graphics &g, const NodeArrow &nodeArrow)
{
    auto* a = nodeArrow.startNode;
    auto* b = nodeArrow.endNode;

    auto aBounds = a->getBounds(); auto bBounds = b->getBounds();

    // Arrow size
    float arrowLength = 10.0f; float arrowWidth = 5.0f;
    int radius = b->getBounds().getWidth()/2;

    g.setColour(arrowColour);
    int x1 = aBounds.getCentreX() - nodeArrow.getX();
    int y1 = aBounds.getCentreY() - nodeArrow.getY();
    int x2 = bBounds.getCentreX() - nodeArrow.getX();
    int y2 = bBounds.getCentreY() - nodeArrow.getY();

    // Calculate direction vector
    float dx = float(x2 - x1);
    float dy = float(y2 - y1);
    float length = std::sqrt(dx*dx + dy*dy);

    // Normalize direction
    float nx = dx / length;
    float ny = dy / length;

    x2 = x2 - nx*radius;
    y2 = y2 - ny*radius;

    // Draw main line
    g.drawLine(x1, y1, x2, y2, 2.0f);

    // Calculate the two points for the arrowhead lines
    float leftX = x2 - arrowLength * nx + arrowWidth * ny;
    float leftY = y2 - arrowLength * ny - arrowWidth * nx;

    float rightX = x2 - arrowLength * nx - arrowWidth * ny;
    float rightY = y2 - arrowLength * ny + arrowWidth * nx;

    // Draw arrowhead lines
    g.drawLine(x2, y2, leftX, leftY, 1.0f);
    g.drawLine(x2, y2, rightX, rightY, 1.0f);
}

void CustomLookAndFeel::drawTitleBar(juce::Graphics &g, const TitleBar &titleBar)
{
    auto bounds = titleBar.getLocalBounds().toFloat().reduced(2.0f);

    juce::Path rectPath;
    rectPath.addRectangle(bounds);

    barDropShadow.drawForPath(g, rectPath);

    g.setColour(barColour);
    g.fillRect(bounds);
}

void CustomLookAndFeel::drawCanvas(juce::Graphics &g, const NodeCanvas &canvas) { g.fillAll(canvasColour); }

void CustomLookAndFeel::MenuBar(juce::Graphics &g, const SelectionBar &selectionBar)
{
    auto bounds = selectionBar.getLocalBounds().reduced(2.0f).toFloat();
    g.setColour(barColour);
    g.fillRect(bounds);
}

void CustomLookAndFeel::drawPlayButton(juce::Graphics &g, bool isMouseOver, bool isButtonDown, const PlayButton &button)
{
    auto area = button.getLocalBounds().reduced(5); // stay inside our component bounds
    g.setColour(buttonColour);

    if (button.isOn)
    {
        // Draw play triangle ( > )
        juce::Path playButton;
        playButton.startNewSubPath((float)area.getX(), (float)area.getY());
        playButton.lineTo((float)area.getRight(), (float)area.getCentreY());
        playButton.lineTo((float)area.getX(), (float)area.getBottom());
        playButton.closeSubPath(); // complete the triangle
        g.fillPath(playButton); // use fill instead of stroke for solid arrow
    }
    else
    {
        // Draw pause bars ( | | )
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

void CustomLookAndFeel::drawSyncButton(juce::Graphics &g, bool isMouseOver, bool isButtonDown, const SyncButton &button) {

    juce::Rectangle<float> bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(buttonColour);
    g.fillEllipse(bounds);
}

void CustomLookAndFeel::drawEditor(juce::Graphics &g, DynamicEditor& editor) {

    if (auto* dynamicEditor = dynamic_cast<DynamicEditor*>(&editor)) {
        dynamicEditor->setColour(juce::TextEditor::backgroundColourId, editorColour);
        dynamicEditor->setColour(juce::TextEditor::outlineColourId, editorColour);
        dynamicEditor->setColour(juce::TextEditor::textColourId, textColour); // choose text color
        dynamicEditor->setColour(juce::TextEditor::highlightColourId, juce::Colours::darkgrey); // optional for selection
        dynamicEditor->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
        dynamicEditor->TextEditor::paint(g);
    }
}
