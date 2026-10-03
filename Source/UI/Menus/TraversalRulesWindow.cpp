#include "TraversalRulesWindow.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/TraversalRuleState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Plugin/PluginProcessor.h"
#include "../../Script/ScriptCompiler.h"

TraversalRulesWindow::TraversalRulesWindow(const ApplicationContext& context)
    : titlebar(context),
      rulesPanel(context),
      context(context)
{
    setLookAndFeel(context.lookAndFeel);

    rulesPanel.resizer.onWidthDragged          = [this](int newWidth) { setPanelWidth(newWidth); };
    rulesPanel.labelPanel.onLabelClicked       = [this](FileLabel* label) { setActivePage(label->fileId); };
    rulesPanel.panelTitlebar.addButton.onClick = [this] { addRule(); };
    titlebar.playButton.onClick                = [this] { makeViewedRuleActive(); };

    rulesPanel.labelPanel.onLabelsReordered = [this](std::vector<int> fileIds) {
        this->context.traversalRuleState->reorderRules(fileIds, nullptr);
    };

    rulesPanel.labelPanel.onLabelRemoved = [this](int fileId) {
        juce::MessageManager::callAsync(
            [safeThis = juce::Component::SafePointer<TraversalRulesWindow>(this), fileId] {
                if (safeThis != nullptr) {
                    safeThis->removeRule(fileId);
                }
            });
    };

    addAndMakeVisible(titlebar);
    addAndMakeVisible(rulesPanel);

    context.traversalRuleState->rules.addListener(this);

    syncWithRuleState();
}

TraversalRulesWindow::~TraversalRulesWindow()
{
    context.traversalRuleState->rules.removeListener(this);

    setLookAndFeel(nullptr);
}

void TraversalRulesWindow::paint(juce::Graphics& graphics)
{
    const Theme&               theme        = CustomLookAndFeel::get(*this);
    const juce::Rectangle<int> statusBounds = getLocalBounds().removeFromBottom(statusBarHeight);

    graphics.setColour(theme.baseDarkColour2);
    graphics.fillRect(getLocalBounds());

    graphics.setColour(theme.baseDarkColour1);
    graphics.fillRect(statusBounds);

    if (statusIsError) {
        graphics.setColour(theme.scriptErrorColour);
    }
    else {
        graphics.setColour(theme.scriptOkColour);
    }

    graphics.setFont(juce::Font(juce::FontOptions(Theme::labelFontHeight)));
    graphics.drawText(statusText, statusBounds.reduced(Theme::menuEdgeInset, 0), juce::Justification::centredLeft, true);

    graphics.setColour(juce::Colours::black);
    graphics.drawRect(getLocalBounds(), 1);
}

void TraversalRulesWindow::resized()
{
    auto bounds = getLocalBounds();

    panelWidth = clampPanelWidth(panelWidth);

    bounds.removeFromBottom(statusBarHeight);

    rulesPanel.setBounds(bounds.removeFromLeft(panelWidth));
    titlebar.setBounds(bounds.removeFromTop(RulesTitlebar::preferredHeight));

    if (activePage != nullptr) {
        activePage->setBounds(bounds);
    }
}

void TraversalRulesWindow::setActivePage(int id)
{
    const auto match = filePages.find(id);

    if (match == filePages.end()) {
        return;
    }

    FilePage* page = match->second.get();

    if (page == activePage) {
        return;
    }

    if (activePage != nullptr) {
        removeChildComponent(activePage);
    }

    activePage   = page;
    viewedRuleId = id;

    addAndMakeVisible(page);

    rulesPanel.selectLabel(id);

    resized();

    compileViewedPage();
}

void TraversalRulesWindow::timerCallback()
{
    stopTimer();

    compileViewedPage();
}

void TraversalRulesWindow::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    triggerAsyncUpdate();
}

void TraversalRulesWindow::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex)
{
    triggerAsyncUpdate();
}

void TraversalRulesWindow::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (property == ValueTreeIdentifiers::ActiveRuleId) {
        triggerAsyncUpdate();
    }
}

void TraversalRulesWindow::handleAsyncUpdate()
{
    syncWithRuleState();
}

