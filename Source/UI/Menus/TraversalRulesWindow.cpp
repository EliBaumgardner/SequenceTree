//
// Created by Eli Baumgardner on 7/21/26.
//

#include "TraversalRulesWindow.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/ValueTreeState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Plugin/PluginProcessor.h"
#include "../../Script/ScriptCompiler.h"

TraversalRulesWindow::TraversalRulesWindow(ApplicationContext& context) : context(context),
    titlebar(context), rulesPanel(context)
{
    setLookAndFeel(context.lookAndFeel);

    rulesPanel.onWidthDragged = [this](int newWidth) { setPanelWidth(newWidth); };

    filePageViewport.setScrollBarsShown(true, false);

    addAndMakeVisible(titlebar);
    addAndMakeVisible(rulesPanel);
    addAndMakeVisible(filePageViewport);

    rulesPanel.propagateLabelClicked = [this](int fileId) { setActivePage(fileId); };
    rulesPanel.propagateAddClicked   = [this] { addRule(); };

    rulesPanel.propagateLabelRemoved = [this](int fileId) {
        juce::MessageManager::callAsync(
            [safeThis = juce::Component::SafePointer<TraversalRulesWindow>(this), fileId] {
                if (safeThis != nullptr) {
                    safeThis->removeRule(fileId);
                }
            });
    };

    titlebar.onPlayClicked = [this] { makeViewedRuleActive(); };

    loadRules();
}

TraversalRulesWindow::~TraversalRulesWindow() {
    setLookAndFeel(nullptr);
}

void TraversalRulesWindow::paint(juce::Graphics& g) {
    const Theme& theme = CustomLookAndFeel::get(*this);

    g.setColour(theme.baseDarkColour2);
    g.fillRect(getLocalBounds());

    const juce::Rectangle<int> statusBounds = statusBarBounds();

    g.setColour(theme.baseDarkColour1);
    g.fillRect(statusBounds);

    g.setColour(statusIsError ? theme.scriptErrorColour : theme.scriptOkColour);
    g.setFont(juce::Font(juce::FontOptions(Theme::labelFontHeight)));
    g.drawText(statusText, statusBounds.reduced(Theme::menuEdgeInset, 0),
               juce::Justification::centredLeft, true);

    g.setColour(juce::Colours::black);
    g.drawRect(getLocalBounds(), 1);
}

juce::Rectangle<int> TraversalRulesWindow::statusBarBounds() const {
    return getLocalBounds().removeFromBottom(statusBarHeight);
}

void TraversalRulesWindow::resized() {
    panelWidth = clampPanelWidth(panelWidth);

    auto bounds = getLocalBounds();

    bounds.removeFromBottom(statusBarHeight);

    rulesPanel.setBounds(bounds.removeFromLeft(panelWidth));
    titlebar.setBounds(bounds.removeFromTop(RulesTitlebar::preferredHeight));

    filePageViewport.setBounds(bounds);

    if (activePage != nullptr) {
        const int pageWidth = filePageViewport.getMaximumVisibleWidth();

        activePage->setSize(pageWidth, activePage->preferredHeightForWidth(pageWidth));
    }
}

void TraversalRulesWindow::loadRules() {
    ValueTreeState& state = *context.valueTreeState;

    state.ensureDefaultTraversalRule();

    for (int i = 0; i < state.traversalRules.getNumChildren(); ++i) {
        const juce::ValueTree rule = state.traversalRules.getChild(i);

        const int ruleId = rule.getProperty(ValueTreeIdentifiers::Id);

        rulesPanel.addLabel(ruleId, rule.getProperty(ValueTreeIdentifiers::RuleName).toString());
        createPage(ruleId, rule.getProperty(ValueTreeIdentifiers::RuleSource).toString());
    }

    setActivePage(state.getActiveTraversalRuleId());
}

void TraversalRulesWindow::createPage(int ruleId, const juce::String& source) {
    auto page = std::make_unique<FilePage>(context);

    page->setText(source);
    page->onTextChanged = [this] { startTimer(compileDelayMs); };

    filePages.emplace(ruleId, std::move(page));
}

