//
// Created by Eli Baumgardner on 7/20/26.
//

#include "MenuBar.h"

MenuBar::MenuBar(ApplicationContext& context)
    : Bar(context, { Orientation::vertical, Background::flat, iconInset })
{
    treeIcon = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawTreeIcon(g, bounds, state);
        }, context.lookAndFeel);

    nodeIcon = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawNodeIcon(g, bounds, state);
        }, context.lookAndFeel);

    traversalIcon = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawTraversalIcon(g, bounds, state);
        }, context.lookAndFeel);

    addAndMakeVisible(treeIcon.get());
    addAndMakeVisible(nodeIcon.get());
    addAndMakeVisible(traversalIcon.get());
}

void MenuBar::resized()
{
    const auto bounds = getContentBounds();

    constexpr int numIcons = 3;

    const int iconSize = juce::jmin(bounds.getWidth(), maxIconSize);
    const int gap      = (bounds.getHeight() - iconSize * numIcons) / (numIcons + 1);
    const int x        = bounds.getX() + (bounds.getWidth() - iconSize) / 2;

    int y = bounds.getY() + gap;

    treeIcon->setBounds(x, y, iconSize, iconSize);
    y += iconSize + gap;

    nodeIcon->setBounds(x, y, iconSize, iconSize);
    y += iconSize + gap;

    traversalIcon->setBounds(x, y, iconSize, iconSize);
}
