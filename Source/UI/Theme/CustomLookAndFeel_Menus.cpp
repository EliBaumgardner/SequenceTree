//
// Created by Eli Baumgardner on 7/31/26.
//

#include "CustomLookAndFeel.h"

CustomLookAndFeel::CustomLookAndFeel()
{
    setColour(juce::PopupMenu::backgroundColourId,            popupMenuColour);
    setColour(juce::PopupMenu::textColourId,                  popupMenuTextColour);
    setColour(juce::PopupMenu::headerTextColourId,            popupMenuTextColour);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, popupMenuHighlightColour);
    setColour(juce::PopupMenu::highlightedTextColourId,       popupMenuHighlightTextColour);
}

juce::Font CustomLookAndFeel::getPopupMenuFont()
{
    return juce::Font(juce::FontOptions(labelFontHeight));
}

int CustomLookAndFeel::getPopupMenuBorderSize()
{
    return popupMenuPadding;
}

void CustomLookAndFeel::drawPopupMenuBackgroundWithOptions(juce::Graphics& g, int width, int height,
                                                           const juce::PopupMenu::Options&)
{
    const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, (float) width, (float) height)
                            .reduced(popupMenuBorderThickness * 0.5f);

    g.setColour(popupMenuColour);
    g.fillRoundedRectangle(bounds, paneCornerRadius);

    g.setColour(popupMenuBorderColour);
    g.drawRoundedRectangle(bounds, paneCornerRadius, popupMenuBorderThickness);
}

void CustomLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                          bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                                          bool hasSubMenu, const juce::String& text,
                                          const juce::String& shortcutKeyText,
                                          const juce::Drawable* icon, const juce::Colour* textColourToUse)
{
    if (isSeparator) {
        auto line = area.toFloat().reduced(menuEdgeInset, 0.0f).withHeight(1.0f);
        line.setY(area.toFloat().getCentreY());

        g.setColour(popupMenuTextColour.withAlpha(0.25f));
        g.fillRect(line);
        return;
    }

    const auto itemBounds = area.toFloat().reduced(popupMenuItemInset, 1.0f);

    juce::Colour itemTextColour = textColourToUse == nullptr ? popupMenuTextColour : *textColourToUse;

    if (isHighlighted && isActive) {
        g.setColour(popupMenuHighlightColour);
        g.fillRoundedRectangle(itemBounds, paneCornerRadius);

        itemTextColour = popupMenuHighlightTextColour;
    }

    g.setColour(itemTextColour.withMultipliedAlpha(isActive ? 1.0f : 0.4f));
    g.setFont(getPopupMenuFont());

    auto textBounds = itemBounds.reduced(popupMenuTextInset, 0.0f);

    const auto gutter = textBounds.removeFromLeft(textBounds.getHeight());

    if (icon != nullptr) {
        icon->drawWithin(g, gutter, juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
    }
    else if (isTicked) {
        const juce::Path tick = getTickShape(1.0f);
        g.fillPath(tick, tick.getTransformToScaleToFit(gutter.reduced(gutter.getWidth() / 4.0f), true));
    }

    if (hasSubMenu) {
        const float arrowHeight = 0.6f * getPopupMenuFont().getAscent();
        const float x           = textBounds.removeFromRight(arrowHeight).getX();
        const float centreY     = textBounds.getCentreY();

        juce::Path arrow;
        arrow.startNewSubPath(x, centreY - arrowHeight * 0.5f);
        arrow.lineTo(x + arrowHeight * 0.6f, centreY);
        arrow.lineTo(x, centreY + arrowHeight * 0.5f);

        g.strokePath(arrow, juce::PathStrokeType(1.5f));
    }

    g.drawFittedText(text, textBounds.toNearestInt(), juce::Justification::centredLeft, 1);

    if (shortcutKeyText.isNotEmpty()) {
        g.setFont(juce::Font(juce::FontOptions(labelFontHeight * 0.85f)));
        g.drawText(shortcutKeyText, textBounds, juce::Justification::centredRight, true);
    }
}

void CustomLookAndFeel::getIdealPopupMenuItemSize(const juce::String& text, bool isSeparator,
                                                  int standardMenuItemHeight,
                                                  int& idealWidth, int& idealHeight)
{
    if (isSeparator) {
        idealWidth  = 50;
        idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight / 2 : popupMenuItemHeight / 2;
        return;
    }

    idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight : popupMenuItemHeight;
    idealWidth  = juce::GlyphArrangement::getStringWidthInt(getPopupMenuFont(), text)
                    + idealHeight
                    + (int) (popupMenuTextInset * 4.0f);
}
