#include "TraversalRulesWindow.h"
#include <ArsNovaUI/Theme/IconLibrary.h>
#include "../../Graph/TraversalRuleState.h"
#include "../../Graph/TypedProperties.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Plugin/AudioSnapshotPublisher.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>

TraversalRulesWindow::TraversalRulesWindow(ArsNovaUI::CustomLookAndFeel& lookAndFeel, TraversalRuleState& traversalRuleState, AudioSnapshotPublisher& snapshots,
                                           juce::UndoManager& undoManager)
    : resizer           (ArsNovaUI::PanelResizer::Edge::Right),
      panelBar          (ArsNovaUI::Bar::Orientation::Horizontal, ArsNovaUI::Theme::contentInsetRatio),
      treePanel         (traversalRuleState, undoManager),
      traversalRuleState(traversalRuleState),
      snapshots         (snapshots),
      undoManager       (undoManager)
{
    resizer.onWidthDragged             = [this](int newWidth) { setPanelWidth(newWidth); };
    treePanel.treeView.onRuleOpened    = [this](int ruleId) { openRule(ruleId); };
    treePanel.treeView.onRuleAdded     = [this](int folderId) { addRule(folderId); };
    treePanel.treeView.onRuleRemoved   = [this](int ruleId) { removeRule(ruleId); };
    treePanel.treeView.onFolderRemoved = [this](int folderId) { removeFolder(folderId); };
    treePanel.treeView.onRuleMadeLive  = [this](int ruleId) { makeRuleLive(ruleId); };
    tabBar.tabStrip.onTabClicked       = [this](int ruleId) { viewRule(ruleId); };
    tabBar.tabStrip.onTabClosed        = [this](int ruleId) { closeTab(ruleId); };

    tabBar.transportPane.addButton(ArsNovaUI::IconLibrary::get().play, "Make this the live traversal rule", [this] { makeRuleLive(viewedRuleId); });

    tabBar.undoRedoPane.addButton(ArsNovaUI::IconLibrary::get().undo, "Undo", [this] {
        if (activePage != nullptr) {
            activePage->juce::CodeEditorComponent::undo();
        }
    });

    tabBar.undoRedoPane.addButton(ArsNovaUI::IconLibrary::get().redo, "Redo", [this] {
        if (activePage != nullptr) {
            activePage->juce::CodeEditorComponent::redo();
        }
    });

    panelTitle.setFont(lookAndFeel.font(ArsNovaUI::Theme::FontStyle::SemiBold, ArsNovaUI::Theme::labelFontHeight));
    panelTitle.setInterceptsMouseClicks(false, false);
    panelTitle.setText("Rules", juce::dontSendNotification);

    panelBar.addAndMakeVisible(panelTitle);
    sidePanel.addAndMakeVisible(panelBar);
    sidePanel.addAndMakeVisible(treePanel);
    sidePanel.addAndMakeVisible(resizer);

    addAndMakeVisible(sidePanel);
    addAndMakeVisible(tabBar);
    addAndMakeVisible(pathBar);

    setLookAndFeel(&lookAndFeel);

    traversalRuleState.rules.addListener(this);
    traversalRuleState.folders.addListener(this);

    syncWithRuleState();

    openRule(traversalRuleState.activeRuleId);
}

TraversalRulesWindow::~TraversalRulesWindow()
{
    traversalRuleState.rules.removeListener(this);
    traversalRuleState.folders.removeListener(this);
}

void TraversalRulesWindow::paint(juce::Graphics& graphics)
{
    const ArsNovaUI::Theme& theme = ArsNovaUI::CustomLookAndFeel::get(*this);

    const juce::Rectangle<int> statusBounds = getLocalBounds().removeFromBottom(statusBarHeight).reduced(ArsNovaUI::Theme::menuEdgeInset, 0);

    const juce::Font statusFont = theme.font(ArsNovaUI::Theme::FontStyle::Mono, ArsNovaUI::Theme::labelFontHeight);
    const juce::Font hintFont   = theme.font(ArsNovaUI::Theme::FontStyle::Regular, ArsNovaUI::RowList::rowFontHeight);

    graphics.setColour(theme.surfaceColour);
    graphics.fillRect(getLocalBounds());

    graphics.setColour(theme.raisedColour);
    graphics.fillRect(getLocalBounds().removeFromBottom(statusBarHeight));

    if (activePage == nullptr) {
        graphics.setColour(theme.mutedTextColour);
        graphics.setFont(hintFont);
        graphics.drawFittedText("No rule open. Pick one in the Rules panel.", editorArea.reduced(ArsNovaUI::Theme::windowGutter), juce::Justification::centred, 2);
    }

    graphics.setFont(statusFont);

    graphics.setColour(theme.mutedTextColour);
    graphics.drawText(caretText, statusBounds, juce::Justification::centredRight, false);

    if (statusIsError) {
        graphics.setColour(theme.errorColour);
    }
    else {
        graphics.setColour(theme.successColour);
    }

    const int caretReserve = juce::GlyphArrangement::getStringWidthInt(statusFont, caretText) + ArsNovaUI::Theme::windowGutter;

    graphics.drawText(statusText, statusBounds.withTrimmedRight(caretReserve), juce::Justification::centredLeft, true);

    graphics.setColour(theme.borderColour);
    graphics.drawRect(getLocalBounds(), 1);
}

