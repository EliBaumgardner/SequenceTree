//
// Created by Eli Baumgardner on 7/21/26.
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <map>

struct ButtonState {
    bool isHovered  = false;
    bool isDown     = false;
    bool isSelected = false;

    juce::String text;
};

struct NodeVisual {
    juce::Rectangle<float> bounds;
    juce::Colour colour;
    const std::map<int, juce::Colour>& highlights;
    bool isHovered  = false;
    bool isSelected = false;
};

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
    juce::Colour lineNumberColour    = juce::Colours::lightgrey.withAlpha(0.4f);

    juce::Colour scriptErrorColour   = juce::Colour::fromRGB(207, 102, 90);
    juce::Colour scriptOkColour      = juce::Colour::fromRGB(126, 168, 116);

    juce::Colour selectionBoxColour  = baseDarkColour2;
    juce::Colour selectionRingColour = juce::Colours::black;

    juce::Colour popupMenuColour              = baseDarkColour1.withAlpha(0.97f);
    juce::Colour popupMenuBorderColour        = juce::Colours::black.withAlpha(0.5f);
    juce::Colour popupMenuTextColour          = baseLightColour1;
    juce::Colour popupMenuHighlightColour     = baseLightColour2;
    juce::Colour popupMenuHighlightTextColour = juce::Colours::black.withAlpha(0.8f);

    juce::Colour scrollBarTrackColour      = baseDarkColour2.darker(0.5f);
    juce::Colour scrollBarThumbColour      = baseLightColour2.withAlpha(0.5f);
    juce::Colour scrollBarThumbHoverColour = baseLightColour2.withAlpha(0.85f);

    juce::Colour arrowColour         = juce::Colours::black;
    juce::Colour arrowProgressColour = baseLightColour2;
    juce::Colour arrowHeadColour     = juce::Colours::black;

    static constexpr float arrowHeadOutlineThickness = 0.75f;

    static constexpr float selectionRingGap   = 1.5f;
    static constexpr float selectionRingWidth = 1.25f;
    static constexpr float selectionRimWidth  = 3.0f;

    static constexpr float nodeCirclePad = 4.0f;

    static constexpr float incrementIconWidthInset  = 0.30f;
    static constexpr float incrementIconHeightInset = 0.12f;

    static constexpr float paneCornerRadius = 4.0f;

    static constexpr float fileLabelMarkerWidth = 2.0f;

    static constexpr float innerButtonBoundsReduction = 5.0f;
    static constexpr float outerButtonBoundsReduction = 2.0f;

    static constexpr float labelFontHeight = 9.0f;

    static constexpr int textButtonHeight = 22;
    static constexpr int menuEdgeInset    = 6;

    static constexpr float popupMenuBorderThickness = 1.0f;
    static constexpr float popupMenuItemInset       = 2.0f;
    static constexpr float popupMenuTextInset       = 6.0f;
    static constexpr int   scrollBarThickness      = 8;
    static constexpr float scrollBarThumbInset     = 1.5f;
    static constexpr float scrollBarCornerRadius   = 3.0f;

    static constexpr int   popupMenuItemHeight      = 18;
    static constexpr int   popupMenuPadding         = 4;
};