void TraversalRulesWindow::syncWithRuleState()
{
    TraversalRuleState& state = *context.traversalRuleState;
    std::vector<int>    ruleOrder;

    state.ensureDefaultRule();

    ruleOrder.reserve(static_cast<size_t>(state.rules.getNumChildren()));

    for (int i = 0; i < state.rules.getNumChildren(); ++i) {
        const juce::ValueTree rule   = state.rules.getChild(i);
        const int             ruleId = rule.getProperty(ValueTreeIdentifiers::Id);
        const juce::String    source = rule.getProperty(ValueTreeIdentifiers::RuleSource).toString();
        const auto            match  = filePages.find(ruleId);

        ruleOrder.push_back(ruleId);

        if (match == filePages.end()) {
            rulesPanel.addLabel(ruleId, rule.getProperty(ValueTreeIdentifiers::RuleName).toString());

            createPage(ruleId, source);
        }
        else if (match->second->getDocument().getAllContent() != source) {
            const juce::ScopedValueSetter<bool> silence(match->second->suppressTextChanged, true);

            match->second->loadContent(source);
        }
    }

    for (auto page = filePages.begin(); page != filePages.end(); ) {
        if (state.rules.getChildWithProperty(ValueTreeIdentifiers::Id, page->first).isValid()) {
            ++page;

            continue;
        }

        if (activePage == page->second.get()) {
            activePage   = nullptr;
            viewedRuleId = -1;
        }

        rulesPanel.removeLabel(page->first);

        page = filePages.erase(page);
    }

    rulesPanel.labelPanel.applyOrder(ruleOrder);

    if (activePage == nullptr) {
        setActivePage(state.rules.getProperty(ValueTreeIdentifiers::ActiveRuleId, -1));

        return;
    }

    resized();

    compileViewedPage();
}

void TraversalRulesWindow::addRule()
{
    const juce::ValueTree rule = context.traversalRuleState->addRule(nullptr);

    syncWithRuleState();

    setActivePage(rule.getProperty(ValueTreeIdentifiers::Id));
}

void TraversalRulesWindow::removeRule(int ruleId)
{
    TraversalRuleState& state       = *context.traversalRuleState;
    const bool          wasLiveRule = static_cast<int>(state.rules.getProperty(ValueTreeIdentifiers::ActiveRuleId, -1)) == ruleId;

    state.removeRule(ruleId, nullptr);

    if (state.rules.getNumChildren() == 0) {
        addRule();

        state.rules.setProperty(ValueTreeIdentifiers::ActiveRuleId, viewedRuleId, nullptr);
    }
    else {
        syncWithRuleState();
    }

    if (wasLiveRule) {
        context.processor->snapshots.publishActiveTraversalRule();
    }

    compileViewedPage();
}

void TraversalRulesWindow::createPage(int ruleId, const juce::String& source)
{
    auto      page        = std::make_unique<FilePage>(context);
    FilePage* createdPage = page.get();

    page->loadContent(source);

    page->onTextChanged = [this, ruleId, createdPage] {
        context.traversalRuleState->setRuleSource(ruleId, createdPage->getDocument().getAllContent(), nullptr);

        startTimer(compileDelayMs);
    };

    filePages.emplace(ruleId, std::move(page));
}

void TraversalRulesWindow::makeViewedRuleActive()
{
    if (viewedRuleId < 0) {
        return;
    }

    context.traversalRuleState->rules.setProperty(ValueTreeIdentifiers::ActiveRuleId, viewedRuleId, nullptr);

    compileViewedPage();
}