void TraversalRulesWindow::paintOverChildren(juce::Graphics& graphics)
{
    const int separatorWidth = 1;
    const int separatorX     = sidePanel.getRight() - separatorWidth;

    graphics.setColour(ArsNovaUI::CustomLookAndFeel::get(*this).borderColour);
    graphics.fillRect(separatorX, sidePanel.getY(), separatorWidth, sidePanel.getHeight());
}

void TraversalRulesWindow::resized()
{
    const int resizerWidth = juce::roundToInt(ArsNovaUI::Theme::barHeight * resizerWidthRatio);

    auto bounds = getLocalBounds();

    bounds.removeFromBottom(statusBarHeight);

    panelWidth = clampPanelWidth(panelWidth);

    sidePanel.setBounds(bounds.removeFromLeft(panelWidth));

    auto side = sidePanel.getLocalBounds();

    resizer.setBounds(side.removeFromRight(resizerWidth));
    panelBar.setBounds(side.withHeight(ArsNovaUI::Theme::barHeight));
    panelTitle.setBounds(panelBar.getLocalBounds().reduced(ArsNovaUI::Theme::windowGutter, 0));
    treePanel.setBounds(side);

    tabBar.setBounds(bounds.removeFromTop(RuleTabBar::preferredHeight));
    pathBar.setBounds(bounds.removeFromTop(ArsNovaUI::PathBar::preferredHeight));

    editorArea = bounds;

    if (activePage != nullptr) {
        activePage->setBounds(bounds);
    }
}

void TraversalRulesWindow::openRule(int ruleId)
{
    const juce::ValueTree rule = traversalRuleState.rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, ruleId);

    const bool isOpen = std::ranges::find(tabBar.tabStrip.tabs, ruleId, &ArsNovaUI::TabBar::Tab::id) != tabBar.tabStrip.tabs.end();

    const RuleProperties properties(rule);

    if (! rule.isValid()) {
        return;
    }

    if (! isOpen) {
        tabBar.tabStrip.tabs.push_back({ ruleId, properties.ruleName, false });
    }

    viewRule(ruleId);
}

void TraversalRulesWindow::addRule(int folderId)
{
    undoManager.beginNewTransaction();

    const juce::ValueTree rule = traversalRuleState.addRule(folderId, &undoManager);

    const int ruleId = rule.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    syncWithRuleState();

    openRule(ruleId);

    if (treePanel.isShowing()) {
        treePanel.treeView.beginRename(RuleItemKind::Rule, ruleId);
    }
}

void TraversalRulesWindow::removeRule(int ruleId)
{
    const bool wasLiveRule = traversalRuleState.activeRuleId == ruleId;

    undoManager.beginNewTransaction();

    traversalRuleState.removeRule(ruleId, &undoManager);

    syncWithRuleState();

    if (wasLiveRule) {
        snapshots.publishActiveTraversalRule();
    }
}

void TraversalRulesWindow::removeFolder(int folderId)
{
    undoManager.beginNewTransaction();

    traversalRuleState.removeFolder(folderId, &undoManager);

    syncWithRuleState();
}

void TraversalRulesWindow::makeRuleLive(int ruleId)
{
    if (ruleId < 0) {
        return;
    }

    traversalRuleState.activeRuleId.setValue(ruleId, nullptr);

    if (ruleId != viewedRuleId) {
        snapshots.publishActiveTraversalRule();
    }

    compileViewedPage();
}

