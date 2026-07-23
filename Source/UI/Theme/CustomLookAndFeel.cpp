//
// Created by Eli Baumgardner on 11/4/25.
//

#include "CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"
#include "CustomTextEditor.h"
#include "CustomTextCaret.h"

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

void CustomLookAndFeel::drawBar(juce::Graphics& g, juce::Rectangle<float> bounds, bool isLitFromTop)
{
    juce::Colour topColour    = barColour.darker(0.04f);
    juce::Colour bottomColour = barColour.brighter(0.06f);

    juce::Colour topEdge    = juce::Colours::black.withAlpha(0.35f);
    juce::Colour bottomEdge = barColour.brighter(0.12f);

    if (isLitFromTop) {
        std::swap(topColour, bottomColour);
        std::swap(topEdge, bottomEdge);
    }

    g.setGradientFill(juce::ColourGradient(topColour,    0, bounds.getY(),
                                           bottomColour, 0, bounds.getBottom(), false));
    g.fillRect(bounds);

    g.setColour(topEdge);
    g.drawHorizontalLine((int) bounds.getY(), bounds.getX(), bounds.getRight());

    g.setColour(bottomEdge);
    g.drawHorizontalLine((int) bounds.getBottom() - 1, bounds.getX(), bounds.getRight());
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

juce::CaretComponent* CustomLookAndFeel::createCaretComponent(juce::Component* keyFocusOwner) {
    auto* caret = new CustomTextCaret(keyFocusOwner);
    caret->caretColour = baseDarkColour1;
    caret->caretWidth  = 1.0f;
    return caret;
}
