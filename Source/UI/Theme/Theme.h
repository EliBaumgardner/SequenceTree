//
// Created by Eli Baumgardner on 7/21/26.
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

struct Theme
{
    juce::Colour getButtonColour() const { return buttonColour; }
    juce::Colour getTextColour()   const { return textColour; }
    juce::Colour getBarColour()    const { return barColour; }

    juce::Colour dropShadowColour     = juce::Colours::black;
    juce::Colour baseDarkColour1      = juce::Colour::fromRGB(40,40,38);
    juce::Colour baseDarkColour2      = juce::Colour::fromRGB(30,30,30);
    juce::Colour baseLightColour1     = juce::Colour::fromRGB(195,174,132);
    juce::Colour baseLightColour2     = juce::Colour::fromRGB(162,150,131);
    juce::Colour baseLightColour3     = juce::Colour::fromRGB(217,217,217);
    juce::Colour darkBrownColour      = juce::Colour::fromRGB(48, 32, 22);

    juce::Colour canvasColour = baseLightColour1.darker();

    juce::Colour gridColour          = juce::Colour::fromRGB(15, 15, 15);
    juce::Colour barColour           = baseDarkColour2;
    juce::Colour buttonColour        = baseLightColour2;
    juce::Colour buttonBarColour     = baseDarkColour1;
    juce::Colour editorColour        = baseDarkColour1;
    juce::Colour traversalMenuColour = darkBrownColour;
    juce::Colour textColour          = baseLightColour1;

    juce::Colour arrowColour         = juce::Colours::black;
    juce::Colour arrowProgressColour = baseLightColour2;
    juce::Colour arrowHeadColour     = juce::Colours::black;

    static constexpr float nodeCirclePad = 2.0f;

    static constexpr float paneCornerRadius = 4.0f;

    static constexpr float innerButtonBoundsReduction = 5.0f;
    static constexpr float outerButtonBoundsReduction = 2.0f;

    static constexpr float labelFontHeight = 9.0f;

    static constexpr int textButtonHeight = 22;
    static constexpr int menuEdgeInset    = 6;
};