void TraversalRulesWindow::syncWithRuleState()
{
    TraversalRuleState& state = traversalRuleState;

    bool viewedRuleRemoved = false;

    state.ensureDefaultRule();

    for (int i = 0; i < state.rules.getNumChildren(); ++i) {
        const juce::ValueTree rule = state.rules.getChild(i);

        const RuleProperties properties(rule);

        const int ruleId = rule.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        const juce::String source = properties.ruleSource;

        const auto match = filePages.find(ruleId);

        if (match == filePages.end()) {
            createPage(ruleId, source);
        }
        else if (match->second->getDocument().getAllContent() != source) {
            const juce::ScopedValueSetter<bool> silence(match->second->suppressTextChanged, true);

            match->second->loadContent(source);
        }
    }

    for (auto page = filePages.begin(); page != filePages.end(); ) {
        const int ruleId = page->first;

        if (state.rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, ruleId).isValid()) {
            ++page;

            continue;
        }

        if (activePage == page->second.get()) {
            removeChildComponent(activePage);

            activePage        = nullptr;
            viewedRuleId      = -1;
            viewedRuleRemoved = true;
        }

        std::erase_if(tabBar.tabStrip.tabs, [ruleId](const ArsNovaUI::TabBar::Tab& tab) { return tab.id == ruleId; });

        page = filePages.erase(page);
    }

    treePanel.treeView.liveRuleId   = state.activeRuleId;
    treePanel.treeView.viewedRuleId = viewedRuleId;

    treePanel.treeView.rebuild();

    if (viewedRuleRemoved && ! tabBar.tabStrip.tabs.empty()) {
        viewRule(tabBar.tabStrip.tabs.front().id);

        return;
    }

    if (viewedRuleRemoved) {
        openRule(state.activeRuleId);

        return;
    }

    if (treePanel.treeView.liveRuleId != viewedRuleId) {
        snapshots.publishActiveTraversalRule();
    }

    refreshTabs();

    compileViewedPage();
}

void TraversalRulesWindow::refreshTabs()
{
    const int liveRuleId = traversalRuleState.activeRuleId;

    for (ArsNovaUI::TabBar::Tab& tab : tabBar.tabStrip.tabs) {
        const RuleProperties rule(traversalRuleState.rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, tab.id));

        tab.name   = rule.ruleName;
        tab.marked = tab.id == liveRuleId;
    }

    tabBar.tabStrip.activeTabId = viewedRuleId;

    tabBar.tabStrip.resized();
}

void TraversalRulesWindow::viewRule(int ruleId)
{
    const auto match = filePages.find(ruleId);

    if (match == filePages.end()) {
        return;
    }

    ArsNovaUI::FilePage* page = match->second.get();

    if (page != activePage) {
        if (activePage != nullptr) {
            removeChildComponent(activePage);
        }

        activePage    = page;
        viewedRuleId  = ruleId;
        viewedOutline = {};

        addAndMakeVisible(page);

        resized();
    }

    treePanel.treeView.viewedRuleId = ruleId;

    treePanel.treeView.revealRule(ruleId);

    refreshTabs();

    compileViewedPage();
}

void TraversalRulesWindow::closeTab(int ruleId)
{
    const auto tab = std::ranges::find(tabBar.tabStrip.tabs, ruleId, &ArsNovaUI::TabBar::Tab::id);

    const int index = static_cast<int>(std::distance(tabBar.tabStrip.tabs.begin(), tab));

    if (tab == tabBar.tabStrip.tabs.end()) {
        return;
    }

    tabBar.tabStrip.tabs.erase(tab);

    if (ruleId != viewedRuleId) {
        refreshTabs();

        return;
    }

    if (! tabBar.tabStrip.tabs.empty()) {
        const int lastTab = static_cast<int>(tabBar.tabStrip.tabs.size()) - 1;

        const size_t nextTab = static_cast<size_t>(juce::jmin(index, lastTab));

        viewRule(tabBar.tabStrip.tabs[nextTab].id);

        return;
    }

    removeChildComponent(activePage);

    activePage                      = nullptr;
    viewedRuleId                    = -1;
    viewedOutline                   = {};
    treePanel.treeView.viewedRuleId = -1;

    treePanel.treeView.rebuild();

    refreshTabs();

    compileViewedPage();

    repaint();
}

void TraversalRulesWindow::createPage(int ruleId, const juce::String& source)
{
    auto page = std::make_unique<ArsNovaUI::FilePage>(ArsNovaUI::CustomLookAndFeel::get(*this));

    ArsNovaUI::FilePage* createdPage = page.get();

    page->loadContent(source);

    page->onTextChanged = [this, ruleId, createdPage] {
        traversalRuleState.setRuleSource(ruleId, createdPage->getDocument().getAllContent(), nullptr);

        startTimer(compileDelayMs);
    };

    page->onCaretMoved = [this, createdPage] {
        if (createdPage == activePage) {
            showCaretPosition();
        }
    };

    filePages.emplace(ruleId, std::move(page));
}

