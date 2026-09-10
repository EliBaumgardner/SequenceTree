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
    applicationContext.processor          = &p;
    applicationContext.undoManager        = &undoManager;
    applicationContext.lookAndFeel        = &lookAndFeel;
    applicationContext.graphState         = &p.graphState;
    applicationContext.traversalRuleState = &p.traversalRuleState;
    applicationContext.rtGraphBuilder     = &p.rtGraphBuilder;

    canvas = std::make_unique<NodeCanvas>(applicationContext);
    applicationContext.canvas = canvas.get();

    nodeController = std::make_unique<NodeController>(applicationContext, *canvas);
    applicationContext.nodeController = nodeController.get();

    port      = std::make_unique<DynamicPort>(canvas.get());
    menuArea  = std::make_unique<MenuArea>(applicationContext);
    titleBar  = std::make_unique<Titlebar>(applicationContext);
    bottomBar = std::make_unique<BottomBar>(applicationContext);

    titleBar->onDisplayModeChanged = [this](NodeDisplayMode mode) { bottomBar->applyDisplayMode(mode); };

    menuArea->resizer.onWidthDragged = [this](int newWidth) {
        int total = getWidth();
        if (total <= 0) return;
        menuAreaWidthRatio = juce::jlimit(0.01f, 0.9f, static_cast<float>(newWidth) / static_cast<float>(total));
        resized();
    };

    port->onZoomChanged = [canvasPtr = canvas.get()](float z) { canvasPtr->valueField.setViewZoom(z); };

    if (audioProcessor.pendingRestoreState.isValid()) {
        audioProcessor.applyRestoredState();
    }

    canvas->rebuildFromNodeMap(applicationContext.graphState->nodeMap);

    canvas->addMouseListener(nodeController.get(),true);

    attachStateListeners();


    addAndMakeVisible(port.get());
    addAndMakeVisible(menuArea.get());
    addAndMakeVisible(titleBar.get());
    addAndMakeVisible(bottomBar.get());

    setResizable(true,false);
    setSize (700, 500);
}

SequenceTreeAudioProcessorEditor::~SequenceTreeAudioProcessorEditor()
{
    if (keyListenerTarget != nullptr) {
        keyListenerTarget->removeKeyListener(this);
    }

    auto& desktop = juce::Desktop::getInstance();

    if (desktop.getKioskModeComponent() == getTopLevelComponent()) {
        desktop.setKioskModeComponent(nullptr);
    }

    detachStateListeners();
}

void SequenceTreeAudioProcessorEditor::attachStateListeners()
{
    applicationContext.graphState->nodeMap.addListener(&canvas->treeListener);
    applicationContext.graphState->traversals.map.addListener(&canvas->treeListener);
}

void SequenceTreeAudioProcessorEditor::detachStateListeners()
{
    applicationContext.graphState->nodeMap.removeListener(&canvas->treeListener);
    applicationContext.graphState->traversals.map.removeListener(&canvas->treeListener);
}


void SequenceTreeAudioProcessorEditor::paint (juce::Graphics& g) { g.fillAll(juce::Colours::white); }

void SequenceTreeAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    auto barHeight = static_cast<int>(bounds.getHeight() * 0.05f);
    auto menuAreaWidth = juce::jmax(MenuArea::minMenuWidth, static_cast<int>(bounds.getWidth() * menuAreaWidthRatio));

    auto menuAreaBounds   = bounds.removeFromLeft(menuAreaWidth);
    auto titleArea        = bounds.removeFromTop(barHeight);
    auto bottomArea       = bounds.removeFromBottom(barHeight);

    menuArea ->setBounds(menuAreaBounds);
    titleBar ->setBounds(titleArea);
    bottomBar->setBounds(bottomArea);
    port->setBounds(bounds);
}

void SequenceTreeAudioProcessorEditor::parentHierarchyChanged()
{
    auto* top = getTopLevelComponent();

    if (top == keyListenerTarget) {
        return;
    }

    if (keyListenerTarget != nullptr) {
        keyListenerTarget->removeKeyListener(this);
    }

    keyListenerTarget = top;

    if (keyListenerTarget != nullptr) {
        keyListenerTarget->addKeyListener(this);
    }
}

bool SequenceTreeAudioProcessorEditor::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    if (audioProcessor.wrapperType != juce::AudioProcessor::wrapperType_Standalone) {
        return false;
    }

    if (key.getModifiers().isShiftDown()
        && (key.getKeyCode() == '1' || key.getTextCharacter() == '!'))
    {
        toggleFullScreen();
        return true;
    }

    if (key == juce::KeyPress::escapeKey
        && juce::Desktop::getInstance().getKioskModeComponent() != nullptr)
    {
        toggleFullScreen();
        return true;
    }

    return false;
}

void SequenceTreeAudioProcessorEditor::toggleFullScreen()
{
    auto& desktop = juce::Desktop::getInstance();

    if (desktop.getKioskModeComponent() == nullptr) {
        desktop.setKioskModeComponent(getTopLevelComponent(), true);
    }
    else {
        desktop.setKioskModeComponent(nullptr);
    }
}
