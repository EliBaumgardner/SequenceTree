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

class Titlebar : public ArsNovaUI::Bar, private juce::ChangeListener
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

    ArsNovaUI::ButtonPane   transportPane;
    ArsNovaUI::ButtonPane   buttonPane;
    ArsNovaUI::ItemSelector displaySelector;
    TempoDisplay            tempoDisplay;
    ArsNovaUI::ButtonPane   undoRedoPane;
    PatchControls           patchControls;

    ArsNovaUI::IconButton* playButton = nullptr;
    ArsNovaUI::IconButton* syncButton = nullptr;

    ArsNovaUI::IconButton* undoButton = nullptr;
    ArsNovaUI::IconButton* redoButton = nullptr;

    std::unique_ptr<juce::ParameterAttachment> tempoAttachment;
    std::unique_ptr<juce::ParameterAttachment> syncAttachment;
};
