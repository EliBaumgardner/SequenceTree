/*
  ==============================================================================

    TitleBar.h
    Created: 12 Jun 2025 6:50:43pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once


#include "Bar.h"
#include "Buttons/IconButton.h"
#include "Buttons/ButtonPane.h"
#include "Menus/ItemSelector.h"
#include "Buttons/TempoDisplay.h"
#include "../Input/NodeController.h"

class Titlebar : public Bar {

public:

    Titlebar(ApplicationContext& context);

    std::function<void()> toggled;

private:

    void paintOverBar(juce::Graphics& g) override;
    void resized() override;

    void configureDisplaySelector();
    void configureModePane();
    void configureTransportPane();
    void configureUndoRedoPane();
    void setControllerMode(NodeController::NodeControllerMode mode);
    void setDanglingArrowMode(bool shouldBeActive);

    void togglePlayback();
    void resetTraversals();

    ButtonPane           transportPane;
    ButtonPane           buttonPane;
    ItemSelector         displaySelector;
    TempoDisplay         tempoDisplay;
    ButtonPane           undoRedoPane;

    IconButton*          playButton = nullptr;
};
