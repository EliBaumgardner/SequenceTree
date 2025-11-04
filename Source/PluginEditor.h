/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "Util/ProjectModules.h"

#include "PluginProcessor.h"
#include "Node/NodeCanvas.h"
#include "Node/Node.h"
#include "Logic/DynamicPort.h"
#include "UI/TitleBar.h"
#include "UI/SelectionBar.h"


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
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SequenceTreeAudioProcessor& audioProcessor;
    
    NodeCanvas canvas;

    std::unique_ptr<DynamicPort> port;
    
    TitleBar titleBar;
    SelectionBar selectionBar;
    
    float menuWidthRatio = 0.25f;
    float menuHeightRatio = 0.25f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceTreeAudioProcessorEditor)
};
