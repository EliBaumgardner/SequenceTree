/*
  ==============================================================================

    TitleBar.h
    Created: 12 Jun 2025 6:50:43pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once


#include "../Util/PluginModules.h"
#include "../Util/ApplicationContext.h"
#include "Buttons/PlayButton.h"
#include "Buttons/ResetButton.h"
#include "Buttons/ButtonPane.h"
#include "Buttons/DisplayMenu.h"
#include "Buttons/TempoDisplay.h"
#include "Buttons/UndoRedoPane.h"
#include "Buttons/ColorIntensityControl.h"

class Titlebar : public juce::Component {

public:

    Titlebar(ApplicationContext& context);

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void()> toggled;

private:

    ApplicationContext& applicationContext;

    ButtonPane           buttonPane;
    DisplayMenu          displaySelector;
    TempoDisplay         tempoDisplay;
    ColorIntensityControl colorIntensityControl;
    PlayButton           playButton;
    ResetButton          resetButton;
    UndoRedoPane         undoRedoPane;
};
