#include "../Canvas/NodeCanvas.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Plugin/PluginProcessor.h"

#include "Titlebar.h"

Titlebar::Titlebar(const ApplicationContext& context)
    : Bar(context, { Orientation::Horizontal, Theme::contentInsetRatio, Surface::Frosted }),
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
    configureTempoDisplay();
}

Titlebar::~Titlebar()
{
    applicationContext.undoManager->removeChangeListener(this);
}

void Titlebar::resized()
{
    auto bounds = getContentBounds();

    int transportPaneWidth = transportPane.idealWidth(bounds.getHeight());
    int tempoDisplayWidth = bounds.getWidth() / 9;
    int buttonPaneWidth = bounds.getWidth() / 4;
    int displaySelectorWidth = bounds.getWidth() / 6;
    int undoRedoPaneWidth = undoRedoPane.idealWidth(bounds.getHeight());
    int spacing = juce::roundToInt(bounds.getWidth() * Theme::contentSpacingRatio);
    float textHeight = CustomLookAndFeel::get(*this).textHeight;

    tempoDisplay.editor.setFontHeight(textHeight);
    displaySelector.labelEditor.setFontHeight(textHeight);

    transportPane.setBounds(bounds.removeFromLeft(transportPaneWidth));
    bounds.removeFromLeft(spacing);

    tempoDisplay.setBounds(bounds.removeFromLeft(tempoDisplayWidth));
    bounds.removeFromLeft(spacing);

    undoRedoPane.setBounds(bounds.removeFromLeft(undoRedoPaneWidth));
    bounds.removeFromLeft(spacing);

    displaySelector.setBounds(bounds.removeFromRight(displaySelectorWidth));
    bounds.removeFromRight(spacing);

    buttonPane.setBounds(bounds.removeFromRight(buttonPaneWidth));
}

void Titlebar::configureTransportPane()
{
    playButton = &transportPane.addButton(&CustomLookAndFeel::drawPlayIcon, "Play / Pause",
        [this]() { applyPlaybackState(!applicationContext.canvas->start); });

    playButton->state.look = ButtonState::Look::Accent;

    if (applicationContext.processor->wrapperType != juce::AudioProcessor::wrapperType_Standalone) {
        playButton->onClick = nullptr;

        playButton->setTooltip("Follows host transport");
    }

    playButton->setSelected(applicationContext.processor->isPlaying.load());

    transportPane.addButton(&CustomLookAndFeel::drawResetIcon, "Reset",
        [this]() { resetTraversals(); });

    if (applicationContext.processor->wrapperType == juce::AudioProcessor::wrapperType_Standalone) {
        return;
    }

    syncButton = &transportPane.addButton(&CustomLookAndFeel::drawSyncIcon, "Sync to host playhead",
        [this]() { syncAttachment->setValueAsCompleteGesture(static_cast<float>(! syncButton->state.isSelected)); });

    syncAttachment = std::make_unique<juce::ParameterAttachment>(
        *applicationContext.processor->valueTreeState.getParameter(SequenceTreeAudioProcessor::hostSyncParameterId),
        [this](float synced) { syncButton->setSelected(synced >= 0.5f); });

    syncAttachment->sendInitialUpdate();
}

void Titlebar::applyPlaybackState(bool shouldPlay)
{
    playButton->setSelected(shouldPlay);

    applicationContext.canvas->setProcessorPlayback(shouldPlay);
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
    buttonPane.selection = ButtonPane::Selection::ExclusiveOrNone;

    buttonPane.onSelectionChanged = [this](const IconButton* selected) {
        applicationContext.nodeController->setArrowMode(selected == nullptr);
    };

    IconButton& nodeButton = buttonPane.addButton(&CustomLookAndFeel::drawNodeModeIcon, "Node Mode",
        [this]() { applicationContext.nodeController->nodeControllerMode = NodeController::NodeControllerMode::Node; });

    IconButton& modulatorButton = buttonPane.addButton(&CustomLookAndFeel::drawModulatorIcon, "Modulator Mode",
        [this]() { applicationContext.nodeController->nodeControllerMode = NodeController::NodeControllerMode::Modulator; });

    IconButton& flagButton = buttonPane.addButton(&CustomLookAndFeel::drawTraversalFlagIcon, "Traversal Flag Mode",
        [this]() { applicationContext.nodeController->nodeControllerMode = NodeController::NodeControllerMode::TraversalFlag; });

    nodeButton.state.look      = ButtonState::Look::Raised;
    modulatorButton.state.look = ButtonState::Look::Raised;
    flagButton.state.look      = ButtonState::Look::Raised;

    nodeButton.setText("Node");
    modulatorButton.setText("Modulator");
    flagButton.setText("Flag");

    buttonPane.setSelectedButton(&nodeButton);
}

void Titlebar::configureUndoRedoPane()
{
    undoButton = &undoRedoPane.addButton(&CustomLookAndFeel::drawUndoIcon, "Undo",
        [this]() { applicationContext.undoManager->undo(); });

    redoButton = &undoRedoPane.addButton(&CustomLookAndFeel::drawRedoIcon, "Redo",
        [this]() { applicationContext.undoManager->redo(); });

    applicationContext.undoManager->addChangeListener(this);

    changeListenerCallback(applicationContext.undoManager);
}

void Titlebar::changeListenerCallback(juce::ChangeBroadcaster*)
{
    undoButton->setEnabled(applicationContext.undoManager->canUndo());
    redoButton->setEnabled(applicationContext.undoManager->canRedo());
}

void Titlebar::configureDisplaySelector()
{
    auto addDisplayMode = [this](int itemId, juce::String label, NodeDisplayMode mode) {
        displaySelector.addItem(itemId, std::move(label), [this, mode]() {
            applicationContext.canvas->nodeManager.setDisplayMode(mode);

            if (onDisplayModeChanged) {
                onDisplayModeChanged(mode);
            }
        });
    };

    displaySelector.labelEditor.autoFitText = false;
    displaySelector.leadingIcon             = &CustomLookAndFeel::drawEyeIcon;

    addDisplayMode(1, "show pitch",       NodeDisplayMode::Pitch);
    addDisplayMode(2, "show velocity",    NodeDisplayMode::Velocity);
    addDisplayMode(3, "show countLimit",  NodeDisplayMode::CountLimit);
    addDisplayMode(4, "show channel",     NodeDisplayMode::Channel);
    addDisplayMode(5, "show repeatValue", NodeDisplayMode::RepeatValue);
    addDisplayMode(6, "show probability", NodeDisplayMode::Probability);

    displaySelector.setSelectedItem(1);
}

void Titlebar::configureTempoDisplay()
{
    SequenceTreeAudioProcessor& processor = *applicationContext.processor;

    tempoAttachment = std::make_unique<juce::ParameterAttachment>(
        *processor.valueTreeState.getParameter(SequenceTreeAudioProcessor::tempoParameterId),
        [this](float tempoMultiplier) { tempoDisplay.editor.boundValue.setValue(tempoMultiplier); });

    tempoDisplay.editor.onValueChange = [this]() {
        tempoAttachment->setValueAsCompleteGesture(static_cast<float>(static_cast<double>(tempoDisplay.editor.boundValue.getValue())));
    };

    tempoAttachment->sendInitialUpdate();
}
