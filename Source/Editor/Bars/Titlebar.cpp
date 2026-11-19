#include "Editor/Canvas/NodeCanvas.h"
#include <ArsNovaUI/Theme/IconLibrary.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "Processor/PluginProcessor.h"
#include "Editor/Canvas/GraphDocument.h"

#include "Titlebar.h"

Titlebar::Titlebar(SequenceTreeAudioProcessor& processor, NodeCanvas& nodeCanvas, NodeController& nodeController, GraphDocument& graphDocument)
    : ArsNovaUI::Bar (nodeCanvas, Orientation::Horizontal, 0.0f),
      processor      (processor),
      nodeCanvas     (nodeCanvas),
      nodeController (nodeController),
      graphDocument  (graphDocument),
      undoManager    (processor.document.undoManager),
      displaySelector(undoManager),
      tempoDisplay   (undoManager),
      patchControls  (processor, processor.document.patch, processor.document.patchFolder)
{
    transportPane.look   = ArsNovaUI::ButtonPane::Look::Flat;
    buttonPane.look      = ArsNovaUI::ButtonPane::Look::Flat;
    undoRedoPane.look    = ArsNovaUI::ButtonPane::Look::Flat;
    displaySelector.look = ArsNovaUI::ItemSelector::Look::Flat;

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
    const float textHeight = ArsNovaUI::Theme::textHeight;

    const ArsNovaUI::Bar::Layout layout = makeLayout();

    tempoDisplay.editor.setFontHeight(textHeight);
    displaySelector.labelEditor.setFontHeight(textHeight);

    placeLayout(layout);
}

void Titlebar::applyPlaybackState(bool shouldPlay)
{
    playButton->setSelected(shouldPlay);

    processor.playback.isPlaying.store(shouldPlay);

    nodeCanvas.setPlaybackActive(shouldPlay);

    processor.rtGraphBuilder.handleUpdateNowIfNeeded();
}

int Titlebar::narrowestWidth()
{
    const ArsNovaUI::Bar::Layout layout = makeLayout();

    return layoutWidth(layout);
}

ArsNovaUI::Bar::Layout Titlebar::makeLayout()
{
    const int height = getContentBounds().getHeight();

    const ArsNovaUI::Bar::Group transportGroup { { &transportPane, transportPane.idealWidth(height) }, { &tempoDisplay, tempoDisplayWidth } };
    const ArsNovaUI::Bar::Group editGroup      { { &undoRedoPane, undoRedoPane.idealWidth(height) } };
    const ArsNovaUI::Bar::Group patchGroup     { { &patchControls, patchControls.idealWidth(height) } };
    const ArsNovaUI::Bar::Group createGroup    { { &buttonPane, buttonPane.idealWidth(height) } };
    const ArsNovaUI::Bar::Group viewGroup      { { &displaySelector, displaySelectorWidth } };

    return { { transportGroup, editGroup, patchGroup }, { createGroup, viewGroup } };
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