void TraversalRulesWindow::addRule() {
    const juce::ValueTree rule = context.valueTreeState->addTraversalRule(nullptr);

    const int ruleId = rule.getProperty(ValueTreeIdentifiers::Id);

    rulesPanel.addLabel(ruleId, rule.getProperty(ValueTreeIdentifiers::RuleName).toString());
    createPage(ruleId, rule.getProperty(ValueTreeIdentifiers::RuleSource).toString());

    setActivePage(ruleId);
}

void TraversalRulesWindow::removeRule(int ruleId) {
    ValueTreeState& state = *context.valueTreeState;

    const bool wasLiveRule = state.getActiveTraversalRuleId() == ruleId;

    state.removeTraversalRule(ruleId, nullptr);

    const auto match = filePages.find(ruleId);

    if (match != filePages.end()) {
        if (activePage == match->second.get()) {
            activePage   = nullptr;
            viewedRuleId = -1;

            filePageViewport.setViewedComponent(nullptr, false);
        }

        filePages.erase(match);
    }

    if (state.traversalRules.getNumChildren() == 0) {
        addRule();
        state.setActiveTraversalRuleId(viewedRuleId, nullptr);
    }
    else if (activePage == nullptr) {
        setActivePage(state.getActiveTraversalRuleId());
    }

    if (wasLiveRule) {
        context.processor->snapshots.publishActiveTraversalRule();
    }

    compileViewedPage();
}

void TraversalRulesWindow::setActivePage(int id) {
    const auto match = filePages.find(id);

    if (match == filePages.end())
        return;

    FilePage* page = match->second.get();

    if (page == activePage) {
        return;
    }

    activePage   = page;
    viewedRuleId = id;

    filePageViewport.setViewedComponent(page, false);
    rulesPanel.selectLabel(id);

    resized();
    compileViewedPage();
}

void TraversalRulesWindow::makeViewedRuleActive() {
    if (viewedRuleId < 0) {
        return;
    }

    context.valueTreeState->setActiveTraversalRuleId(viewedRuleId, nullptr);

    compileViewedPage();
}

void TraversalRulesWindow::timerCallback() {
    stopTimer();
    compileViewedPage();
}

void TraversalRulesWindow::compileViewedPage() {
    if (activePage == nullptr || viewedRuleId < 0) {
        setStatus({}, false);
        return;
    }

    ValueTreeState& state = *context.valueTreeState;

    const juce::String source = activePage->getText();

    state.setTraversalRuleSource(viewedRuleId, source, nullptr);

    ScriptCompileResult result = compileTraversalScript(source.toStdString());

    activePage->clearLineErrors();

    for (const ScriptDiagnostic& diagnostic : result.diagnostics) {
        activePage->setLineError(diagnostic.line, diagnostic.message);
    }

    if (!result.succeeded()) {
        const ScriptDiagnostic& first = result.diagnostics.front();

        setStatus(juce::String(first.line) + ":" + juce::String(first.column)
                  + "  " + juce::String(first.message), true);
        return;
    }

    const bool isLiveRule = state.getActiveTraversalRuleId() == viewedRuleId;

    const juce::String instructionCount = juce::String((int) result.script.instructions.size());

    if (!isLiveRule) {
        setStatus(instructionCount + " instructions - press play to make this the live rule", false);
        return;
    }

    context.processor->snapshots.publishScript(std::make_shared<RTScript>(std::move(result.script)));

    setStatus(instructionCount + " instructions - live", false);
}

void TraversalRulesWindow::setStatus(const juce::String& text, bool isError) {
    if (statusText == text && statusIsError == isError) {
        return;
    }

    statusText    = text;
    statusIsError = isError;

    repaint(statusBarBounds());
}

int TraversalRulesWindow::clampPanelWidth(int newWidth) const {
    const int available = getWidth() - minContentWidth;
    const int maxWidth  = juce::jmax(RulesPanel::minPanelWidth, available);

    return juce::jlimit(RulesPanel::minPanelWidth, maxWidth, newWidth);
}

void TraversalRulesWindow::setPanelWidth(int newWidth) {
    const int clamped = clampPanelWidth(newWidth);

    if (clamped == panelWidth) {
        return;
    }

    panelWidth = clamped;
    resized();
}

