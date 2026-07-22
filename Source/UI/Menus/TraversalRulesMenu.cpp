//
// Created by Eli Baumgardner on 7/21/26.
//

#include "TraversalRulesMenu.h"
#include "../Theme/CustomLookAndFeel.h"

TraversalRulesMenu::TraversalRulesMenu(ApplicationContext& context)
    : ResizablePanel(context, ResizeEdge::Right, resizerWidth)
{
    addAndMakeVisible(panel);
}

void TraversalRulesMenu::paint(juce::Graphics& g) {
    ResizablePanel::paint(g);

    g.setColour(juce::Colours::black);
    g.drawRect(getLocalBounds(), 1);
}

void TraversalRulesMenu::resized() {
    panelWidth = clampPanelWidth(panelWidth);

    auto bounds = getLocalBounds();

    panel.setBounds(bounds.removeFromLeft(panelWidth));
    resizer.setBounds(bounds.removeFromLeft(resizerWidth));
}

int TraversalRulesMenu::clampPanelWidth(int newWidth) const {
    const int available = getWidth() - resizerWidth - minContentWidth;
    const int maxWidth  = juce::jmax(minPanelWidth, available);

    return juce::jlimit(minPanelWidth, maxWidth, newWidth);
}

void TraversalRulesMenu::setPanelWidth(int newWidth) {
    const int clamped = clampPanelWidth(newWidth);

    if (clamped == panelWidth) {
        return;
    }

    panelWidth = clamped;
    resized();
}

void TraversalRulesMenu::Panel::paint(juce::Graphics& g) {
    const Theme& theme = CustomLookAndFeel::get(*this);

    g.setColour(theme.baseDarkColour1);
    g.fillRect(getLocalBounds());
}
