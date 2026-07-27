//
// Created by Eli Baumgardner on 7/21/26.
//

#include "TraversalRulesWindow.h"
#include "../Theme/CustomLookAndFeel.h"

TraversalRulesWindow::TraversalRulesWindow(ApplicationContext& context)
    : rulesPanel(context)
{
    setLookAndFeel(context.lookAndFeel);

    rulesPanel.onWidthDragged = [this](int newWidth) { setPanelWidth(newWidth); };

    addAndMakeVisible(rulesPanel);
}

TraversalRulesWindow::~TraversalRulesWindow() {
    setLookAndFeel(nullptr);
}

void TraversalRulesWindow::paint(juce::Graphics& g) {
    const Theme& theme = CustomLookAndFeel::get(*this);

    g.setColour(theme.baseDarkColour2);
    g.fillRect(getLocalBounds());

    g.setColour(juce::Colours::black);
    g.drawRect(getLocalBounds(), 1);
}

void TraversalRulesWindow::resized() {
    panelWidth = clampPanelWidth(panelWidth);

    rulesPanel.setBounds(getLocalBounds().removeFromLeft(panelWidth));
}

int TraversalRulesWindow::clampPanelWidth(int newWidth) const {
    const int available = getWidth() - minContentWidth;
    const int maxWidth  = juce::jmax(RulesPanel::minPanelWidth, available);

    return juce::jlimit(RulesPanel::minPanelWidth, maxWidth, newWidth);
}

void TraversalRulesWindow::setPanelWidth(int newWidth) {
    const int clamped = clampPanelWidth(newWidth);

    if (clamped == panelWidth) {
        return;
    }

    panelWidth = clamped;
    resized();
}

TraversalRulesWindow::RulesPanel::RulesPanel(ApplicationContext& context)
    : ResizablePanel(context, ResizeEdge::Right, resizerWidth)
{
}

void TraversalRulesWindow::RulesPanel::paint(juce::Graphics& g) {
    const Theme& theme = CustomLookAndFeel::get(*this);

    g.setColour(theme.baseDarkColour1);
    g.fillRect(getLocalBounds());
}

void TraversalRulesWindow::RulesPanel::resized() {
    resizer.setBounds(getLocalBounds().removeFromRight(resizerWidth));
}
