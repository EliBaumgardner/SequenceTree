#include "CustomLookAndFeel.h"

CustomLookAndFeel::CustomLookAndFeel()
{
    setColour(juce::PopupMenu::backgroundColourId,            popupMenuColour);
    setColour(juce::PopupMenu::textColourId,                  popupMenuTextColour);
    setColour(juce::PopupMenu::headerTextColourId,            popupMenuTextColour);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, popupMenuHighlightColour);
    setColour(juce::PopupMenu::highlightedTextColourId,       popupMenuHighlightTextColour);
    setColour(juce::CaretComponent::caretColourId,            baseDarkColour1);
}

void CustomLookAndFeel::drawPopupMenuBackgroundWithOptions(juce::Graphics& graphics, int width, int height,
                                                           const juce::PopupMenu::Options&)
{
    const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height))
                            .reduced(popupMenuBorderThickness * 0.5f);

    graphics.setColour(popupMenuColour);
    graphics.fillRoundedRectangle(bounds, paneCornerRadius);

    graphics.setColour(popupMenuBorderColour);
    graphics.drawRoundedRectangle(bounds, paneCornerRadius, popupMenuBorderThickness);
}

void CustomLookAndFeel::drawPopupMenuItem(juce::Graphics& graphics, const juce::Rectangle<int>& area,
                                          bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                                          bool hasSubMenu, const juce::String& text,
                                          const juce::String& shortcutKeyText,
                                          const juce::Drawable* icon, const juce::Colour* textColourToUse)
{
    if (isSeparator) {
        auto line = area.toFloat().reduced(menuEdgeInset, 0.0f).withHeight(1.0f);
        line.setY(area.toFloat().getCentreY());

        graphics.setColour(popupMenuTextColour.withAlpha(0.25f));
        graphics.fillRect(line);
        return;
    }

    const auto itemBounds = area.toFloat().reduced(popupMenuItemInset, 1.0f);

    juce::Colour itemTextColour = popupMenuTextColour;

    if (textColourToUse != nullptr) {
        itemTextColour = *textColourToUse;
    }

    if (isHighlighted && isActive) {
        graphics.setColour(popupMenuHighlightColour);
        graphics.fillRoundedRectangle(itemBounds, paneCornerRadius);

        itemTextColour = popupMenuHighlightTextColour;
    }

    float textAlpha = 0.4f;

    if (isActive) {
        textAlpha = 1.0f;
    }

    graphics.setColour(itemTextColour.withMultipliedAlpha(textAlpha));
    graphics.setFont(getPopupMenuFont());

    auto textBounds = itemBounds.reduced(popupMenuTextInset, 0.0f);

    const auto gutter = textBounds.removeFromLeft(textBounds.getHeight());

    if (icon != nullptr) {
        icon->drawWithin(graphics, gutter, juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
    }
    else if (isTicked) {
        const juce::Path tick = getTickShape(1.0f);
        graphics.fillPath(tick, tick.getTransformToScaleToFit(gutter.reduced(gutter.getWidth() / 4.0f), true));
    }

    if (hasSubMenu) {
        const float arrowHeight = 0.6f * getPopupMenuFont().getAscent();
        const float arrowX      = textBounds.removeFromRight(arrowHeight).getX();
        const float centreY     = textBounds.getCentreY();
        juce::Path  arrow;

        arrow.startNewSubPath(arrowX, centreY - arrowHeight * 0.5f);
        arrow.lineTo(arrowX + arrowHeight * 0.6f, centreY);
        arrow.lineTo(arrowX, centreY + arrowHeight * 0.5f);

        graphics.strokePath(arrow, juce::PathStrokeType(1.5f));
    }

    graphics.drawFittedText(text, textBounds.toNearestInt(), juce::Justification::centredLeft, 1);

    if (shortcutKeyText.isNotEmpty()) {
        graphics.setFont(juce::Font(juce::FontOptions(labelFontHeight * 0.85f)));
        graphics.drawText(shortcutKeyText, textBounds, juce::Justification::centredRight, true);
    }
}

juce::Font CustomLookAndFeel::getPopupMenuFont()
{
    return juce::Font(juce::FontOptions(labelFontHeight));
}

void CustomLookAndFeel::getIdealPopupMenuItemSize(const juce::String& text, bool isSeparator,
                                                  int standardMenuItemHeight,
                                                  int& idealWidth, int& idealHeight)
{
    int itemHeight = popupMenuItemHeight;

    if (standardMenuItemHeight > 0) {
        itemHeight = standardMenuItemHeight;
    }

    if (isSeparator) {
        idealWidth  = 50;
        idealHeight = itemHeight / 2;
        return;
    }

    idealHeight = itemHeight;
    idealWidth  = juce::GlyphArrangement::getStringWidthInt(getPopupMenuFont(), text)
                    + idealHeight
                    + static_cast<int>(popupMenuTextInset * 4.0f);
}

int CustomLookAndFeel::getPopupMenuBorderSize()
{
    return popupMenuPadding;
}

void CustomLookAndFeel::drawCallOutBoxBackground(juce::CallOutBox& box, juce::Graphics& graphics, const juce::Path& path,
                                                 juce::Image& cachedShadow)
{
    if (cachedShadow.isNull()) {
        cachedShadow = juce::Image(juce::Image::ARGB, box.getWidth(), box.getHeight(), true);

        juce::Graphics shadowGraphics(cachedShadow);

        juce::DropShadow(dropShadowColour.withAlpha(callOutShadowAlpha), callOutShadowRadius, {}).drawForPath(shadowGraphics, path);
    }

    graphics.setColour(dropShadowColour);
    graphics.drawImageAt(cachedShadow, 0, 0);

    graphics.setColour(popupMenuColour);
    graphics.fillPath(path);

    graphics.setColour(popupMenuBorderColour);
    graphics.strokePath(path, juce::PathStrokeType(popupMenuBorderThickness));
}