void TraversalRulesWindow::compileViewedPage()
{
    if (activePage == nullptr || viewedRuleId < 0) {
        setStatus({}, false);

        showCaretPosition();

        return;
    }

    TraversalRuleState& state = traversalRuleState;

    const juce::String source = activePage->getDocument().getAllContent();

    ScriptCompileResult result = compileTraversalScript(source.toStdString());

    const bool isLiveRule = state.activeRuleId == viewedRuleId;

    const int instructionTotal = static_cast<int>(result.script.instructions.size());

    const juce::String instructionCount = juce::String(instructionTotal);

    state.setRuleSource(viewedRuleId, source, nullptr);

    activePage->errorLines.clear();

    for (const ScriptDiagnostic& diagnostic : result.diagnostics) {
        const int errorLineIndex = diagnostic.line - 1;

        activePage->errorLines.push_back(errorLineIndex);
    }

    activePage->repaint();

    if (! result.outline.className.empty() || ! result.outline.entries.empty()) {
        viewedOutline = result.outline;
    }

    showCaretPosition();

    if (! result.succeeded()) {
        const ScriptDiagnostic& first = result.diagnostics.front();

        setStatus(juce::String(first.line) + ":" + juce::String(first.column) + "  " + juce::String(first.message), true);

        return;
    }

    if (! isLiveRule) {
        setStatus(instructionCount + " instructions - press play to make this the live rule", false);

        return;
    }

    snapshots.publishScript(std::make_shared<RTScript>(std::move(result.script)));

    setStatus(instructionCount + " instructions - live", false);
}

void TraversalRulesWindow::showCaretPosition()
{
    juce::StringArray segments = traversalRuleState.pathOf(viewedRuleId);

    int entry = -1;

    caretText = {};

    if (activePage != nullptr) {
        const juce::CodeDocument::Position caret = activePage->getCaretPos();

        const int lineNumber   = caret.getLineNumber() + 1;
        const int columnNumber = caret.getIndexInLine() + 1;

        entry     = outlineEntryAt(lineNumber);
        caretText = "Ln " + juce::String(lineNumber) + ", Col " + juce::String(columnNumber);
    }

    if (activePage != nullptr && ! viewedOutline.className.empty()) {
        segments.add(juce::String(viewedOutline.className));
    }

    if (entry >= 0) {
        segments.add(juce::String(viewedOutline.entries[static_cast<size_t>(entry)].name));
    }

    pathBar.segments = segments;

    pathBar.repaint();

    repaint(getLocalBounds().removeFromBottom(statusBarHeight));
}

void TraversalRulesWindow::setStatus(const juce::String& text, bool isError)
{
    if (statusText == text && statusIsError == isError) {
        return;
    }

    statusText    = text;
    statusIsError = isError;

    repaint(getLocalBounds().removeFromBottom(statusBarHeight));
}

void TraversalRulesWindow::setPanelWidth(int newWidth)
{
    const int clamped = clampPanelWidth(newWidth);

    if (clamped == panelWidth) {
        return;
    }

    panelWidth = clamped;

    resized();
}

void TraversalRulesWindow::timerCallback()
{
    stopTimer();

    compileViewedPage();
}

void TraversalRulesWindow::handleAsyncUpdate()
{
    syncWithRuleState();
}

void TraversalRulesWindow::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    triggerAsyncUpdate();
}

void TraversalRulesWindow::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex)
{
    triggerAsyncUpdate();
}

void TraversalRulesWindow::valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex)
{
    triggerAsyncUpdate();
}

void TraversalRulesWindow::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (property != ValueTreeIdentifiers::RuleSource) {
        triggerAsyncUpdate();
    }
}

int TraversalRulesWindow::clampPanelWidth(int newWidth) const
{
    const int available = getWidth() - minContentWidth;
    const int maxWidth  = juce::jmax(minPanelWidth, available);

    return juce::jlimit(minPanelWidth, maxWidth, newWidth);
}

int TraversalRulesWindow::outlineEntryAt(int line) const
{
    int found = -1;

    for (int index = 0; index < static_cast<int>(viewedOutline.entries.size()); ++index) {
        if (viewedOutline.entries[static_cast<size_t>(index)].line <= line) {
            found = index;
        }
    }

    return found;
}
