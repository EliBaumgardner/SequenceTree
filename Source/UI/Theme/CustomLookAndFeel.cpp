//
// Created by Eli Baumgardner on 11/4/25.
//

#include "CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"
#include "CustomTextEditor.h"
#include "../Node/NodeTextEditor.h"
#include "CustomTextCaret.h"

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

void CustomLookAndFeel::drawNodeTextEditor(juce::Graphics &g, NodeTextEditor &nodeTextEditor) {
}

juce::CaretComponent* CustomLookAndFeel::createCaretComponent(juce::Component* keyFocusOwner) {
    auto* caret = new CustomTextCaret(keyFocusOwner);
    caret->caretColour = baseDarkColour1;
    caret->caretWidth  = 1.0f;
    return caret;
}
