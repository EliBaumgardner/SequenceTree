#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <ArsNovaUI/Theme/InterfaceIdentifiers.h>
#include "../Graph/ValueTreeIdentifiers.h"

SequenceTreeAudioProcessorEditor::SequenceTreeAudioProcessorEditor (SequenceTreeAudioProcessor& p)
    : AudioProcessorEditor(p),
      audioProcessor      (p),
      graphDocument       (p.document.graphState, p.document.undoManager, p.document.canvasSettings),
      notePreviewGestures (p.playback.traversalSession, p.rtGraphBuilder),
      nodeLimitWarning    (p.document.graphState, workspace)
{
    juce::PropertiesFile::Options interfaceSettingsOptions;

    interfaceSettingsOptions.applicationName     = "Interface";
    interfaceSettingsOptions.filenameSuffix      = ".settings";
    interfaceSettingsOptions.folderName          = JucePlugin_Name;
    interfaceSettingsOptions.osxLibrarySubFolder = "Application Support";

    interfaceSettings = std::make_unique<juce::PropertiesFile>(interfaceSettingsOptions);

    if (interfaceSettings->containsKey(ArsNovaUI::InterfaceIdentifiers::ThemeColour.toString())) {
        const juce::String savedThemeColour = interfaceSettings->getValue(ArsNovaUI::InterfaceIdentifiers::ThemeColour.toString());

        lookAndFeel.applyThemeColour(juce::Colour::fromString(savedThemeColour));
    }

    setLookAndFeel(&lookAndFeel);


    scaledView     = std::make_unique<ArsNovaUI::ScaledView>(workspace, *interfaceSettings);
    nodeCanvas     = std::make_unique<NodeCanvas>(graphDocument, p.bridge, p.document.ornamentLibraries, lookAndFeel);
    nodeController = std::make_unique<NodeController>(*nodeCanvas, graphDocument, notePreviewGestures, p.document.ornamentLibraries);
    port           = std::make_unique<ArsNovaUI::DynamicPort>(*nodeCanvas);
    traversalMenu  = std::make_unique<TraversalMenu>(notePreviewGestures, p.document.graphState, p.document.colourPresets, p.document.undoManager);
    nodeMenu       = std::make_unique<NodeMenu>(*nodeCanvas, p.document.graphState, p.document.colourPresets, p.document.undoManager);
    menuArea       = std::make_unique<MenuArea>(*nodeCanvas, *traversalMenu, *nodeMenu);
    titleBar       = std::make_unique<Titlebar>(p, *nodeCanvas, *nodeController, graphDocument);
    bottomBar      = std::make_unique<BottomBar>(*nodeCanvas, graphDocument, notePreviewGestures, p.document.colourPresets, p.document.ornamentLibraries);
    canvasFrame    = std::make_unique<CanvasFrame>(*nodeCanvas, *menuArea, *titleBar, *bottomBar);

    wireCallbacks();

    if (audioProcessor.pendingRestoreState.isValid()) {
        audioProcessor.applyRestoredState();
    }

    nodeCanvas->rebuildFromNodeMap(audioProcessor.document.graphState.nodeMap);

    titleBar->applyPlaybackState(audioProcessor.playback.isPlaying.load());

    nodeCanvas->addMouseListener(nodeController.get(), true);

    audioProcessor.document.graphState.nodeMap.addListener(&nodeCanvas->treeListener);

    workspace.addAndMakeVisible(port.get());
    workspace.addAndMakeVisible(canvasFrame.get());
    workspace.addAndMakeVisible(menuArea.get());
    workspace.addAndMakeVisible(titleBar.get());
    workspace.addAndMakeVisible(bottomBar.get());

    configureWindowSize();

    audioCommandFrames = juce::VBlankAttachment(this, [this](double) {
        lookAndFeel.frostedBackdropStale = true;

        if (audioProcessor.playback.playbackStateChanged.exchange(false)) {
            titleBar->applyPlaybackState(audioProcessor.playback.isPlaying.load());
        }

        if (audioProcessor.bridge.hasPendingCommands() || !nodeCanvas->asyncUpdates.empty()) {
            nodeCanvas->handleAsyncUpdate();
        }
    });
}

