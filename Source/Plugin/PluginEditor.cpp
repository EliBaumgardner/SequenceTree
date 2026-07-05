/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../Graph/ValueTreeIdentifiers.h"


SequenceTreeAudioProcessorEditor::SequenceTreeAudioProcessorEditor (SequenceTreeAudioProcessor& p)
: AudioProcessorEditor(p), audioProcessor(p)
{
    applicationContext.processor   = &p;
    applicationContext.undoManager = &undoManager;
    applicationContext.lookAndFeel = &lookAndFeel;

    canvas         = std::make_unique<NodeCanvas>(applicationContext);
    rtGraphBuilder = std::make_unique<RTGraphBuilder>(applicationContext, *canvas);
    nodeController = std::make_unique<NodeController>(applicationContext);
    valueTreeState = std::make_unique<ValueTreeState>();
    port           = std::make_unique<DynamicPort>(canvas.get());
    traversalMenu  = std::make_unique<TraversalMenu>(applicationContext);

    traversalMenu->onWidthDragged = [this](int newWidth) {
        int total = getWidth();
        if (total <= 0) return;
        menuWidthRatio = juce::jlimit(0.05f, 0.9f, static_cast<float>(newWidth) / static_cast<float>(total));
        resized();
    };

    applicationContext.canvas            = canvas.get();
    port->onZoomChanged = [canvasPtr = canvas.get()](float z) { canvasPtr->setViewZoom(z); };

    audioProcessor.notifyUi = [canvasPtr = canvas.get()] {
        if (canvasPtr) {
            canvasPtr->triggerAsyncUpdate();
        }
    };

    audioProcessor.applyStateToUi = [canvasPtr = canvas.get()](juce::ValueTree restoredTree) {
        juce::MessageManager::callAsync([canvasPtr, restoredTree]() {
            ValueTreeState::nodeMap.removeListener(&canvasPtr->treeListener);
            ValueTreeState::nodeMap.removeAllChildren(nullptr);

            for (int i = 0; i < restoredTree.getNumChildren(); ++i)
                ValueTreeState::nodeMap.addChild(restoredTree.getChild(i).createCopy(), -1, nullptr);

            int maxId = 0;
            for (int i = 0; i < ValueTreeState::nodeMap.getNumChildren(); ++i) {
                int id = ValueTreeState::nodeMap.getChild(i).getProperty(ValueTreeIdentifiers::Id);
                if (id > maxId) {
                    maxId = id;
                }
            }
            ValueTreeState::nodeIdIncrement = maxId;

            canvasPtr->setValueTreeState(ValueTreeState::nodeMap);
            ValueTreeState::nodeMap.addListener(&canvasPtr->treeListener);
        });
    };

    applicationContext.rtGraphBuilder    = rtGraphBuilder.get();
    applicationContext.valueTreeState    = valueTreeState.get();
    applicationContext.nodeController    = nodeController.get();


    titleBar  = std::make_unique<Titlebar>(applicationContext);
    bottomBar = std::make_unique<BottomBar>(applicationContext);

    canvas->addMouseListener(nodeController.get(),true);

    valueTreeState.get()->canvasData.addListener(&canvas->treeListener);
    valueTreeState.get()->nodeMap.addListener(&canvas->treeListener);
    valueTreeState.get()->nodeTreeMap.addListener(&canvas->treeListener);
    valueTreeState.get()->traversalMap.addListener(&canvas->treeListener);


    addAndMakeVisible(port.get());
    addAndMakeVisible(titleBar.get());
    addAndMakeVisible(bottomBar.get());
    addAndMakeVisible(traversalMenu.get());

    setResizable(true,false);
    setSize (700, 500);
}

SequenceTreeAudioProcessorEditor::~SequenceTreeAudioProcessorEditor()
{
    audioProcessor.notifyUi       = nullptr;
    audioProcessor.applyStateToUi = nullptr;

    ValueTreeState::canvasData.removeListener(&canvas->treeListener);
    ValueTreeState::nodeMap.removeListener(&canvas->treeListener);
    ValueTreeState::nodeTreeMap.removeListener(&canvas->treeListener);
}


void SequenceTreeAudioProcessorEditor::paint (juce::Graphics& g) { g.fillAll(juce::Colours::white); }

void SequenceTreeAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    auto barHeight = static_cast<int>(bounds.getHeight() * 0.05f);
    auto menuWidth = static_cast<int>(bounds.getWidth() * menuWidthRatio);

    auto traversalMenuArea= bounds.removeFromRight(menuWidth);
    auto titleArea        = bounds.removeFromTop(barHeight);
    auto bottomArea       = bounds.removeFromBottom(barHeight);

    traversalMenu->setBounds(traversalMenuArea);
    titleBar ->setBounds(titleArea);
    bottomBar->setBounds(bottomArea);
    port->setBounds(bounds);
}
