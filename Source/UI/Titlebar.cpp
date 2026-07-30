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
    : Bar(context, { Orientation::horizontal, Background::litFromTop }),
      transportPane(context),
      buttonPane(context),
      displaySelector(context),
      tempoDisplay(context),
      undoRedoPane(context)
{
    addAndMakeVisible(transportPane);
    addAndMakeVisible(tempoDisplay);
    addAndMakeVisible(buttonPane);
    addAndMakeVisible(displaySelector);
    addAndMakeVisible(undoRedoPane);

    configureTransportPane();
    configureModePane();
    configureUndoRedoPane();

    configureDisplaySelector();

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

void Titlebar::configureTransportPane()
{
    playButton = &transportPane.addButton(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawPlayIcon(g, bounds, state);
        },
        "Play / Pause",
        [this]() { togglePlayback(); });

    playButton->setSelected(true);

    transportPane.addButton(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawResetIcon(g, bounds, state);
        },
        "Reset",
        [this]() { resetTraversals(); });
}

void Titlebar::togglePlayback()
{
    NodeCanvas& canvas = *applicationContext.canvas;
    jassert(&canvas);

    playButton->setSelected(!playButton->isSelected());

    canvas.start = !canvas.start;
    canvas.setProcessorPlayblack(canvas.start);
}

void Titlebar::resetTraversals()
{
    applicationContext.processor->resetRequested.store(true);

    if (auto* canvas = applicationContext.canvas) {
        canvas->arrowManager.resetAllProgress();
    }
}

void Titlebar::configureModePane()
{
    buttonPane.enableToggleSelection();
    buttonPane.allowEmptySelection();

    buttonPane.onSelectionChanged = [this](const IconButton* selected) {
        setDanglingArrowMode(selected == nullptr);
    };

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

void Titlebar::setDanglingArrowMode(bool shouldBeActive)
{
    applicationContext.canvas->danglingArrowLayer.setArrowMode(shouldBeActive);
}

void Titlebar::paintOverBar(juce::Graphics& g)
{
    drawSeparator(g, (transportPane.getRight() + tempoDisplay.getX()) / 2);
    drawSeparator(g, (tempoDisplay.getRight() + undoRedoPane.getX()) / 2);
    drawSeparator(g, (buttonPane.getRight() + displaySelector.getX()) / 2);
}

void Titlebar::resized()
{
    auto bounds = getContentBounds();

    int transportPaneWidth = bounds.getWidth() / 8;
    int tempoDisplayWidth = bounds.getWidth() / 8;
    int buttonPaneWidth = bounds.getWidth() / 8;
    int displaySelectorWidth = bounds.getWidth() / 8;
    int undoRedoPaneWidth = bounds.getWidth() / 8;

    transportPane.setBounds(bounds.removeFromLeft(transportPaneWidth));
    bounds.removeFromLeft(contentSpacing);

    tempoDisplay.setBounds(bounds.removeFromLeft(tempoDisplayWidth));
    bounds.removeFromLeft(contentSpacing);

    undoRedoPane.setBounds(bounds.removeFromLeft(undoRedoPaneWidth));
    bounds.removeFromLeft(contentSpacing);

    displaySelector.setBounds(bounds.removeFromRight(displaySelectorWidth));
    bounds.removeFromRight(contentSpacing);

    buttonPane.setBounds(bounds.removeFromRight(buttonPaneWidth));
}
