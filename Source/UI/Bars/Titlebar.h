#pragma once

#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/Buttons/ButtonPane.h>
#include <ArsNovaUI/Menus/ItemSelector.h>
#include "../Menus/PatchControls.h"
#include "../Buttons/TempoDisplay.h"
#include "../../Input/NodeController.h"

/*
    The bar along the top of the editor, holding the controls for playback and for what new nodes
    are: play, reset and (in a plugin host) host sync buttons; the tempo multiplier; undo and redo;
    patch controls; three buttons that choose whether dragging creates a node, a modulator or a
    traversal flag; and a selector for which property nodes display. It is a Bar drawn as frosted
    glass over the NodeCanvas.

    The play button calls applyPlaybackState, which stores the processor's play state through
    NodeCanvas::setProcessorPlayback; in a plugin host it only follows the host transport. Reset
    sets the processor's resetRequested flag and clears the arrows' progress. The tempo editor and
    the sync button are connected to the processor's parameters through ParameterAttachments. The
    creation buttons set GraphDocument::creationMode and switch arrow mode through
    arrowGestures. The undo and redo buttons use the UndoManager and are enabled to match it. The
    display selector sets NodeManager's display mode and then calls onDisplayModeChanged.

    PluginEditor owns it and asks narrowestWidth for the smallest window the controls fit in.
*/

class NodeCanvas;
class GraphDocument;
class SequenceTreeAudioProcessor;

class Titlebar : public ArsNovaUI::Bar, private juce::ChangeListener
{
public:

    Titlebar(SequenceTreeAudioProcessor& processor, NodeCanvas& nodeCanvas, NodeController& nodeController, GraphDocument& graphDocument,
             juce::UndoManager& undoManager);
    ~Titlebar() override;

    void resized() override;

    void applyPlaybackState(bool shouldPlay);

    int narrowestWidth() const;

    std::function<void(NodeInfo::DisplayMode)> onDisplayModeChanged;

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
    GraphDocument&              graphDocument;
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
