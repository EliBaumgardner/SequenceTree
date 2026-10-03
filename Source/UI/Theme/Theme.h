#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

struct Theme
{
    enum class FontStyle { Regular, SemiBold, Mono };

    juce::Colour dropShadowColour   = juce::Colours::black;
    juce::Colour windowColour       = juce::Colour::fromRGB(12, 14, 17);
    juce::Colour surfaceColour      = juce::Colour::fromRGB(20, 23, 28);
    juce::Colour raisedColour       = juce::Colour::fromRGB(28, 32, 39);
    juce::Colour selectedColour     = juce::Colour::fromRGB(35, 40, 49);
    juce::Colour hoverColour        = juce::Colour::fromRGB(220, 230, 255).withAlpha(0.06f);
    juce::Colour borderColour       = juce::Colour::fromRGB(220, 230, 255).withAlpha(0.07f);
    juce::Colour borderStrongColour = juce::Colour::fromRGB(220, 230, 255).withAlpha(0.14f);
    juce::Colour textColour         = juce::Colour::fromRGB(230, 233, 239);
    juce::Colour softTextColour     = juce::Colour::fromRGB(179, 186, 198);
    juce::Colour mutedTextColour    = juce::Colour::fromRGB(139, 147, 161);
    juce::Colour dimTextColour      = juce::Colour::fromRGB(75, 82, 96);
    juce::Colour accentColour       = juce::Colour::fromRGB(122, 162, 255);
    juce::Colour onAccentColour     = juce::Colour::fromRGB(11, 18, 32);
    juce::Colour accentSoftColour   = accentColour.withAlpha(0.16f);

    juce::Colour canvasColour = juce::Colour::fromRGB(16, 19, 24);

    juce::Colour gridColour        = juce::Colour::fromRGB(200, 215, 255).withAlpha(0.10f);
    juce::Colour barColour         = windowColour;
    juce::Colour captionColour     = mutedTextColour;
    juce::Colour lineNumberColour  = dimTextColour;

    juce::Colour scriptErrorColour   = juce::Colour::fromRGB(207, 102, 90);
    juce::Colour scriptOkColour      = juce::Colour::fromRGB(126, 168, 116);

    juce::Colour selectionBoxColour  = accentColour;
    juce::Colour selectionRingColour = accentColour;
    juce::Colour hoverRingColour     = softTextColour;
    juce::Colour spanOutlineColour   = softTextColour;

    juce::Colour popupMenuColour              = raisedColour;
    juce::Colour popupMenuBorderColour        = borderStrongColour;

    float textHeight = labelFontHeight;
    juce::Colour popupMenuTextColour          = textColour;
    juce::Colour popupMenuHighlightColour     = selectedColour;
    juce::Colour popupMenuHighlightTextColour = textColour;

    juce::Colour scrollBarTrackColour      = surfaceColour;
    juce::Colour scrollBarThumbColour      = borderStrongColour;
    juce::Colour scrollBarThumbHoverColour = mutedTextColour.withAlpha(0.6f);

    juce::Colour arrowColour     = juce::Colour::fromRGB(93, 102, 117);
    juce::Colour arrowHeadColour = arrowColour;

    juce::Typeface::Ptr regularTypeface;
    juce::Typeface::Ptr semiBoldTypeface;
    juce::Typeface::Ptr monoTypeface;

    juce::Font font(FontStyle style, float height) const
    {
        juce::Typeface::Ptr typeface = regularTypeface;

        switch (style) {
            case FontStyle::Regular:  typeface = regularTypeface;  break;
            case FontStyle::SemiBold: typeface = semiBoldTypeface; break;
            case FontStyle::Mono:     typeface = monoTypeface;     break;
        }

        return juce::Font(juce::FontOptions(typeface).withHeight(height));
    }

    static constexpr float arrowHeadOutlineThickness = 1.3f;
    static constexpr float arrowShaftThickness       = 1.6f;
    static constexpr float arrowShaftHoverThickness  = 2.2f;
    static constexpr float arrowTrailThickness       = 2.6f;

    static constexpr float selectionRingGap   = 1.5f;
    static constexpr float selectionRingWidth = 1.5f;

    static constexpr float highlightRingWidth   = 1.5f;
    static constexpr float highlightRingSpacing = 2.0f;
    static constexpr float highlightTintAmount  = 0.25f;
    static constexpr float hoverFillBrightness  = 0.15f;

    static constexpr float spanOutlineGap   = 2.5f;
    static constexpr float spanOutlineWidth = 1.5f;

    static constexpr int   nodeRadius    = 24;
    static constexpr float nodeCirclePad = 4.0f;

    static constexpr float nodeShadowOffsetX = 1.0f;
    static constexpr float nodeShadowOffsetY = 3.0f;
    static constexpr float nodeShadowBlur    = 5.0f;
    static constexpr float nodeShadowAlpha   = 0.45f;

    static constexpr float modulatorCornerRatio = 0.22f;

    static constexpr float encapsulatorRimInset = 2.5f;
    static constexpr float encapsulatorRimWidth = 1.0f;

    static constexpr float encapsulationRingWidth = 2.0f;

    static constexpr float borderThickness    = 1.0f;
    static constexpr float paneCornerRadius   = 7.0f;
    static constexpr float groupCornerRadius  = 10.0f;
    static constexpr float canvasCornerRadius = 14.0f;

    static constexpr float glyphGridSize    = 24.0f;
    static constexpr float glyphSizeRatio   = 0.52f;
    static constexpr float glyphStrokeRatio = 1.75f / glyphGridSize;
    static constexpr float glyphFillAlpha   = 0.28f;
    static constexpr float incrementGlyphScale = 3.0f;

    static constexpr float disabledButtonAlpha = 0.4f;
    static constexpr float buttonAspectRatio   = 34.0f / 32.0f;
    static constexpr int   buttonGap           = 2;

    static constexpr float gridDotDiameter = 2.0f;

    static constexpr float fileLabelMarkerWidth = 2.0f;
    static constexpr float selectedMarkerWidth  = 3.0f;
    static constexpr float fieldTextInset       = 6.0f;

    static constexpr float resizerGripWidth       = 4.0f;
    static constexpr float resizerGripHeightRatio = 0.05f;

    static constexpr float labelFontHeight = 9.0f;

    static constexpr int menuEdgeInset = 6;
    static constexpr int windowGutter  = 8;

    static constexpr float barHeightRatio        = 0.05f;
    static constexpr float textHeightRatio       = 0.376f;
    static constexpr float textWidthRatio        = 0.0136f;
    static constexpr float contentInsetRatio     = 0.1f;
    static constexpr float contentSpacingRatio   = 0.018f;
    static constexpr float iconGapRatio          = 0.2f;
    static constexpr float menuSpacingRatio      = 0.24f;
    static constexpr float menuRowHeightRatio    = 0.8f;
    static constexpr float menuButtonHeightRatio = 0.88f;

    static constexpr float popupMenuBorderThickness = 1.0f;
    static constexpr float popupMenuItemInset       = 2.0f;
    static constexpr float popupMenuTextInset       = 6.0f;
    static constexpr int   scrollBarThickness      = 8;
    static constexpr float scrollBarThumbInset     = 1.5f;
    static constexpr float scrollBarCornerRadius   = 3.0f;

    static constexpr int   popupMenuItemHeight      = 18;
    static constexpr int   popupMenuPadding         = 4;

    static constexpr float callOutShadowAlpha  = 0.6f;
    static constexpr int   callOutShadowRadius = 8;
};
