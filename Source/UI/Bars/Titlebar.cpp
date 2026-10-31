#include "../Canvas/NodeCanvas.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../../Plugin/PluginProcessor.h"
#include "../../Input/GraphDocument.h"

#include "Titlebar.h"

Titlebar::Titlebar(SequenceTreeAudioProcessor& processor, NodeCanvas& nodeCanvas, NodeController& nodeController, GraphDocument& graphDocument,
                   juce::UndoManager& undoManager)
    : ArsNovaUI::Bar(nodeCanvas, Orientation::Horizontal, ArsNovaUI::Theme::contentInsetRatio),
      processor(processor),
      nodeCanvas(nodeCanvas),
      nodeController(nodeController),
      graphDocument(graphDocument),
      undoManager(undoManager),
      displaySelector(undoManager),
      tempoDisplay(undoManager),
      patchControls(processor, processor.patch, processor.patchFolder)
{
    addAndMakeVisible(transportPane);
    addAndMakeVisible(tempoDisplay);
    addAndMakeVisible(buttonPane);
    addAndMakeVisible(displaySelector);
    addAndMakeVisible(undoRedoPane);
    addAndMakeVisible(patchControls);

    configureTransportPane();
    configureModePane();
    configureUndoRedoPane();

    configureDisplaySelector();
    configureTempoDisplay();
}

Titlebar::~Titlebar()
{
    undoManager.removeChangeListener(this);
}

void Titlebar::resized()
{
    auto bounds = getContentBounds();

    int transportPaneWidth = transportPane.idealWidth(bounds.getHeight());
    int undoRedoPaneWidth = undoRedoPane.idealWidth(bounds.getHeight());
    int patchPaneWidth = patchControls.idealWidth(bounds.getHeight());
    int spacing = contentSpacing;
    float textHeight = ArsNovaUI::Theme::textHeight;

    tempoDisplay.editor.setFontHeight(textHeight);
    displaySelector.labelEditor.setFontHeight(textHeight);

    transportPane.setBounds(bounds.removeFromLeft(transportPaneWidth));
    bounds.removeFromLeft(spacing);

    tempoDisplay.setBounds(bounds.removeFromLeft(tempoDisplayWidth));
    bounds.removeFromLeft(spacing);

    undoRedoPane.setBounds(bounds.removeFromLeft(undoRedoPaneWidth));
    bounds.removeFromLeft(spacing);

    patchControls.setBounds(bounds.removeFromLeft(patchPaneWidth));
    bounds.removeFromLeft(spacing);

    displaySelector.setBounds(bounds.removeFromRight(displaySelectorWidth));
    bounds.removeFromRight(spacing);

    buttonPane.setBounds(bounds.removeFromRight(modePaneWidth));
}

void Titlebar::applyPlaybackState(bool shouldPlay)
{
    playButton->setSelected(shouldPlay);

    nodeCanvas.setProcessorPlayback(shouldPlay);
}

int Titlebar::narrowestWidth() const
{
    const auto bounds = getContentBounds();
    const int  height = bounds.getHeight();

    return getWidth() - bounds.getWidth() + transportPane.idealWidth(height) + tempoDisplayWidth + undoRedoPane.idealWidth(height) + patchControls.idealWidth(height)
           + displaySelectorWidth + modePaneWidth + contentSpacing * 5;
}

void Titlebar::configureTransportPane()
{
    playButton = &transportPane.addButton(&ArsNovaUI::CustomLookAndFeel::drawPlayIcon, "Play / Pause",
                                          [this]() { applyPlaybackState(!nodeCanvas.start); });

    playButton->state.look = ArsNovaUI::ButtonState::Look::Accent;

    if (processor.wrapperType != juce::AudioProcessor::wrapperType_Standalone) {
        playButton->onClick = nullptr;

        playButton->setTooltip("Follows host transport");
    }

    playButton->setSelected(processor.isPlaying.load());

    transportPane.addButton(&ArsNovaUI::CustomLookAndFeel::drawSkipBackIcon, "Reset",
        [this]() { resetTraversals(); });

    if (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone) {
        return;
    }

    syncButton = &transportPane.addButton(&ArsNovaUI::CustomLookAndFeel::drawLinkIcon, "Sync to host playhead",
        [this]() { syncAttachment->setValueAsCompleteGesture(static_cast<float>(! syncButton->state.isSelected)); });

    syncAttachment = std::make_unique<juce::ParameterAttachment>(
        *processor.valueTreeState.getParameter(SequenceTreeAudioProcessor::hostSyncParameterId),
        [this](float synced) { syncButton->setSelected(synced >= 0.5f); });

    syncAttachment->sendInitialUpdate();
}

