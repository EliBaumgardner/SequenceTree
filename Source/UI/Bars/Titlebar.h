#pragma once

#include "Bar.h"
#include "../Buttons/IconButton.h"
#include "../Buttons/ButtonPane.h"
#include "../Menus/ItemSelector.h"
#include "../Buttons/TempoDisplay.h"
#include "../../Input/NodeController.h"

class Titlebar : public Bar
{
public:

    Titlebar(const ApplicationContext& context);

    void paintOverBar(juce::Graphics& graphics) override;
    void resized() override;

    void applyPlaybackState(bool shouldPlay);

    std::function<void(NodeDisplayMode)> onDisplayModeChanged;

private:

    void configureDisplaySelector();
    void configureTempoDisplay();
    void configureModePane();
    void configureTransportPane();
    void configureUndoRedoPane();

    void resetTraversals();

    ButtonPane           transportPane;
    ButtonPane           buttonPane;
    ItemSelector         displaySelector;
    TempoDisplay         tempoDisplay;
    ButtonPane           undoRedoPane;

    IconButton*          playButton = nullptr;
    IconButton*          syncButton = nullptr;

    std::unique_ptr<juce::ParameterAttachment> tempoAttachment;
    std::unique_ptr<juce::ParameterAttachment> syncAttachment;
};