void TraversalRulesWindow::compileViewedPage()
{
    if (activePage == nullptr || viewedRuleId < 0) {
        setStatus({}, false);

        return;
    }

    TraversalRuleState& state            = *context.traversalRuleState;
    const juce::String  source           = activePage->getDocument().getAllContent();
    ScriptCompileResult result           = compileTraversalScript(source.toStdString());
    const bool          isLiveRule       = static_cast<int>(state.rules.getProperty(ValueTreeIdentifiers::ActiveRuleId, -1)) == viewedRuleId;
    const juce::String  instructionCount = juce::String(static_cast<int>(result.script.instructions.size()));

    state.setRuleSource(viewedRuleId, source, nullptr);

    activePage->errorLines.clear();

    for (const ScriptDiagnostic& diagnostic : result.diagnostics) {
        activePage->errorLines.push_back(diagnostic.line - 1);
    }

    activePage->repaint();

    if (!result.succeeded()) {
        const ScriptDiagnostic& first = result.diagnostics.front();

        setStatus(juce::String(first.line) + ":" + juce::String(first.column) + "  " + juce::String(first.message), true);

        return;
    }

    if (!isLiveRule) {
        setStatus(instructionCount + " instructions - press play to make this the live rule", false);

        return;
    }

    context.processor->snapshots.publishScript(std::make_shared<RTScript>(std::move(result.script)));

    setStatus(instructionCount + " instructions - live", false);
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

int TraversalRulesWindow::clampPanelWidth(int newWidth) const
{
    const int available = getWidth() - minContentWidth;
    const int maxWidth  = juce::jmax(RulesPanel::minPanelWidth, available);

    return juce::jlimit(RulesPanel::minPanelWidth, maxWidth, newWidth);
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

TraversalRulesWindow::RulesTitlebar::RulesTitlebar(const ApplicationContext& context)
    : Bar(context, { Orientation::Horizontal, titlebarInsetRatio }),
      undoRedoPane(context)
{
    playButton.painter = [this](juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state) {
        ButtonState triangleState = state;

        triangleState.isSelected = true;

        CustomLookAndFeel::get(*this).drawPlayIcon(graphics, bounds, triangleState);
    };

    playButton.setLookAndFeel(context.lookAndFeel);

    undoRedoPane.addButton(&CustomLookAndFeel::drawUndoIcon, "Undo");

    undoRedoPane.addButton(&CustomLookAndFeel::drawRedoIcon, "Redo");

    playButton.setTooltip("Make this the live traversal rule");

    addAndMakeVisible(playButton);
    addAndMakeVisible(undoRedoPane);
}

void TraversalRulesWindow::RulesTitlebar::paintOverBar(juce::Graphics& graphics)
{
    drawSeparator(graphics, (playButton.getRight() + undoRedoPane.getX()) / 2);
}

void TraversalRulesWindow::RulesTitlebar::resized()
{
    auto      bounds     = getContentBounds();
    const int buttonSize = bounds.getHeight();

    playButton.setBounds(bounds.removeFromLeft(buttonSize));

    bounds.removeFromLeft(contentSpacing);

    undoRedoPane.setBounds(bounds.removeFromLeft(buttonSize * 3));
}

TraversalRulesWindow::RulesPanel::RulesPanel(const ApplicationContext& context)
    : resizer(context, PanelResizer::Edge::Right),
      labelPanel(context),
      panelTitlebar(context)
{
    setLookAndFeel(context.lookAndFeel);

    addAndMakeVisible(panelTitlebar);
    addAndMakeVisible(labelPanel);
    addAndMakeVisible(resizer);
}

TraversalRulesWindow::RulesPanel::~RulesPanel()
{
    setLookAndFeel(nullptr);
}

void TraversalRulesWindow::RulesPanel::paint(juce::Graphics& graphics)
{
    const Theme& theme = CustomLookAndFeel::get(*this);

    graphics.setColour(theme.baseDarkColour2);
    graphics.fillRect(getLocalBounds());
}

void TraversalRulesWindow::RulesPanel::resized()
{
    auto bounds = getLocalBounds();

    resizer.setBounds(bounds.removeFromRight(resizerWidth));
    panelTitlebar.setBounds(bounds.removeFromTop(RulesTitlebar::preferredHeight));
    labelPanel.setBounds(bounds);
}

void TraversalRulesWindow::RulesPanel::addLabel(int fileId, const juce::String& name)
{
    labelPanel.addFileLabel(name);

    labelPanel.labels.back()->fileId = fileId;
}

void TraversalRulesWindow::RulesPanel::removeLabel(int fileId)
{
    for (auto& label : labelPanel.labels) {
        if (label->fileId == fileId) {
            labelPanel.removeFileLabel(label.get());

            return;
        }
    }
}

void TraversalRulesWindow::RulesPanel::selectLabel(int fileId)
{
    for (auto& label : labelPanel.labels) {
        if (label->fileId == fileId) {
            labelPanel.setSelectedLabel(label.get());

            return;
        }
    }
}

TraversalRulesWindow::RulesPanel::PanelTitlebar::PanelTitlebar(const ApplicationContext& context)
    : Bar(context, { Orientation::Horizontal, RulesTitlebar::titlebarInsetRatio })
{
    addButton.icon = &CustomLookAndFeel::drawAddIcon;

    addButton.setLookAndFeel(context.lookAndFeel);

    addButton.setTooltip("Add Rule");

    addAndMakeVisible(addButton);
}

void TraversalRulesWindow::RulesPanel::PanelTitlebar::resized()
{
    auto bounds = getContentBounds();

    addButton.setBounds(bounds.removeFromLeft(bounds.getHeight()));
}