TraversalRulesWindow::RulesTitlebar::RulesTitlebar(ApplicationContext& context)
    : Bar(context, { Orientation::horizontal, Background::litFromTop }),
      undoRedoPane(context)
{
    playButton = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            ButtonState triangleState = state;
            triangleState.isSelected = true;

            CustomLookAndFeel::get(*this).drawPlayIcon(g, bounds, triangleState);
        }, context.lookAndFeel);

    playButton->setTooltip("Make this the live traversal rule");

    playButton->onClick = [this] {
        if (onPlayClicked != nullptr) {
            onPlayClicked();
        }
    };

    addAndMakeVisible(playButton.get());
    addAndMakeVisible(undoRedoPane);

    configureUndoRedoPane();
}

void TraversalRulesWindow::RulesTitlebar::configureUndoRedoPane() {
    undoRedoPane.addButton(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawUndoIcon(g, bounds, state);
        },
        "Undo");

    undoRedoPane.addButton(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawRedoIcon(g, bounds, state);
        },
        "Redo");
}

void TraversalRulesWindow::RulesTitlebar::paintOverBar(juce::Graphics& g) {
    drawSeparator(g, (playButton->getRight() + undoRedoPane.getX()) / 2);
}

void TraversalRulesWindow::RulesTitlebar::resized() {
    auto bounds = getContentBounds();

    const int buttonSize = bounds.getHeight();

    playButton->setBounds(bounds.removeFromLeft(buttonSize));
    bounds.removeFromLeft(contentSpacing);

    undoRedoPane.setBounds(bounds.removeFromLeft(buttonSize * 3));
}

TraversalRulesWindow::RulesPanel::RulesPanel(ApplicationContext& context)
    : ResizablePanel(context, ResizeEdge::Right, resizerWidth),
      panelTitlebar(context)
{

    labelPanel = std::make_unique<LabelPanel>(context);

    panelTitlebar.onAddClicked = [this] {
        if (propagateAddClicked != nullptr) {
            propagateAddClicked();
        }
    };

    labelPanel->onLabelClicked = [this](FileLabel* label) {
        if (propagateLabelClicked != nullptr) {
            propagateLabelClicked(label->fileId);
        }
    };

    labelPanel->onLabelRemoved = [this](int fileId) {
        if (propagateLabelRemoved != nullptr) {
            propagateLabelRemoved(fileId);
        }
    };

    addAndMakeVisible(panelTitlebar);
    addAndMakeVisible(labelPanel.get());
}

void TraversalRulesWindow::RulesPanel::addLabel(int fileId, const juce::String& name) {
    labelPanel->addFileLabel(name);
    labelPanel->labels.back()->fileId = fileId;
}

void TraversalRulesWindow::RulesPanel::selectLabel(int fileId) {
    for (auto& label : labelPanel->labels) {
        if (label->fileId == fileId) {
            labelPanel->setSelectedLabel(label.get());
            return;
        }
    }
}

void TraversalRulesWindow::RulesPanel::paint(juce::Graphics& g) {
    const Theme& theme = CustomLookAndFeel::get(*this);

    g.setColour(theme.baseDarkColour2);
    g.fillRect(getLocalBounds());
}

void TraversalRulesWindow::RulesPanel::resized() {
    auto bounds = getLocalBounds();

    resizer.setBounds(bounds.removeFromRight(resizerWidth));
    panelTitlebar.setBounds(bounds.removeFromTop(RulesTitlebar::preferredHeight));
    labelPanel->setBounds(bounds);
}

TraversalRulesWindow::RulesPanel::PanelTitlebar::PanelTitlebar(ApplicationContext& context)
    : Bar(context, { Orientation::horizontal, Background::litFromTop })
{
    addButton = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawAddIcon(g, bounds, state);
        }, context.lookAndFeel);

    addButton->setTooltip("Add Rule");

    addButton->onClick = [this] {
        if (onAddClicked) {
            onAddClicked();
        }
    };

    addAndMakeVisible(addButton.get());
}

void TraversalRulesWindow::RulesPanel::PanelTitlebar::resized() {
    auto bounds = getContentBounds();

    addButton->setBounds(bounds.removeFromLeft(bounds.getHeight()));
}
