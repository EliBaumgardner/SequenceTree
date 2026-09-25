/*
  ==============================================================================

    TitleBar.h
    Created: 12 Jun 2025 6:50:43pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once


#include "Bar.h"
#include "../Buttons/IconButton.h"
#include "../Buttons/ButtonPane.h"
#include "../Menus/ItemSelector.h"
#include "../Buttons/TempoDisplay.h"
#include "../../Input/NodeController.h"

class Titlebar : public Bar {

public:

    Titlebar(const ApplicationContext& context);

    void applyPlaybackState(bool shouldPlay);
    void togglePlayback();

    std::function<void(NodeDisplayMode)> onDisplayModeChanged;

private:

    void paintOverBar(juce::Graphics& g) override;
    void resized() override;

    void configureDisplaySelector();
    void configureTempoDisplay();
    void configureModePane();
    void configureTransportPane();
    void configureUndoRedoPane();
    void setControllerMode(NodeController::NodeControllerMode mode);
    void setDanglingArrowMode(bool shouldBeActive);

    void resetTraversals();

    ButtonPane           transportPane;
    ButtonPane           buttonPane;
    ItemSelector         displaySelector;
    TempoDisplay         tempoDisplay;
    ButtonPane           undoRedoPane;

    IconButton*          playButton = nullptr;

    std::unique_ptr<juce::ParameterAttachment> tempoAttachment;
};
