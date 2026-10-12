#pragma once

#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/Buttons/ButtonPane.h>
#include <ArsNovaUI/Menus/ItemSelector.h>
#include "../Menus/PatchControls.h"
#include "../Buttons/TempoDisplay.h"
#include "../../Input/NodeController.h"

class NodeCanvas;
class SequenceTreeAudioProcessor;

class Titlebar : public Bar, private juce::ChangeListener
{
public:

    Titlebar(SequenceTreeAudioProcessor& processor, NodeCanvas& nodeCanvas, NodeController& nodeController, juce::UndoManager& undoManager);
    ~Titlebar() override;

    void resized() override;

    void applyPlaybackState(bool shouldPlay);

    int narrowestWidth() const;

    std::function<void(NodeDisplayMode)> onDisplayModeChanged;

private:

    void configureTransportPane  ();
    void configureModePane       ();
    void configureUndoRedoPane   ();
    void configureDisplaySelector();
    void configureTempoDisplay   ();

    void resetTraversals();

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    static constexpr int tempoDisplayWidth    = 57;
    static constexpr int modePaneWidth        = 159;
    static constexpr int displaySelectorWidth = 70;

    SequenceTreeAudioProcessor& processor;
    NodeCanvas&                 nodeCanvas;
    NodeController&             nodeController;
    juce::UndoManager&          undoManager;

    ButtonPane    transportPane;
    ButtonPane    buttonPane;
    ItemSelector  displaySelector;
    TempoDisplay  tempoDisplay;
    ButtonPane    undoRedoPane;
    PatchControls patchControls;

    IconButton* playButton = nullptr;
    IconButton* syncButton = nullptr;

    IconButton* undoButton = nullptr;
    IconButton* redoButton = nullptr;

    std::unique_ptr<juce::ParameterAttachment> tempoAttachment;
    std::unique_ptr<juce::ParameterAttachment> syncAttachment;
};