SequenceTreeAudioProcessorEditor::~SequenceTreeAudioProcessorEditor()
{
    if (keyListenerTarget != nullptr) {
        keyListenerTarget->removeKeyListener(this);
    }

    audioProcessor.document.graphState.nodeMap.removeListener(&nodeCanvas->treeListener);

    setLookAndFeel(nullptr);
}

void SequenceTreeAudioProcessorEditor::paint (juce::Graphics& g) { g.fillAll(lookAndFeel.windowColour); }

void SequenceTreeAudioProcessorEditor::resized()
{
    scaledView->setBounds(getLocalBounds());

    const auto windowArea = workspace.getLocalBounds().reduced(ArsNovaUI::Theme::windowGutter);

    auto bounds = windowArea;

    const int barHeight       = ArsNovaUI::Theme::barHeight;
    const int panelSpacing    = juce::roundToInt(barHeight * ArsNovaUI::Theme::menuSpacingRatio);
    const int menuBarWidth    = juce::roundToInt(barHeight * MenuArea::menuBarWidthRatio);
    const int resizerWidth    = juce::roundToInt(barHeight * MenuArea::resizerWidthRatio);
    const int fullMenuWidth   = menuBarWidth + resizerWidth + ArsNovaUI::Theme::menuContentWidth + panelSpacing * 2;
    const int barRowHeight    = barHeight + barCanvasGap;

    menuWidth = juce::jlimit(minimumMenuWidth, fullMenuWidth, menuWidth);

    const int menuColumnWidth = menuWidth + ArsNovaUI::Theme::windowGutter;

    auto menuAreaBounds   = bounds.removeFromLeft(menuColumnWidth).withTrimmedRight(ArsNovaUI::Theme::windowGutter);
    auto titleArea        = bounds.removeFromTop(barRowHeight).withTrimmedBottom(barCanvasGap);
    auto bottomArea       = bounds.removeFromBottom(barRowHeight).withTrimmedTop(barCanvasGap);

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
    const juce::ModifierKeys command      = juce::ModifierKeys::commandModifier;
    const juce::ModifierKeys commandShift = juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier;

    SelectionGestures& selectionGestures = nodeController->selectionGestures;
    juce::UndoManager& undoManager       = audioProcessor.document.undoManager;

    if (key == juce::KeyPress::spaceKey) {
        titleBar->applyPlaybackState(!audioProcessor.playback.isPlaying.load());

        return true;
    }

    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) {
        selectionGestures.deleteSelection();
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
        selectionGestures.copySelection();
        return true;
    }

    if (key == juce::KeyPress('v', command, 0)) {
        juce::Point<float> pastePoint = nodeCanvas->getLocalPoint(port.get(), port->getLocalBounds().getCentre()).toFloat();

        if (port->getLocalBounds().contains(port->getMouseXYRelative())) {
            pastePoint = nodeCanvas->getMouseXYRelative().toFloat();
        }

        selectionGestures.pasteAt(pastePoint.transformedBy(nodeCanvas->modelTransform).roundToInt());
        return true;
    }

    if (key == juce::KeyPress('a', command, 0)) {
        nodeCanvas->nodeManager.selectAll();
        return true;
    }

    return false;
}

void SequenceTreeAudioProcessorEditor::configureWindowSize()
{
    scaledView->onZoomChanged = [this]() {
        const float                          zoom        = scaledView->zoom;
        const float                          zoomRatio   = scaledView->previousZoom / zoom;
        juce::Component* const               window      = getTopLevelComponent();
        const juce::Displays::Display* const display     = juce::Desktop::getInstance().getDisplays().getDisplayForRect(window->getScreenBounds());

        const float rescaledZoom = port->view.zoom * zoomRatio;

        const juce::Point<float> zoomOrigin = -port->getPosition().toFloat();

        menuWidth = juce::roundToInt(menuWidth * zoomRatio);

        port->setZoom(rescaledZoom, zoomOrigin);

        updateResizeLimits();

        resized();

        if (audioProcessor.wrapperType == juce::AudioProcessor::wrapperType_Standalone && display != nullptr) {
            window->setBounds(window->getBounds().constrainedWithin(display->userArea));
        }
    };

    addAndMakeVisible(*scaledView);

    setResizable(true, false);
    setSize(seedWidth, seedHeight);

    const int barsWidth = juce::jmax(titleBar->narrowestWidth(), bottomBar->narrowestWidth());

    const int minimumContentWidth  = ArsNovaUI::Theme::windowGutter * horizontalGutterCount + minimumMenuWidth + barsWidth;
    const int minimumContentHeight = ArsNovaUI::Theme::windowGutter * verticalGutterCount + menuArea->narrowestHeight();

    scaledView->minimumContentSize = { minimumContentWidth, minimumContentHeight };

    scaledView->onZoomChanged();

    const int openingWidth  = juce::roundToInt(getConstrainer()->getMinimumWidth() * openingScale);
    const int openingHeight = juce::roundToInt(getConstrainer()->getMinimumHeight() * openingScale);

    setSize(openingWidth, openingHeight);
}

