#include "MenuBar.h"
#include "../Theme/CustomLookAndFeel.h"

MenuBar::MenuBar(const ApplicationContext& context)
    : Bar(context, { Orientation::Vertical, iconInsetRatio })
{
    treeIcon.icon      = &CustomLookAndFeel::drawTreeIcon;
    nodeIcon.icon      = &CustomLookAndFeel::drawNodeIcon;
    traversalIcon.icon = &CustomLookAndFeel::drawTraversalIcon;

    treeIcon     .setLookAndFeel(context.lookAndFeel);
    nodeIcon     .setLookAndFeel(context.lookAndFeel);
    traversalIcon.setLookAndFeel(context.lookAndFeel);

    addAndMakeVisible(treeIcon);
    addAndMakeVisible(nodeIcon);
    addAndMakeVisible(traversalIcon);
}

void MenuBar::resized()
{
    const auto  bounds         = getContentBounds();
    const float heightPerIcon  = bounds.getHeight() / (iconCount + (iconCount + 1) * Theme::iconGapRatio);
    const int   iconSize       = juce::jmax(0, juce::jmin(bounds.getWidth(), static_cast<int>(heightPerIcon)));
    const int   iconGap        = (bounds.getHeight() - iconSize * iconCount) / (iconCount + 1);
    const int   iconX          = bounds.getX() + (bounds.getWidth() - iconSize) / 2;
    int         iconY          = bounds.getY() + iconGap;

    treeIcon.setBounds(iconX, iconY, iconSize, iconSize);

    iconY += iconSize + iconGap;

    nodeIcon.setBounds(iconX, iconY, iconSize, iconSize);

    iconY += iconSize + iconGap;

    traversalIcon.setBounds(iconX, iconY, iconSize, iconSize);
}
