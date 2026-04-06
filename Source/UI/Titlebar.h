/*
  ==============================================================================

    TitleBar.h
    Created: 12 Jun 2025 6:50:43pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once


#include "../Util/PluginModules.h"
#include "Buttons/PlayButton.h"
#include "Buttons/ResetButton.h"
#include "Buttons/ButtonPane.h"
#include "Buttons/DisplayMenu.h"
#include "Buttons/TempoDisplay.h"
#include "Buttons/UndoRedoPane.h"

class Titlebar : public juce::Component {
    
public:
    
    Titlebar();

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void()> toggled;

private:

    ButtonPane buttonPane;
    DisplayMenu displaySelector;

    TempoDisplay tempoDisplay;
    PlayButton   playButton;
    ResetButton  resetButton;

    UndoRedoPane undoRedoPane;
};
