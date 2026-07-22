/*
  ==============================================================================

    TitleBar.cpp
    Created: 12 Jun 2025 6:50:43pm
    Author:  Eli Baumgardner

  ==============================================================================
*/


#include "Canvas/NodeCanvas.h"
#include "Theme/CustomLookAndFeel.h"
#include "../Plugin/PluginProcessor.h"

#include "Titlebar.h"


Titlebar::Titlebar(ApplicationContext& context)
    : applicationContext(context),
      buttonPane(context),
      displaySelector(context),
      tempoDisplay(context),
      undoRedoPane(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    playButton = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawPlayIcon(g, bounds, state);
        }, context.lookAndFeel);

    resetButton = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawResetIcon(g, bounds, state);
        }, context.lookAndFeel);

    playButton->setTooltip("Play / Pause");
    playButton->setSelected(true);
    resetButton->setTooltip("Reset");

    addAndMakeVisible(playButton.get());
    addAndMakeVisible(resetButton.get());
    addAndMakeVisible(tempoDisplay);
    addAndMakeVisible(buttonPane);
    addAndMakeVisible(displaySelector);
    addAndMakeVisible(undoRedoPane);

    configureModePane();
    configureUndoRedoPane();

    configureDisplaySelector();

    playButton->onClick = [this]() {
        NodeCanvas& canvas = *applicationContext.canvas;
        jassert(&canvas);

        playButton->setSelected(!playButton->isSelected());

        canvas.start = !canvas.start;
        canvas.setProcessorPlayblack(canvas.start);
    };

    resetButton->onClick = [this]() {
        applicationContext.processor->resetRequested.store(true);
        if (auto* canvas = applicationContext.canvas) {
            canvas->arrowManager.resetAllProgress();
        }
    };

    auto applyMultiplier = [this]() {
        double value = tempoDisplay.editor.getText().getDoubleValue();
        if (value > 0.0) {
            applicationContext.processor->tempoMultiplier.store(value);
        }
        else {
            tempoDisplay.editor.setText(juce::String(applicationContext.processor->tempoMultiplier.load()), false);
        }
    };

    tempoDisplay.editor.onReturnKey = applyMultiplier;
    tempoDisplay.editor.onFocusLost = applyMultiplier;
}

void Titlebar::configureDisplaySelector()
{
    auto addDisplayMode = [this](int itemId, juce::String label, NodeDisplayMode mode) {
        displaySelector.addItem(itemId, std::move(label), [this, mode]() {
            applicationContext.canvas->nodeManager.setDisplayMode(mode);
            applicationContext.currentDisplayMode = mode;

            if (applicationContext.onDisplayModeChanged) {
                applicationContext.onDisplayModeChanged(mode);
            }
        });
    };

    addDisplayMode(1, "show pitch",       NodeDisplayMode::Pitch);
    addDisplayMode(2, "show velocity",    NodeDisplayMode::Velocity);
    addDisplayMode(3, "show countLimit",  NodeDisplayMode::CountLimit);
    addDisplayMode(4, "show channel",     NodeDisplayMode::Channel);
    addDisplayMode(5, "show repeatValue", NodeDisplayMode::RepeatValue);

    displaySelector.setSelectedItem(1);
}

void Titlebar::configureModePane()
{
    buttonPane.enableToggleSelection();

    IconButton& nodeButton = buttonPane.addButton(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawNodeModeIcon(g, bounds, state);
        },
        "Node Mode",
        [this]() { setControllerMode(NodeController::NodeControllerMode::Node); });

    buttonPane.addButton(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawModulatorIcon(g, bounds, state);
        },
        "Modulator Mode",
        [this]() { setControllerMode(NodeController::NodeControllerMode::Modulator); });

    buttonPane.addButton(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawTraversalFlagIcon(g, bounds, state);
        },
        "Traversal Flag Mode",
        [this]() { setControllerMode(NodeController::NodeControllerMode::TraversalFlag); });

    buttonPane.setSelectedButton(&nodeButton);
}

void Titlebar::configureUndoRedoPane()
{
    undoRedoPane.addButton(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawUndoIcon(g, bounds, state);
        },
        "Undo",
        [this]() { applicationContext.undoManager->undo(); });

    undoRedoPane.addButton(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawRedoIcon(g, bounds, state);
        },
        "Redo",
        [this]() { applicationContext.undoManager->redo(); });
}

void Titlebar::setControllerMode(NodeController::NodeControllerMode mode)
{
    applicationContext.nodeController->nodeControllerMode = mode;
}

void Titlebar::paint(juce::Graphics& g)
{
    const Theme& theme = CustomLookAndFeel::get(*this);
    auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient gradient(theme.barColour.brighter(0.06f), 0, bounds.getY(),
                                  theme.barColour.darker(0.04f),   0, bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRect(bounds);

    g.setColour(theme.barColour.brighter(0.12f));
    g.drawHorizontalLine((int) bounds.getY(), bounds.getX(), bounds.getRight());

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawHorizontalLine((int) bounds.getBottom() - 1, bounds.getX(), bounds.getRight());

    auto& laf = CustomLookAndFeel::get(*this);
    g.setColour(laf.getTextColour().withAlpha(0.12f));
    float inset = getHeight() * 0.22f;

    auto drawSep = [&](int x) {
        g.drawVerticalLine(x, inset, getHeight() - inset);
    };

    drawSep((resetButton->getRight() + tempoDisplay.getX()) / 2);
    drawSep((tempoDisplay.getRight() + undoRedoPane.getX()) / 2);
    drawSep((buttonPane.getRight() + displaySelector.getX()) / 2);
}

void Titlebar::resized()
{
    auto bounds = getLocalBounds().reduced(4.0f);
    int spacing = 12;

    int buttonSize = std::min(bounds.getHeight(), bounds.getWidth() / 25);
    int tempoDisplayWidth = bounds.getWidth() / 8;
    int buttonPaneWidth = bounds.getWidth() / 8;
    int displaySelectorWidth = bounds.getWidth() / 8;
    int undoRedoPaneWidth = bounds.getWidth() / 8;

    auto area = bounds;

    playButton->setBounds(area.removeFromLeft(buttonSize));
    area.removeFromLeft(spacing);

    resetButton->setBounds(area.removeFromLeft(buttonSize));
    area.removeFromLeft(spacing);

    tempoDisplay.setBounds(area.removeFromLeft(tempoDisplayWidth));
    area.removeFromLeft(spacing);

    undoRedoPane.setBounds(area.removeFromLeft(undoRedoPaneWidth));
    area.removeFromLeft(spacing);

    displaySelector.setBounds(area.removeFromRight(displaySelectorWidth));
    area.removeFromRight(spacing);

    buttonPane.setBounds(area.removeFromRight(buttonPaneWidth));
}
