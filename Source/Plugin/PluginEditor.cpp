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
    juce::PropertiesFile::Options interfaceSettingsOptions;

    interfaceSettingsOptions.applicationName     = "Interface";
    interfaceSettingsOptions.filenameSuffix      = ".settings";
    interfaceSettingsOptions.folderName          = JucePlugin_Name;
    interfaceSettingsOptions.osxLibrarySubFolder = "Application Support";

    interfaceSettings = std::make_unique<juce::PropertiesFile>(interfaceSettingsOptions);

    if (interfaceSettings->containsKey(ValueTreeIdentifiers::ThemeColour.toString())) {
        lookAndFeel.applyThemeColour(juce::Colour::fromString(interfaceSettings->getValue(ValueTreeIdentifiers::ThemeColour.toString())));
    }

    setLookAndFeel(&lookAndFeel);

    tooltipWindow.setOpaque(false);

    nodeCanvas     = std::make_unique<NodeCanvas>(p, p.graphState, p.rtGraphBuilder, p.undoManager, lookAndFeel);
    nodeController = std::make_unique<NodeController>(*nodeCanvas, p.graphState, p.undoManager, p.traversalSession, p.rtGraphBuilder);
    port           = std::make_unique<DynamicPort>(*nodeCanvas);
    menuArea       = std::make_unique<MenuArea>(*nodeCanvas, p.graphState, p.traversalRuleState, p.snapshots, *interfaceSettings, p.colourPresets, p.undoManager);
    titleBar       = std::make_unique<Titlebar>(p, *nodeCanvas, *nodeController, p.undoManager);
    bottomBar      = std::make_unique<BottomBar>(*nodeCanvas, p.traversalSession, p.colourPresets, p.undoManager);

    titleBar->onDisplayModeChanged = [this](NodeDisplayMode mode) { bottomBar->applyDisplayMode(mode); };

    menuArea->resizer.onWidthDragged = [this](int newWidth) {
        int total = getWidth();
        if (total <= 0) return;
        menuAreaWidthRatio = juce::jlimit(0.01f, 0.9f, static_cast<float>(newWidth) / static_cast<float>(total));
        resized();

        repaint();
    };

    if (audioProcessor.pendingRestoreState.isValid()) {
        audioProcessor.applyRestoredState();
    }

    nodeCanvas->rebuildFromNodeMap(audioProcessor.graphState.nodeMap);

    titleBar->applyPlaybackState(audioProcessor.isPlaying.load());

    nodeCanvas->addMouseListener(nodeController.get(), true);

    audioProcessor.graphState.nodeMap.addListener(&nodeCanvas->treeListener);

    addAndMakeVisible(port.get());
    addAndMakeVisible(menuArea.get());
    addAndMakeVisible(titleBar.get());
    addAndMakeVisible(bottomBar.get());

    setResizable(true,false);
    setResizeLimits(minimumWindowSide, minimumWindowSide, maximumWindowSide, maximumWindowSide);
    setSize (700, 500);

    audioCommandFrames = juce::VBlankAttachment(this, [this](double) {
        lookAndFeel.frostedBackdropStale = true;

        if (audioProcessor.playbackStateChanged.exchange(false)) {
            titleBar->applyPlaybackState(audioProcessor.isPlaying.load());
        }

        if (audioProcessor.eventManager.bridge.hasPendingCommands() || !nodeCanvas->asyncUpdates.empty()) {
            nodeCanvas->handleAsyncUpdate();
        }
    });
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

    audioProcessor.graphState.nodeMap.removeListener(&nodeCanvas->treeListener);

    setLookAndFeel(nullptr);
}

void SequenceTreeAudioProcessorEditor::paint (juce::Graphics& g) { g.fillAll(lookAndFeel.windowColour); }

