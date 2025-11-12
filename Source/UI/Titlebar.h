/*
  ==============================================================================

    TitleBar.h
    Created: 12 Jun 2025 6:50:43pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include "CustomLookAndFeel.h"
#include "DynamicEditor.h"
#include "../Util/PluginModules.h"
#include "../Util/PluginContext.h"
#include "TitleBarButtons.h"

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
    PlayButton playButton;
};
