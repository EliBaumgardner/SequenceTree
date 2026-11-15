#include "MenuBar.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../Canvas/NodeCanvas.h"

MenuBar::MenuBar(NodeCanvas& nodeCanvas)
    : ArsNovaUI::Bar(nodeCanvas, Orientation::Vertical, iconInsetRatio)
{
    treeIcon.icon      = &ArsNovaUI::CustomLookAndFeel::drawForkIcon;
    settingsIcon.icon  = &ArsNovaUI::CustomLookAndFeel::drawSettingsIcon;
    nodeIcon.icon      = &ArsNovaUI::CustomLookAndFeel::drawRingDotIcon;
    traversalIcon.icon = &ArsNovaUI::CustomLookAndFeel::drawRouteIcon;

    treeIcon.setTooltip("Tree");
    settingsIcon.setTooltip("Settings");
    nodeIcon.setTooltip("Node settings");
    traversalIcon.setTooltip("Traversal settings");

    treeIcon.setEnabled(false);

    addChildComponent(treeIcon);

    addAndMakeVisible(settingsIcon);
    addAndMakeVisible(nodeIcon);
    addAndMakeVisible(traversalIcon);
}

void MenuBar::paintOverBar(juce::Graphics& graphics)
{
    ArsNovaUI::CustomLookAndFeel& lookAndFeel = ArsNovaUI::CustomLookAndFeel::get(*this);

    lookAndFeel.drawPane(graphics, settingsIcon.getBounds().toFloat());

    graphics.setColour(lookAndFeel.accentColour);

    for (const ArsNovaUI::IconButton* icon : { &nodeIcon, &traversalIcon }) {
        if (icon->state.isSelected) {
            const auto iconBounds = icon->getBounds().toFloat();

            const float markerInset  = iconBounds.getHeight() * markerInsetRatio;
            const float markerRadius = ArsNovaUI::Theme::selectedMarkerWidth * 0.5f;

            const auto marker = juce::Rectangle<float>(0.0f, iconBounds.getY(), ArsNovaUI::Theme::selectedMarkerWidth, iconBounds.getHeight())
                                    .reduced(0.0f, markerInset);

            graphics.fillRoundedRectangle(marker, markerRadius);
        }
    }
}

void MenuBar::resized()
{
    const auto bounds = getContentBounds();

    const int iconWidth  = bounds.getWidth();
    const int iconHeight = juce::roundToInt(iconWidth * ArsNovaUI::Theme::buttonAspectRatio);
    const int iconGap    = juce::roundToInt(iconWidth * ArsNovaUI::Theme::iconGapRatio);
    const int groupGap   = juce::roundToInt(iconWidth * groupGapRatio);
    int       iconY      = bounds.getY();

    treeIcon.setBounds(bounds.getX(), iconY, iconWidth, iconHeight);
    settingsIcon.setBounds(bounds.getX(), iconY, iconWidth, iconHeight);

    iconY += iconHeight + groupGap;

    nodeIcon.setBounds(bounds.getX(), iconY, iconWidth, iconHeight);

    iconY += iconHeight + iconGap;

    traversalIcon.setBounds(bounds.getX(), iconY, iconWidth, iconHeight);
}
