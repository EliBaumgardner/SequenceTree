/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Util/PluginContext.h"


SequenceTreeAudioProcessorEditor::SequenceTreeAudioProcessorEditor (SequenceTreeAudioProcessor& p)
: AudioProcessorEditor(p),audioProcessor (p)
{
    ComponentContext::processor = &p;
    ComponentContext::lookAndFeel = &lookAndFeel;

    canvas       = std::make_unique<NodeCanvas>(); ComponentContext::canvas = canvas.get();
    titleBar     = std::make_unique<Titlebar>();
    port         = std::make_unique<DynamicPort>(canvas.get());
    
    titleBar->toggled = [this](){ canvas->start = !canvas->start; canvas->setProcessorPlayblack(canvas->start); };

    addAndMakeVisible(port.get());
    addAndMakeVisible(titleBar.get());

    setResizable(true,false);
    setSize (400, 300);
}

SequenceTreeAudioProcessorEditor::~SequenceTreeAudioProcessorEditor() {}


void SequenceTreeAudioProcessorEditor::paint (juce::Graphics& g) { g.fillAll(juce::Colours::white); }

void SequenceTreeAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    port.get()->setBounds(bounds);
    
    auto titleHeight = static_cast<int>(bounds.getHeight() * 0.05f);
    auto titleArea = bounds.removeFromTop(titleHeight);
    titleBar->setBounds(titleArea);;
}
