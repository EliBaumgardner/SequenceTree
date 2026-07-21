//
// Created by Eli Baumgardner on 7/21/26.
//

#include "TraversalRulesMenu.h"
#include "Theme/CustomLookAndFeel.h"

TraversalRulesMenu::TraversalRulesMenu(ApplicationContext& context)
    : applicationContext(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);
}

TraversalRulesMenu::~TraversalRulesMenu() {
    setLookAndFeel(nullptr);
}

void TraversalRulesMenu::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour::fromRGB(30, 30, 30));

    g.setColour(juce::Colours::black);
    g.drawRect(getLocalBounds(), 1);
}

void TraversalRulesMenu::resized() {
}

TraversalRulesWindow::TraversalRulesWindow(ApplicationContext& context)
    : juce::DocumentWindow("Traversal Rules", juce::Colour::fromRGB(30, 30, 30),
                           juce::DocumentWindow::closeButton, true)
{
    auto* content = new TraversalRulesMenu(context);
    content->setSize(TraversalRulesMenu::defaultWidth, TraversalRulesMenu::defaultHeight);

    setContentOwned(content, true);
    setResizable(true, true);
}

void TraversalRulesWindow::closeButtonPressed() {
    setVisible(false);
}
