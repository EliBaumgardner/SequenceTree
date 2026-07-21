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
         proc = &audioProcessor](juce::ValueTree restoredTree) {
        juce::MessageManager::callAsync([safeCanvas, proc, restoredTree]() {
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

            ValueTreeState::nodeMap.removeListener(&canvasPtr->treeListener);
            ValueTreeState::traversalMap.removeListener(&canvasPtr->treeListener);

            ValueTreeState::traversalMap.removeAllChildren(nullptr);
            for (int i = 0; i < restoredTraversalMap.getNumChildren(); ++i)
                ValueTreeState::traversalMap.addChild(restoredTraversalMap.getChild(i).createCopy(), -1, nullptr);

            ValueTreeState::nodeMap.removeAllChildren(nullptr);
            for (int i = 0; i < restoredNodeMap.getNumChildren(); ++i)
                ValueTreeState::nodeMap.addChild(restoredNodeMap.getChild(i).createCopy(), -1, nullptr);

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
            ValueTreeState::traversalMap.addListener(&canvasPtr->treeListener);

            proc->pendingRestoreState = juce::ValueTree();
        });
    };

    if (audioProcessor.pendingRestoreState.isValid()) {
        audioProcessor.applyStateToUi(audioProcessor.pendingRestoreState);
    }
    else if (ValueTreeState::nodeMap.getNumChildren() > 0) {
        canvas->setValueTreeState(ValueTreeState::nodeMap);
    }

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
    addAndMakeVisible(menuArea.get());

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

    ValueTreeState::canvasData.removeListener(&canvas->treeListener);
    ValueTreeState::nodeMap.removeListener(&canvas->treeListener);
    ValueTreeState::nodeTreeMap.removeListener(&canvas->treeListener);
    ValueTreeState::traversalMap.removeListener(&canvas->treeListener);
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
