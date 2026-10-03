#pragma once

#include "Bar.h"
#include "../Buttons/IconButton.h"
#include "../Buttons/ButtonPane.h"
#include "../Menus/ItemSelector.h"
#include "../Buttons/TempoDisplay.h"
#include "../../Input/NodeController.h"

class Titlebar : public Bar, private juce::ChangeListener
{
public:

    Titlebar(const ApplicationContext& context);
    ~Titlebar() override;

    void resized() override;

    void applyPlaybackState(bool shouldPlay);

    std::function<void(NodeDisplayMode)> onDisplayModeChanged;

private:

    void configureTransportPane();
    void resetTraversals();
    void configureModePane();
    void configureUndoRedoPane();
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void configureDisplaySelector();
    void configureTempoDisplay();

    ButtonPane           transportPane;
    ButtonPane           buttonPane;
    ItemSelector         displaySelector;
    TempoDisplay         tempoDisplay;
    ButtonPane           undoRedoPane;

    IconButton*          playButton = nullptr;
    IconButton*          syncButton = nullptr;
    IconButton*          undoButton = nullptr;
    IconButton*          redoButton = nullptr;

    std::unique_ptr<juce::ParameterAttachment> tempoAttachment;
    std::unique_ptr<juce::ParameterAttachment> syncAttachment;
};
