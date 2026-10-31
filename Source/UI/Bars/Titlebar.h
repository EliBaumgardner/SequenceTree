#pragma once

#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/Buttons/ButtonPane.h>
#include <ArsNovaUI/Menus/ItemSelector.h>
#include <ArsNovaUI/Menus/SettingsMenu.h>
#include <ArsNovaUI/PopupWindow.h>
#include <ArsNovaUI/ScaledView.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../Menus/PatchControls.h"
#include "../Buttons/TempoDisplay.h"
#include "../../Input/NodeController.h"

/*
    The bar along the top of the editor, holding the controls for playback and for what new nodes
    are: play, reset and (in a plugin host) host sync buttons; the tempo multiplier; undo and redo;
    patch controls; three buttons that choose whether dragging creates a node, a modulator or a
    traversal flag; a selector for which property nodes display; and a settings button that opens
    a SettingsMenu in a pop-up window. It is a Bar drawn as frosted glass over the NodeCanvas.

    The play button calls applyPlaybackState, which stores the processor's play state through
    NodeCanvas::setProcessorPlayback; in a plugin host it only follows the host transport. Reset
    sets the processor's resetRequested flag and clears the arrows' progress. The tempo editor and
    the sync button are connected to the processor's parameters through ParameterAttachments. The
    creation buttons set GraphDocument::creationMode and switch arrow mode through
    arrowGestures. The undo and redo buttons use the UndoManager and are enabled to match it. The
    display selector sets NodeCanvas's display mode and then calls onDisplayModeChanged.

    PluginEditor owns it and asks narrowestWidth for the smallest window the controls fit in.
*/

class NodeCanvas;
class GraphDocument;
class SequenceTreeAudioProcessor;

class Titlebar : public ArsNovaUI::Bar, private juce::ChangeListener
{
public:

    Titlebar(SequenceTreeAudioProcessor& processor, NodeCanvas& nodeCanvas, NodeController& nodeController, GraphDocument& graphDocument,
             juce::PropertiesFile& interfaceSettings, ArsNovaUI::ScaledView& scaledView, juce::ValueTree colourPresets, juce::UndoManager& undoManager);
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
    juce::PropertiesFile&       interfaceSettings;
    ArsNovaUI::ScaledView&      scaledView;

    juce::ValueTree colourPresets;

    ArsNovaUI::ButtonPane   transportPane;
    ArsNovaUI::ButtonPane   buttonPane;
    ArsNovaUI::ItemSelector displaySelector;
    TempoDisplay            tempoDisplay;
    ArsNovaUI::ButtonPane   undoRedoPane;
    PatchControls           patchControls;
    ArsNovaUI::ButtonPane   settingsPane;

    ArsNovaUI::PopupWindowLauncher settingsLauncher {
        "Settings",
        [this]() {
            auto content = std::make_unique<ArsNovaUI::SettingsMenu>(ArsNovaUI::CustomLookAndFeel::get(*this), interfaceSettings, scaledView, colourPresets, undoManager);

            const juce::Point<int> size = content->narrowestSize();

            content->setSize(size.x, size.y);

            return content;
        }
    };

    ArsNovaUI::IconButton* playButton = nullptr;
    ArsNovaUI::IconButton* syncButton = nullptr;

    ArsNovaUI::IconButton* undoButton = nullptr;
    ArsNovaUI::IconButton* redoButton = nullptr;

    std::unique_ptr<juce::ParameterAttachment> tempoAttachment;
    std::unique_ptr<juce::ParameterAttachment> syncAttachment;
};
