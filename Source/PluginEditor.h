/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "Util/PluginModules.h"

#include "PluginProcessor.h"
#include "Node/NodeCanvas.h"
#include "Node/Node.h"
#include "Logic/DynamicPort.h"
#include "UI/Titlebar.h"
#include "UI/CustomLookAndFeel.h"


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
    
    std::unique_ptr<NodeCanvas>   canvas       = nullptr;
    std::unique_ptr<Titlebar>     titleBar     = nullptr;
    std::unique_ptr<DynamicPort>  port         = nullptr;
    CustomLookAndFeel lookAndFeel;
    
    float menuWidthRatio = 0.25f;
    float menuHeightRatio = 0.25f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceTreeAudioProcessorEditor)
};
