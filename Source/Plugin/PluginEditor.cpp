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
    applicationContext.processor      = &p;
    applicationContext.undoManager    = &undoManager;
    applicationContext.lookAndFeel    = &lookAndFeel;
    applicationContext.valueTreeState = &p.graphState;

    canvas = std::make_unique<NodeCanvas>(applicationContext);
    applicationContext.canvas = canvas.get();

    applicationContext.rtGraphBuilder = &p.rtGraphBuilder;

    nodeController = std::make_unique<NodeController>(applicationContext);
    applicationContext.nodeController = nodeController.get();

    jassert(applicationContext.isComplete());

    port      = std::make_unique<DynamicPort>(canvas.get());
    menuArea  = std::make_unique<MenuArea>(applicationContext);
    titleBar  = std::make_unique<Titlebar>(applicationContext);
    bottomBar = std::make_unique<BottomBar>(applicationContext);

    menuArea->onWidthDragged = [this](int newWidth) {
        int total = getWidth();
        if (total <= 0) return;
        menuAreaWidthRatio = juce::jlimit(0.01f, 0.9f, static_cast<float>(newWidth) / static_cast<float>(total));
        resized();
    };

    port->onZoomChanged = [canvasPtr = canvas.get()](float z) { canvasPtr->valueField.setViewZoom(z); };

    audioProcessor.notifyUi = [canvasPtr = canvas.get()] {
        if (canvasPtr) {
            canvasPtr->triggerAsyncUpdate();
        }
    };

    audioProcessor.suspendStateListeners = [this] { detachStateListeners(); };

    audioProcessor.resumeStateListeners = [this] {
        canvas->setValueTreeState(applicationContext.valueTreeState->nodeMap);
        attachStateListeners();
    };

    if (audioProcessor.pendingRestoreState.isValid()) {
        audioProcessor.applyRestoredState();
    }
    else if (applicationContext.valueTreeState->nodeMap.getNumChildren() > 0) {
        canvas->setValueTreeState(applicationContext.valueTreeState->nodeMap);
    }

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
    if (keyListenerTarget != nullptr)
        keyListenerTarget->removeKeyListener(this);

    auto& desktop = juce::Desktop::getInstance();
    if (desktop.getKioskModeComponent() == getTopLevelComponent())
        desktop.setKioskModeComponent(nullptr);

    audioProcessor.notifyUi              = nullptr;
    audioProcessor.suspendStateListeners = nullptr;
    audioProcessor.resumeStateListeners  = nullptr;

    detachStateListeners();
}

void SequenceTreeAudioProcessorEditor::attachStateListeners()
{
    applicationContext.valueTreeState->canvasData.addListener(&canvas->treeListener);
    applicationContext.valueTreeState->nodeMap.addListener(&canvas->treeListener);
    applicationContext.valueTreeState->nodeTreeMap.addListener(&canvas->treeListener);
    applicationContext.valueTreeState->traversalMap.addListener(&canvas->treeListener);
}

void SequenceTreeAudioProcessorEditor::detachStateListeners()
{
    applicationContext.valueTreeState->canvasData.removeListener(&canvas->treeListener);
    applicationContext.valueTreeState->nodeMap.removeListener(&canvas->treeListener);
    applicationContext.valueTreeState->nodeTreeMap.removeListener(&canvas->treeListener);
    applicationContext.valueTreeState->traversalMap.removeListener(&canvas->treeListener);
}


void SequenceTreeAudioProcessorEditor::paint (juce::Graphics& g) { g.fillAll(juce::Colours::white); }

void SequenceTreeAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    auto barHeight = static_cast<int>(bounds.getHeight() * 0.05f);
    auto menuAreaWidth = juce::jmax(MenuArea::resizerWidth, static_cast<int>(bounds.getWidth() * menuAreaWidthRatio));

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
    if (top == keyListenerTarget)
        return;

    if (keyListenerTarget != nullptr)
        keyListenerTarget->removeKeyListener(this);

    keyListenerTarget = top;

    if (keyListenerTarget != nullptr)
        keyListenerTarget->addKeyListener(this);
}

bool SequenceTreeAudioProcessorEditor::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    if (audioProcessor.wrapperType != juce::AudioProcessor::wrapperType_Standalone)
        return false;

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

    if (desktop.getKioskModeComponent() == nullptr)
        desktop.setKioskModeComponent(getTopLevelComponent(), true);
    else
        desktop.setKioskModeComponent(nullptr);
}
