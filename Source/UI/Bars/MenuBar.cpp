#include "MenuBar.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../Canvas/NodeCanvas.h"

MenuBar::MenuBar(NodeCanvas& nodeCanvas)
    : ArsNovaUI::Bar(nodeCanvas, Orientation::Vertical, iconInsetRatio)
{
    treeIcon.icon      = &ArsNovaUI::CustomLookAndFeel::drawForkIcon;
    nodeIcon.icon      = &ArsNovaUI::CustomLookAndFeel::drawRingDotIcon;
    traversalIcon.icon = &ArsNovaUI::CustomLookAndFeel::drawRouteIcon;

    treeIcon.setEnabled(false);

    addAndMakeVisible(treeIcon);
    addAndMakeVisible(nodeIcon);
    addAndMakeVisible(traversalIcon);
}

void MenuBar::paintOverBar(juce::Graphics& graphics)
{
    const ArsNovaUI::Theme& theme = ArsNovaUI::CustomLookAndFeel::get(*this);

    graphics.setColour(theme.accentColour);

    for (const ArsNovaUI::IconButton* icon : { &nodeIcon, &traversalIcon }) {
        if (icon->state.isSelected) {
            const auto iconBounds = icon->getBounds().toFloat();
            const auto marker     = juce::Rectangle<float>(0.0f, iconBounds.getY(), ArsNovaUI::Theme::selectedMarkerWidth, iconBounds.getHeight())
                                        .reduced(0.0f, iconBounds.getHeight() * 0.22f);

            graphics.fillRoundedRectangle(marker, ArsNovaUI::Theme::selectedMarkerWidth * 0.5f);
        }
    }
}

void MenuBar::resized()
{
    const auto bounds   = getContentBounds();
    const int  iconSize = bounds.getWidth();
    const int  iconGap  = juce::roundToInt(iconSize * ArsNovaUI::Theme::iconGapRatio);
    int        iconY    = bounds.getY();

    treeIcon.setBounds(bounds.getX(), iconY, iconSize, iconSize);

    iconY += iconSize + iconGap;

    nodeIcon.setBounds(bounds.getX(), iconY, iconSize, iconSize);

    iconY += iconSize + iconGap;

    traversalIcon.setBounds(bounds.getX(), iconY, iconSize, iconSize);
}
