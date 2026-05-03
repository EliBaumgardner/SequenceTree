/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/
#include "PluginProcessor.h"
#include "PluginEditor.h"


SequenceTreeAudioProcessorEditor::SequenceTreeAudioProcessorEditor (SequenceTreeAudioProcessor& p)
: AudioProcessorEditor(p), audioProcessor(p)
{
    if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
        window->setFullScreen (true);

    applicationContext.processor   = &p;
    applicationContext.undoManager = &undoManager;
    applicationContext.lookAndFeel = &lookAndFeel;

    canvas         = std::make_unique<NodeCanvas>(applicationContext);
    rtGraphBuilder = std::make_unique<RTGraphBuilder>(applicationContext, *canvas);
    nodeController = std::make_unique<NodeController>(applicationContext);
    valueTreeState = std::make_unique<ValueTreeState>();
    port           = std::make_unique<DynamicPort>(canvas.get());

    applicationContext.processor->canvas = canvas.get();
    applicationContext.canvas            = canvas.get();
    applicationContext.rtGraphBuilder    = rtGraphBuilder.get();
    applicationContext.valueTreeState    = valueTreeState.get();
    applicationContext.nodeController    = nodeController.get();

    titleBar = std::make_unique<Titlebar>(applicationContext);

    canvas->addMouseListener(nodeController.get(),true);

    valueTreeState.get()->canvasData.addListener(&canvas->treeListener);
    valueTreeState.get()->nodeMap.addListener(&canvas->treeListener);
    valueTreeState.get()->nodeTreeMap.addListener(&canvas->treeListener);


    addAndMakeVisible(port.get());
    addAndMakeVisible(titleBar.get());

    setResizable(true,false);
    setSize (700, 500);
}

SequenceTreeAudioProcessorEditor::~SequenceTreeAudioProcessorEditor()
{
    ValueTreeState::canvasData.removeListener(&canvas->treeListener);
    ValueTreeState::nodeMap.removeListener(&canvas->treeListener);
    ValueTreeState::nodeTreeMap.removeListener(&canvas->treeListener);
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
