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

    canvas         = std::make_unique<NodeCanvas>(applicationContext);
    rtGraphBuilder = std::make_unique<RTGraphBuilder>(applicationContext, *canvas);
    nodeController = std::make_unique<NodeController>(applicationContext);
    port           = std::make_unique<DynamicPort>(canvas.get());
    menuArea       = std::make_unique<MenuArea>(applicationContext);

    menuArea->onWidthDragged = [this](int newWidth) {
        int total = getWidth();
        if (total <= 0) return;
        menuAreaWidthRatio = juce::jlimit(0.01f, 0.9f, static_cast<float>(newWidth) / static_cast<float>(total));
        resized();
    };

    applicationContext.canvas            = canvas.get();
    port->onZoomChanged = [canvasPtr = canvas.get()](float z) { canvasPtr->setViewZoom(z); };

    audioProcessor.notifyUi = [canvasPtr = canvas.get()] {
        if (canvasPtr) {
            canvasPtr->triggerAsyncUpdate();
        }
    };

    audioProcessor.applyStateToUi =
        [safeCanvas = juce::Component::SafePointer<NodeCanvas>(canvas.get()),
         proc = &audioProcessor,
         state = &p.graphState](juce::ValueTree restoredTree) {
        juce::MessageManager::callAsync([safeCanvas, proc, state, restoredTree]() {
            NodeCanvas* canvasPtr = safeCanvas.getComponent();
            if (canvasPtr == nullptr) {
                return;
            }

            juce::ValueTree restoredNodeMap;
            juce::ValueTree restoredTraversalMap;

            if (restoredTree.getType() == ValueTreeIdentifiers::PluginState) {
                restoredNodeMap      = restoredTree.getChildWithName(ValueTreeIdentifiers::NodeMap);
                restoredTraversalMap = restoredTree.getChildWithName(ValueTreeIdentifiers::TraversalMap);
            }
            else {
                restoredNodeMap = restoredTree;
            }

            state->nodeMap.removeListener(&canvasPtr->treeListener);
            state->traversalMap.removeListener(&canvasPtr->treeListener);

            state->traversalMap.removeAllChildren(nullptr);
            for (int i = 0; i < restoredTraversalMap.getNumChildren(); ++i)
                state->traversalMap.addChild(restoredTraversalMap.getChild(i).createCopy(), -1, nullptr);

            state->nodeMap.removeAllChildren(nullptr);
            for (int i = 0; i < restoredNodeMap.getNumChildren(); ++i)
                state->nodeMap.addChild(restoredNodeMap.getChild(i).createCopy(), -1, nullptr);

            int maxId = 0;
            for (int i = 0; i < state->nodeMap.getNumChildren(); ++i) {
                int id = state->nodeMap.getChild(i).getProperty(ValueTreeIdentifiers::Id);
                if (id > maxId) {
                    maxId = id;
                }
            }
            state->setNodeIdIncrement(maxId);

            canvasPtr->setValueTreeState(state->nodeMap);
            state->nodeMap.addListener(&canvasPtr->treeListener);
            state->traversalMap.addListener(&canvasPtr->treeListener);

            proc->pendingRestoreState = juce::ValueTree();
        });
    };

    if (audioProcessor.pendingRestoreState.isValid()) {
        audioProcessor.applyStateToUi(audioProcessor.pendingRestoreState);
    }
    else if (applicationContext.valueTreeState->nodeMap.getNumChildren() > 0) {
        canvas->setValueTreeState(applicationContext.valueTreeState->nodeMap);
    }

    applicationContext.rtGraphBuilder = rtGraphBuilder.get();
    applicationContext.nodeController = nodeController.get();


    titleBar  = std::make_unique<Titlebar>(applicationContext);
    bottomBar = std::make_unique<BottomBar>(applicationContext);

    canvas->addMouseListener(nodeController.get(),true);

    applicationContext.valueTreeState->canvasData.addListener(&canvas->treeListener);
    applicationContext.valueTreeState->nodeMap.addListener(&canvas->treeListener);
    applicationContext.valueTreeState->nodeTreeMap.addListener(&canvas->treeListener);
    applicationContext.valueTreeState->traversalMap.addListener(&canvas->treeListener);


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

    audioProcessor.notifyUi       = nullptr;
    audioProcessor.applyStateToUi = nullptr;

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
