//
// Created by Eli Baumgardner on 7/21/26.
//

#include "TraversalRulesWindow.h"
#include "../Theme/CustomLookAndFeel.h"

TraversalRulesWindow::TraversalRulesWindow(ApplicationContext& context)
    : titlebar(context), rulesPanel(context)
{
    setLookAndFeel(context.lookAndFeel);

    rulesPanel.onWidthDragged = [this](int newWidth) { setPanelWidth(newWidth); };

    addAndMakeVisible(titlebar);
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

    auto bounds = getLocalBounds();

    titlebar.setBounds(bounds.removeFromTop(RulesTitlebar::preferredHeight));
    rulesPanel.setBounds(bounds.removeFromLeft(panelWidth));
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

TraversalRulesWindow::RulesTitlebar::RulesTitlebar(ApplicationContext& context)
    : Bar(context, { Orientation::horizontal, Background::litFromTop }),
      undoRedoPane(context)
{
    playButton = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            ButtonState triangleState = state;
            triangleState.isSelected = true;

            CustomLookAndFeel::get(*this).drawPlayIcon(g, bounds, triangleState);
        }, context.lookAndFeel);

    playButton->setTooltip("Run Rules");

    addAndMakeVisible(playButton.get());
    addAndMakeVisible(undoRedoPane);

    configureUndoRedoPane();
}

void TraversalRulesWindow::RulesTitlebar::configureUndoRedoPane() {
    undoRedoPane.addButton(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawUndoIcon(g, bounds, state);
        },
        "Undo");

    undoRedoPane.addButton(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawRedoIcon(g, bounds, state);
        },
        "Redo");
}

void TraversalRulesWindow::RulesTitlebar::paintOverBar(juce::Graphics& g) {
    drawSeparator(g, (playButton->getRight() + undoRedoPane.getX()) / 2);
}

void TraversalRulesWindow::RulesTitlebar::resized() {
    auto bounds = getContentBounds();

    const int buttonSize = bounds.getHeight();

    playButton->setBounds(bounds.removeFromLeft(buttonSize));
    bounds.removeFromLeft(contentSpacing);

    undoRedoPane.setBounds(bounds.removeFromLeft(buttonSize * 3));
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
