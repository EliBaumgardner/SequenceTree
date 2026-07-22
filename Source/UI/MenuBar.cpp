//
// Created by Eli Baumgardner on 7/20/26.
//

#include "MenuBar.h"

MenuBar::MenuBar(ApplicationContext& context): context(context)
{
    setLookAndFeel(context.lookAndFeel);

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

void MenuBar::paint(juce::Graphics &g)
{
    const Theme& theme = CustomLookAndFeel::get(*this);
    g.setColour(theme.buttonBarColour);
    g.fillRect(getLocalBounds().toFloat());
}

void MenuBar::resized()
{
    auto bounds = getLocalBounds();

    constexpr int numIcons = 3;
    int iconSize = juce::jmin(bounds.getWidth() - 12, 24);

    int gap = (bounds.getHeight() - iconSize * numIcons) / (numIcons + 1);
    int x   = (bounds.getWidth() - iconSize) / 2;
    int y   = gap;

    treeIcon->setBounds(x, y, iconSize, iconSize);
    y += iconSize + gap;

    nodeIcon->setBounds(x, y, iconSize, iconSize);
    y += iconSize + gap;

    traversalIcon->setBounds(x, y, iconSize, iconSize);
}