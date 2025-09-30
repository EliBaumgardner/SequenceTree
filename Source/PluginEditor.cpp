/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ComponentContext.h"



SequenceTreeAudioProcessorEditor::SequenceTreeAudioProcessorEditor (SequenceTreeAudioProcessor& p)
: AudioProcessorEditor(p),audioProcessor (p)
{
    
    port = std::make_unique<DynamicPort>(&canvas);
    addAndMakeVisible(port.get());
    
    setResizable(true,true);
    
    canvas.setNodeMenu(&menu);
    //addAndMakeVisible(menu);
    
    addAndMakeVisible(selectionBar);
    addAndMakeVisible(titleBar);
    setSize (400, 300);
    
    audioProcessor.canvas = &canvas;
    
    titleBar.toggled = [this](){
        
        canvas.start = !canvas.start;
        canvas.setProcessorPlayblack(canvas.start);
        //canvas.updateProcessorGraph(canvas.root);
    };
    
    ComponentContext::processor = &p;
    ComponentContext::canvas = &canvas;
}

SequenceTreeAudioProcessorEditor::~SequenceTreeAudioProcessorEditor()
{

}


void SequenceTreeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::white);
    
}

void SequenceTreeAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    port.get()->setBounds(bounds);
    
    auto titleHeight = static_cast<int>(bounds.getHeight() * 0.1f);
    auto titleArea = bounds.removeFromTop(titleHeight);
    titleBar.setBounds(titleArea);

    auto selectionHeight = static_cast<int>(bounds.getHeight() * 0.1f);
    auto selectionBarArea = bounds.removeFromBottom(selectionHeight);
    selectionBar.setBounds(selectionBarArea);
    
    //int menuW = static_cast<int>(menuWidthRatio * static_cast<float>(getWidth()));
    //menu.setBounds(bounds.removeFromLeft(menuW)); // Menu on left, taking up width
}


void SequenceTreeAudioProcessorEditor::setManualMenuBounds (juce::Rectangle<int> b)
    {
        // compute ratio from the editor’s current width
//        if (getWidth() > 0)
//        {
//            menuWidthRatio = (float) b.getWidth() / (float) getWidth();
//            // clamp to a sensible range
//            menuWidthRatio = juce::jlimit (0.05f, 0.95f, menuWidthRatio);
//        }
    }