void SequenceTreeAudioProcessorEditor::resized()
{
    const auto windowArea = getLocalBounds().reduced(Theme::windowGutter);
    auto       bounds     = windowArea;

    auto barHeight = static_cast<int>(bounds.getHeight() * Theme::barHeightRatio);
    auto minMenuWidth = juce::roundToInt(barHeight * (MenuArea::menuBarWidthRatio + MenuArea::resizerWidthRatio));
    auto menuAreaWidth = juce::jmax(minMenuWidth, static_cast<int>(bounds.getWidth() * menuAreaWidthRatio));

    auto menuAreaBounds   = bounds.removeFromLeft(menuAreaWidth + Theme::windowGutter).withTrimmedRight(Theme::windowGutter);
    auto titleArea        = bounds.removeFromTop(barHeight + Theme::windowGutter).withTrimmedBottom(Theme::windowGutter);
    auto bottomArea       = bounds.removeFromBottom(barHeight + Theme::windowGutter).withTrimmedTop(Theme::windowGutter);

    canvasFrame = bounds;

    lookAndFeel.textHeight = juce::jmin(barHeight * Theme::textHeightRatio, bounds.getWidth() * Theme::textWidthRatio);

    menuArea ->setBounds(menuAreaBounds);
    titleBar ->setBounds(titleArea);
    bottomBar->setBounds(bottomArea);
    port->setBounds(windowArea);

    lookAndFeel.unfrostedArea = nodeCanvas->getLocalArea(this, canvasFrame);
}

void SequenceTreeAudioProcessorEditor::paintOverChildren (juce::Graphics& graphics)
{
    const auto frame = canvasFrame.toFloat().reduced(Theme::borderThickness * 0.5f);
    juce::Path surround;

    surround.addRectangle(port->getBounds().toFloat());
    surround.addRoundedRectangle(frame, Theme::canvasCornerRadius);
    surround.setUsingNonZeroWinding(false);

    graphics.saveState();

    graphics.excludeClipRegion(menuArea->getBounds());
    graphics.excludeClipRegion(titleBar->getBounds());
    graphics.excludeClipRegion(bottomBar->getBounds());
    graphics.reduceClipRegion(surround);

    if (! graphics.isClipEmpty()) {
        lookAndFeel.drawFrostedGlass(graphics, *this, *nodeCanvas, graphics.getClipBounds());
    }

    graphics.restoreState();

    graphics.setColour(lookAndFeel.borderColour);
    graphics.drawRoundedRectangle(frame, Theme::canvasCornerRadius, Theme::borderThickness);
}

bool SequenceTreeAudioProcessorEditor::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    const bool isStandalone = audioProcessor.wrapperType == juce::AudioProcessor::wrapperType_Standalone;

    if (isStandalone
        && key.getModifiers().isShiftDown()
        && (key.getKeyCode() == '1' || key.getTextCharacter() == '!'))
    {
        toggleFullScreen();
        return true;
    }

    if (isStandalone
        && key == juce::KeyPress::escapeKey
        && juce::Desktop::getInstance().getKioskModeComponent() != nullptr)
    {
        toggleFullScreen();
        return true;
    }

    const juce::ModifierKeys command      = juce::ModifierKeys::commandModifier;
    const juce::ModifierKeys commandShift = juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier;

    SelectionOps&      selectionOps = nodeController->selectionOps;
    juce::UndoManager& undoManager  = audioProcessor.undoManager;

    if (key == juce::KeyPress::spaceKey) {
        titleBar->applyPlaybackState(!nodeCanvas->start);
        return true;
    }

    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) {
        selectionOps.deleteSelection();
        return true;
    }

    if (key == juce::KeyPress('z', command, 0)) {
        undoManager.undo();
        return true;
    }

    if (key == juce::KeyPress('z', commandShift, 0) || key == juce::KeyPress('y', command, 0)) {
        undoManager.redo();
        return true;
    }

    if (key == juce::KeyPress('c', command, 0)) {
        selectionOps.copySelection();
        return true;
    }

    if (key == juce::KeyPress('v', command, 0)) {
        juce::Point<float> pastePoint = nodeCanvas->getLocalPoint(port.get(), port->getLocalBounds().getCentre()).toFloat();

        if (port->getLocalBounds().contains(port->getMouseXYRelative())) {
            pastePoint = nodeCanvas->getMouseXYRelative().toFloat();
        }

        selectionOps.pasteAt(pastePoint.transformedBy(nodeCanvas->modelTransform).roundToInt());
        return true;
    }

    if (key == juce::KeyPress('a', command, 0)) {
        selectionOps.selectAll();
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
