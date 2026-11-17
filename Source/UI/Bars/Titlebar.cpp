#include "../Canvas/NodeCanvas.h"
#include <ArsNovaUI/Theme/IconLibrary.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../../Plugin/PluginProcessor.h"
#include "../../Input/GraphDocument.h"

#include "Titlebar.h"

Titlebar::Titlebar(SequenceTreeAudioProcessor& processor, NodeCanvas& nodeCanvas, NodeController& nodeController, GraphDocument& graphDocument)
    : ArsNovaUI::Bar (nodeCanvas, Orientation::Horizontal, ArsNovaUI::Theme::contentInsetRatio),
      processor      (processor),
      nodeCanvas     (nodeCanvas),
      nodeController (nodeController),
      graphDocument  (graphDocument),
      undoManager    (processor.document.undoManager),
      displaySelector(undoManager),
      tempoDisplay   (undoManager),
      patchControls  (processor, processor.document.patch, processor.document.patchFolder)
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
    int undoRedoPaneWidth  = undoRedoPane.idealWidth(bounds.getHeight());
    int patchPaneWidth     = patchControls.idealWidth(bounds.getHeight());
    int modePaneWidth      = buttonPane.idealWidth(bounds.getHeight());
    int spacing            = contentSpacing;

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

    processor.playback.isPlaying.store(shouldPlay);

    nodeCanvas.setPlaybackActive(shouldPlay);

    processor.rtGraphBuilder.handleUpdateNowIfNeeded();
}

int Titlebar::narrowestWidth() const
{
    const auto bounds = getContentBounds();

    const int paneGapCount = 5;
    const int height       = bounds.getHeight();

    return getWidth() - bounds.getWidth() + transportPane.idealWidth(height) + tempoDisplayWidth + undoRedoPane.idealWidth(height) + patchControls.idealWidth(height)
           + displaySelectorWidth + buttonPane.idealWidth(height) + contentSpacing * paneGapCount;
}

void Titlebar::configureTransportPane()
{
    playButton = &transportPane.addButton(ArsNovaUI::IconLibrary::get().play, "Play / Pause",
                                          [this]() { applyPlaybackState(!processor.playback.isPlaying.load()); });

    playButton->state.look = ArsNovaUI::ButtonState::Look::Raised;

    if (processor.wrapperType != juce::AudioProcessor::wrapperType_Standalone) {
        playButton->onClick = nullptr;

        playButton->setTooltip("Follows host transport");
    }

    playButton->setSelected(processor.playback.isPlaying.load());

    transportPane.addButton(ArsNovaUI::IconLibrary::get().skipBack, "Reset",
        [this]() { resetTraversals(); });

    if (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone) {
        return;
    }

    syncButton = &transportPane.addButton(ArsNovaUI::IconLibrary::get().link, "Sync to host playhead",
        [this]() {
            const float syncValue = static_cast<float>(! syncButton->state.isSelected);

            syncAttachment->setValueAsCompleteGesture(syncValue);
        });

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

    ArsNovaUI::IconButton& nodeButton = buttonPane.addButton(ArsNovaUI::IconLibrary::get().circle, "Node",
        [this]() { graphDocument.creationMode = NodeCreationMode::Node; });

    ArsNovaUI::IconButton& modulatorButton = buttonPane.addButton(ArsNovaUI::IconLibrary::get().roundedSquare, "Modulator",
        [this]() { graphDocument.creationMode = NodeCreationMode::Modulator; });

    ArsNovaUI::IconButton& flagButton = buttonPane.addButton(ArsNovaUI::IconLibrary::get().triangle, "Flag",
        [this]() { graphDocument.creationMode = NodeCreationMode::TraversalFlag; });

    nodeButton.state.look      = ArsNovaUI::ButtonState::Look::Raised;
    modulatorButton.state.look = ArsNovaUI::ButtonState::Look::Raised;
    flagButton.state.look      = ArsNovaUI::ButtonState::Look::Raised;


    buttonPane.setSelectedButton(&nodeButton);
}

void Titlebar::configureUndoRedoPane()
{
    undoButton = &undoRedoPane.addButton(ArsNovaUI::IconLibrary::get().undo, "Undo",
        [this]() { undoManager.undo(); });

    redoButton = &undoRedoPane.addButton(ArsNovaUI::IconLibrary::get().redo, "Redo",
        [this]() { undoManager.redo(); });

    undoManager.addChangeListener(this);

    changeListenerCallback(&undoManager);
}

void Titlebar::configureDisplaySelector()
{
    auto addDisplayMode = [this](int itemId, juce::String label, NodeInfo::DisplayMode mode) {
        displaySelector.addItem(itemId, std::move(label), [this, mode]() {
            nodeCanvas.setDisplayMode(mode);

            if (onDisplayModeChanged) {
                onDisplayModeChanged(mode);
            }
        });
    };

    displaySelector.labelEditor.autoFitText = false;
    displaySelector.leadingIcon             = &ArsNovaUI::IconLibrary::get().eye;

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
        const double typedTempo = tempoDisplay.editor.boundValue.getValue();

        const float tempoValue = static_cast<float>(typedTempo);

        tempoAttachment->setValueAsCompleteGesture(tempoValue);
    };

    tempoAttachment->sendInitialUpdate();
}

void Titlebar::resetTraversals()
{
    processor.playback.resetRequested.store(true);

    nodeCanvas.resetAllProgress();
}

void Titlebar::changeListenerCallback(juce::ChangeBroadcaster*)
{
    undoButton->setEnabled(undoManager.canUndo());
    redoButton->setEnabled(undoManager.canRedo());
}
