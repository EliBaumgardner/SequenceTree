/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <ArsNovaUI/Theme/InterfaceIdentifiers.h>

SequenceTreeAudioProcessorEditor::SequenceTreeAudioProcessorEditor (SequenceTreeAudioProcessor& p)
: AudioProcessorEditor(p), audioProcessor(p)
{
    juce::PropertiesFile::Options interfaceSettingsOptions;

    interfaceSettingsOptions.applicationName     = "Interface";
    interfaceSettingsOptions.filenameSuffix      = ".settings";
    interfaceSettingsOptions.folderName          = JucePlugin_Name;
    interfaceSettingsOptions.osxLibrarySubFolder = "Application Support";

    interfaceSettings = std::make_unique<juce::PropertiesFile>(interfaceSettingsOptions);

    if (interfaceSettings->containsKey(InterfaceIdentifiers::ThemeColour.toString())) {
        lookAndFeel.applyThemeColour(juce::Colour::fromString(interfaceSettings->getValue(InterfaceIdentifiers::ThemeColour.toString())));
    }

    setLookAndFeel(&lookAndFeel);

    tooltipWindow.setOpaque(false);

    scaledView     = std::make_unique<ScaledView>(workspace, *interfaceSettings);
    nodeCanvas     = std::make_unique<NodeCanvas>(p, p.graphState, p.rtGraphBuilder, p.undoManager, lookAndFeel);
    nodeController = std::make_unique<NodeController>(*nodeCanvas, p.graphState, p.undoManager, p.traversalSession, p.rtGraphBuilder, p.ornamentLibraries);
    port           = std::make_unique<DynamicPort>(*nodeCanvas);
    menuArea       = std::make_unique<MenuArea>(*nodeCanvas, p.graphState, p.traversalRuleState, p.snapshots, *interfaceSettings, *scaledView, p.colourPresets, p.undoManager);
    titleBar       = std::make_unique<Titlebar>(p, *nodeCanvas, *nodeController, p.undoManager);
    bottomBar      = std::make_unique<BottomBar>(*nodeCanvas, p.traversalSession, p.colourPresets, p.undoManager, p.ornamentLibraries, p.ornamentUndoManager, nodeController->ornamentApplier);
    canvasFrame    = std::make_unique<CanvasFrame>(*nodeCanvas, *menuArea, *titleBar, *bottomBar);

    titleBar->onDisplayModeChanged = [this](NodeDisplayMode mode) { bottomBar->applyDisplayMode(mode); };

    port->onTransformChanged = [this](const juce::AffineTransform& viewTransform, float zoom) { nodeCanvas->applyViewTransform(viewTransform, zoom); };

    menuArea->resizer.onWidthDragged = [this](int newWidth) {
        menuWidth = newWidth;

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

    workspace.addAndMakeVisible(port.get());
    workspace.addAndMakeVisible(canvasFrame.get());
    workspace.addAndMakeVisible(menuArea.get());
    workspace.addAndMakeVisible(titleBar.get());
    workspace.addAndMakeVisible(bottomBar.get());

    configureWindowSize();

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
    scaledView->setBounds(getLocalBounds());

    const auto windowArea    = workspace.getLocalBounds().reduced(Theme::windowGutter);
    auto       bounds        = windowArea;
    const int  barHeight     = Theme::barHeight;
    const int  barsWidth     = juce::jmax(titleBar->narrowestWidth(), bottomBar->narrowestWidth());
    const int  panelSpacing  = juce::roundToInt(barHeight * Theme::menuSpacingRatio);
    const int  minMenuWidth  = juce::roundToInt(barHeight * (MenuArea::menuBarWidthRatio + MenuArea::resizerWidthRatio));
    const int  fullMenuWidth = juce::roundToInt(barHeight * MenuArea::menuBarWidthRatio) + juce::roundToInt(barHeight * MenuArea::resizerWidthRatio)
                               + Theme::menuContentWidth + panelSpacing * 2;
    const int  maxMenuWidth  = juce::jmin(fullMenuWidth, juce::jmax(minMenuWidth, bounds.getWidth() - Theme::windowGutter - barsWidth));

    menuWidth = juce::jlimit(minMenuWidth, maxMenuWidth, menuWidth);

    auto menuAreaBounds   = bounds.removeFromLeft(menuWidth + Theme::windowGutter).withTrimmedRight(Theme::windowGutter);
    auto titleArea        = bounds.removeFromTop(barHeight + Theme::windowGutter).withTrimmedBottom(Theme::windowGutter);
    auto bottomArea       = bounds.removeFromBottom(barHeight + Theme::windowGutter).withTrimmedTop(Theme::windowGutter);

    canvasFrame->canvasArea = bounds - windowArea.getPosition();

    menuArea   ->setBounds(menuAreaBounds);
    titleBar   ->setBounds(titleArea);
    bottomBar  ->setBounds(bottomArea);
    port       ->setBounds(windowArea);
    canvasFrame->setBounds(windowArea);

    lookAndFeel.unfrostedArea = nodeCanvas->getLocalArea(&workspace, bounds);
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

void SequenceTreeAudioProcessorEditor::configureWindowSize()
{
    const int minMenuWidth = juce::roundToInt(Theme::barHeight * (MenuArea::menuBarWidthRatio + MenuArea::resizerWidthRatio));

    scaledView->onZoomChanged = [this]() {
        const float                          zoom        = scaledView->zoom;
        const float                          zoomRatio   = scaledView->previousZoom / zoom;
        const juce::Point<int>               minimumSize = scaledView->minimumContentSize;
        juce::Component* const               window      = getTopLevelComponent();
        const juce::Displays::Display* const display     = juce::Desktop::getInstance().getDisplays().getDisplayForRect(window->getScreenBounds());

        menuWidth = juce::roundToInt(static_cast<float>(menuWidth) * zoomRatio);

        port->setZoom(port->zoom * zoomRatio, -port->getPosition().toFloat());

        setResizeLimits(juce::roundToInt(minimumSize.x * zoom), juce::roundToInt(minimumSize.y * zoom), maximumWindowSide, maximumWindowSide);

        resized();

        if (audioProcessor.wrapperType == juce::AudioProcessor::wrapperType_Standalone && display != nullptr) {
            window->setBounds(window->getBounds().constrainedWithin(display->userArea));
        }
    };

    addAndMakeVisible(*scaledView);

    setResizable(true, false);
    setSize(seedWidth, seedHeight);

    const int barsWidth = juce::jmax(titleBar->narrowestWidth(), bottomBar->narrowestWidth());

    scaledView->minimumContentSize = { Theme::windowGutter * 3 + minMenuWidth + barsWidth, Theme::windowGutter * 2 + menuArea->narrowestHeight() };

    scaledView->onZoomChanged();

    setSize(juce::roundToInt(getConstrainer()->getMinimumWidth() * openingScale), juce::roundToInt(getConstrainer()->getMinimumHeight() * openingScale));
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
