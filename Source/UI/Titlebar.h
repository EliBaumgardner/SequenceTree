/*
  ==============================================================================

    TitleBar.h
    Created: 12 Jun 2025 6:50:43pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once


#include "../Util/PluginModules.h"
#include "../Util/ApplicationContext.h"
#include "Buttons/IconButton.h"
#include "Buttons/ButtonPane.h"
#include "Menus/ItemSelector.h"
#include "Buttons/TempoDisplay.h"
#include "../Input/NodeController.h"

class Titlebar : public juce::Component {

public:

    Titlebar(ApplicationContext& context);

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void()> toggled;

private:

    void configureDisplaySelector();
    void configureModePane();
    void configureUndoRedoPane();
    void setControllerMode(NodeController::NodeControllerMode mode);

    ApplicationContext& applicationContext;

    ButtonPane           buttonPane;
    ItemSelector         displaySelector;
    TempoDisplay         tempoDisplay;
    std::unique_ptr<IconButton> playButton;
    std::unique_ptr<IconButton> resetButton;
    ButtonPane           undoRedoPane;
};
