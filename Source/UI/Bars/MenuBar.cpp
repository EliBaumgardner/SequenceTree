#include "MenuBar.h"
#include "../Theme/CustomLookAndFeel.h"

MenuBar::MenuBar(NodeCanvas& nodeCanvas)
    : Bar(nodeCanvas, Orientation::Vertical, iconInsetRatio)
{
    treeIcon.icon      = &CustomLookAndFeel::drawTreeIcon;
    nodeIcon.icon      = &CustomLookAndFeel::drawNodeIcon;
    traversalIcon.icon = &CustomLookAndFeel::drawTraversalIcon;

    treeIcon.setEnabled(false);

    addAndMakeVisible(treeIcon);
    addAndMakeVisible(nodeIcon);
    addAndMakeVisible(traversalIcon);
}

void MenuBar::paintOverBar(juce::Graphics& graphics)
{
    const Theme& theme = CustomLookAndFeel::get(*this);

    graphics.setColour(theme.accentColour);

    for (const IconButton* icon : { &nodeIcon, &traversalIcon }) {
        if (icon->state.isSelected) {
            const auto iconBounds = icon->getBounds().toFloat();
            const auto marker     = juce::Rectangle<float>(0.0f, iconBounds.getY(), Theme::selectedMarkerWidth, iconBounds.getHeight())
                                        .reduced(0.0f, iconBounds.getHeight() * 0.22f);

            graphics.fillRoundedRectangle(marker, Theme::selectedMarkerWidth * 0.5f);
        }
    }
}

void MenuBar::resized()
{
    const auto bounds   = getContentBounds();
    const int  iconSize = bounds.getWidth();
    const int  iconGap  = juce::roundToInt(iconSize * Theme::iconGapRatio);
    int        iconY    = bounds.getY();

    treeIcon.setBounds(bounds.getX(), iconY, iconSize, iconSize);

    iconY += iconSize + iconGap;

    nodeIcon.setBounds(bounds.getX(), iconY, iconSize, iconSize);

    iconY += iconSize + iconGap;

    traversalIcon.setBounds(bounds.getX(), iconY, iconSize, iconSize);
}