void SequenceTreeAudioProcessorEditor::updateResizeLimits()
{
    const int barsWidth        = juce::jmax(titleBar->narrowestWidth(), bottomBar->narrowestWidth());
    const int menuColumnWidth  = juce::jmax(menuWidth, minimumMenuWidth);

    scaledView->minimumContentSize.x = ArsNovaUI::Theme::windowGutter * horizontalGutterCount + menuColumnWidth + barsWidth;

    const juce::Point<int> minimumWindowSize = scaledView->scaledMinimumSize();

    setResizeLimits(minimumWindowSize.x, minimumWindowSize.y, maximumWindowSide, maximumWindowSide);

    if (getWidth() < getConstrainer()->getMinimumWidth()) {
        setSize(getConstrainer()->getMinimumWidth(), getHeight());
    }
}


void SequenceTreeAudioProcessorEditor::wireCallbacks()
{
    settingsLauncher.openingScale = ArsNovaUI::SettingsMenu::openingScale;
    ornamentLauncher.openingScale = OrnamentWindow::openingScale;

    titleBar->onDisplayModeChanged                  = [this](NodeInfo::DisplayMode mode) { bottomBar->applyDisplayMode(mode); };
    menuArea->onSettingsClicked                     = [this]() { settingsLauncher.show(*menuArea); };
    traversalMenu->editTraversalRulesButton.onClick = [this]() { traversalRulesLauncher.show(*traversalMenu); };
    nodeMenu->editTraversalRulesButton.onClick      = [this]() { traversalRulesLauncher.show(*nodeMenu); };

    bottomBar->ornamentPane.onWindowRequested = [this]() {
        audioProcessor.document.ornamentLibraries.rescan(audioProcessor.document.ornamentUndoManager);

        ornamentLauncher.show(*bottomBar);
    };

    port->onTransformChanged = [this](const juce::AffineTransform& viewTransform, float zoom) { nodeCanvas->applyViewTransform(viewTransform, zoom); };

    menuArea->resizer.onWidthDragged = [this](int newWidth) {
        menuWidth = newWidth;

        resized();

        updateResizeLimits();

        repaint();
    };
}

std::unique_ptr<ArsNovaUI::SettingsMenu> SequenceTreeAudioProcessorEditor::makeSettingsMenu()
{
    std::unique_ptr<ArsNovaUI::SettingsMenu> menu = std::make_unique<ArsNovaUI::SettingsMenu>(lookAndFeel, *scaledView, audioProcessor.document.colourPresets, graphDocument.undoManager);

    auto graphGridFormat    = std::make_unique<ArsNovaUI::NumberFormat>(GridMultiplier::minimumMultiplier, GridMultiplier::maximumMultiplier, ArsNovaUI::ValueFormat::editableDecimalPlaces);
    auto ornamentGridFormat = std::make_unique<ArsNovaUI::NumberFormat>(GridMultiplier::minimumMultiplier, GridMultiplier::maximumMultiplier, ArsNovaUI::ValueFormat::editableDecimalPlaces);

    graphGridFormat->suffix    = "x";
    ornamentGridFormat->suffix = "x";

    ArsNovaUI::ValueEditor& graphGridEditor    = menu->addEditorRow("Grid spacing", std::move(graphGridFormat));
    ArsNovaUI::ValueEditor& ornamentGridEditor = menu->addEditorRow("Ornament grid spacing", std::move(ornamentGridFormat));

    graphGridEditor.bindEditor(graphDocument.canvasSettings, ValueTreeIdentifiers::GraphGridMultiplier);
    ornamentGridEditor.bindEditor(graphDocument.canvasSettings, ValueTreeIdentifiers::OrnamentGridMultiplier);

    return menu;
}