void Titlebar::configureModePane()
{
    buttonPane.selection = ArsNovaUI::ButtonPane::Selection::ExclusiveOrNone;

    buttonPane.onSelectionChanged = [this](const ArsNovaUI::IconButton* selected) {
        nodeController.arrowGestures.setArrowMode(selected == nullptr);
    };

    ArsNovaUI::IconButton& nodeButton = buttonPane.addButton(&ArsNovaUI::CustomLookAndFeel::drawCircleIcon, "Node Mode",
        [this]() { graphDocument.creationMode = NodeCreationMode::Node; });

    ArsNovaUI::IconButton& modulatorButton = buttonPane.addButton(&ArsNovaUI::CustomLookAndFeel::drawRoundedSquareIcon, "Modulator Mode",
        [this]() { graphDocument.creationMode = NodeCreationMode::Modulator; });

    ArsNovaUI::IconButton& flagButton = buttonPane.addButton(&ArsNovaUI::CustomLookAndFeel::drawTriangleIcon, "Traversal Flag Mode",
        [this]() { graphDocument.creationMode = NodeCreationMode::TraversalFlag; });

    nodeButton.state.look      = ArsNovaUI::ButtonState::Look::Raised;
    modulatorButton.state.look = ArsNovaUI::ButtonState::Look::Raised;
    flagButton.state.look      = ArsNovaUI::ButtonState::Look::Raised;

    nodeButton.setText("Node");
    modulatorButton.setText("Modulator");
    flagButton.setText("Flag");

    buttonPane.setSelectedButton(&nodeButton);
}

void Titlebar::configureUndoRedoPane()
{
    undoButton = &undoRedoPane.addButton(&ArsNovaUI::CustomLookAndFeel::drawUndoIcon, "Undo",
        [this]() { undoManager.undo(); });

    redoButton = &undoRedoPane.addButton(&ArsNovaUI::CustomLookAndFeel::drawRedoIcon, "Redo",
        [this]() { undoManager.redo(); });

    undoManager.addChangeListener(this);

    changeListenerCallback(&undoManager);
}

void Titlebar::configureDisplaySelector()
{
    auto addDisplayMode = [this](int itemId, juce::String label, NodeInfo::DisplayMode mode) {
        displaySelector.addItem(itemId, std::move(label), [this, mode]() {
            nodeCanvas.nodeManager.setDisplayMode(mode);

            if (onDisplayModeChanged) {
                onDisplayModeChanged(mode);
            }
        });
    };

    displaySelector.labelEditor.autoFitText = false;
    displaySelector.leadingIcon             = &ArsNovaUI::CustomLookAndFeel::drawEyeIcon;

    addDisplayMode(1, "pitch",       NodeInfo::DisplayMode::Pitch);
    addDisplayMode(2, "velocity",    NodeInfo::DisplayMode::Velocity);
    addDisplayMode(3, "countLimit",  NodeInfo::DisplayMode::CountLimit);
    addDisplayMode(4, "channel",     NodeInfo::DisplayMode::Channel);
    addDisplayMode(5, "repeatValue", NodeInfo::DisplayMode::RepeatValue);
    addDisplayMode(6, "probability", NodeInfo::DisplayMode::Probability);

    displaySelector.setSelectedItem(1);
}

void Titlebar::configureTempoDisplay()
{
    tempoAttachment = std::make_unique<juce::ParameterAttachment>(
        *processor.valueTreeState.getParameter(SequenceTreeAudioProcessor::tempoParameterId),
        [this](float tempoMultiplier) { tempoDisplay.editor.boundValue.setValue(tempoMultiplier); });

    tempoDisplay.editor.onValueChange = [this]() {
        tempoAttachment->setValueAsCompleteGesture(static_cast<float>(static_cast<double>(tempoDisplay.editor.boundValue.getValue())));
    };

    tempoAttachment->sendInitialUpdate();
}

void Titlebar::resetTraversals()
{
    processor.resetRequested.store(true);

    nodeCanvas.arrowManager.resetAllProgress();
}

void Titlebar::changeListenerCallback(juce::ChangeBroadcaster*)
{
    undoButton->setEnabled(undoManager.canUndo());
    redoButton->setEnabled(undoManager.canRedo());
}
