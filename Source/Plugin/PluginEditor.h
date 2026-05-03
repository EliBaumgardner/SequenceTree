/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../Util/PluginModules.h"

#include "PluginProcessor.h"
#include "../UI/Canvas/NodeCanvas.h"
#include "../UI/Node/Node.h"
#include "../Graph/RTGraphBuilder.h"
#include "../UI/Canvas/DynamicPort.h"
#include "../UI/Titlebar.h"
#include "../UI/Theme/CustomLookAndFeel.h"
#include "../Graph/ValueTreeState.h"
#include "../Util/ApplicationContext.h"


//==============================================================================
/**
*/
class SequenceTreeAudioProcessor;

class SequenceTreeAudioProcessorEditor  : public juce::AudioProcessorEditor
{

public:

    SequenceTreeAudioProcessorEditor (SequenceTreeAudioProcessor&);
    ~SequenceTreeAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void setManualMenuBounds (juce::Rectangle<int> b);

private:
    SequenceTreeAudioProcessor& audioProcessor;
    juce::UndoManager undoManager;
    CustomLookAndFeel lookAndFeel;
    ApplicationContext applicationContext;
    juce::TooltipWindow tooltipWindow { this, 400 };

    std::unique_ptr<NodeCanvas>     canvas         = nullptr;
    std::unique_ptr<RTGraphBuilder> rtGraphBuilder = nullptr;
    std::unique_ptr<NodeController> nodeController   = nullptr;
    std::unique_ptr<ValueTreeState> valueTreeState = nullptr;
    std::unique_ptr<Titlebar>       titleBar       = nullptr;
    std::unique_ptr<DynamicPort>    port           = nullptr;
    
    float menuWidthRatio = 0.25f;
    float menuHeightRatio = 0.25f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceTreeAudioProcessorEditor)
};
