/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../Util/PluginContext.h"


SequenceTreeAudioProcessorEditor::SequenceTreeAudioProcessorEditor (SequenceTreeAudioProcessor& p)
: AudioProcessorEditor(p),audioProcessor (p)
{
    ComponentContext::processor = &p;
    ComponentContext::undoManager = &undoManager;
    ComponentContext::lookAndFeel = &lookAndFeel;

    canvas         = std::make_unique<NodeCanvas>();
    nodeController = std::make_unique<NodeController>();
    valueTreeState = std::make_unique<ValueTreeState>();
    port           = std::make_unique<DynamicPort>(canvas.get());

    ComponentContext::processor->canvas = canvas.get();
    ComponentContext::canvas = canvas.get();
    ComponentContext::valueTreeState    = valueTreeState.get();
    ComponentContext::nodeController   = nodeController.get();

    titleBar       = std::make_unique<Titlebar>();

    canvas->addMouseListener(nodeController.get(),true);

    valueTreeState.get()->canvasData.addListener(canvas.get());
    valueTreeState.get()->nodeMap.addListener(canvas.get());
    valueTreeState.get()->nodeTreeMap.addListener(canvas.get());


    addAndMakeVisible(port.get());
    addAndMakeVisible(titleBar.get());

    setResizable(true,false);
    setSize (700, 500);
}

SequenceTreeAudioProcessorEditor::~SequenceTreeAudioProcessorEditor()
{
    // Remove canvas as a ValueTree listener before it is destroyed.
    // The static ValueTree members survive the editor lifetime, so without
    // this, the next editor construction reassigns them and fires listener
    // callbacks on the already-deleted canvas (dangling pointer → crash).
    ValueTreeState::canvasData.removeListener(canvas.get());
    ValueTreeState::nodeMap.removeListener(canvas.get());
    ValueTreeState::nodeTreeMap.removeListener(canvas.get());
}


void SequenceTreeAudioProcessorEditor::paint (juce::Graphics& g) { g.fillAll(juce::Colours::white); }

void SequenceTreeAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    port.get()->setBounds(bounds);
    
    auto titleHeight = static_cast<int>(bounds.getHeight() * 0.05f);
    auto titleArea = bounds.removeFromTop(titleHeight);
    titleBar->setBounds(titleArea);;
}
