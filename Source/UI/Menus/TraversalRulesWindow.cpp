//
// Created by Eli Baumgardner on 7/21/26.
//

#include "TraversalRulesWindow.h"
#include "../Theme/CustomLookAndFeel.h"

TraversalRulesWindow::TraversalRulesWindow(ApplicationContext& context) : context(context),
    titlebar(context), rulesPanel(context)
{
    setLookAndFeel(context.lookAndFeel);

    rulesPanel.onWidthDragged = [this](int newWidth) { setPanelWidth(newWidth); };

    filePageViewport.setScrollBarsShown(true, false);

    addAndMakeVisible(titlebar);
    addAndMakeVisible(rulesPanel);
    addAndMakeVisible(filePageViewport);

    rulesPanel.propagateLabelClicked = [this](int fileId) {\
        DBG("setting active page to " << fileId);
        setActivePage(fileId);
    };

}

TraversalRulesWindow::~TraversalRulesWindow() {
    setLookAndFeel(nullptr);
}

void TraversalRulesWindow::paint(juce::Graphics& g) {
    const Theme& theme = CustomLookAndFeel::get(*this);

    g.setColour(theme.baseDarkColour2);
    g.fillRect(getLocalBounds());

    g.setColour(juce::Colours::black);
    g.drawRect(getLocalBounds(), 1);
}

void TraversalRulesWindow::resized() {
    panelWidth = clampPanelWidth(panelWidth);

    auto bounds = getLocalBounds();

    rulesPanel.setBounds(bounds.removeFromLeft(panelWidth));
    titlebar.setBounds(bounds.removeFromTop(RulesTitlebar::preferredHeight));

    filePageViewport.setBounds(bounds);

    if (activePage != nullptr) {
        const int pageWidth = filePageViewport.getMaximumVisibleWidth();

        activePage->setSize(pageWidth, activePage->preferredHeightForWidth(pageWidth));
    }
}

void TraversalRulesWindow::createNewPage(int id) {
    filePages.emplace(id, std::make_unique<FilePage>(context));
}

void TraversalRulesWindow::setActivePage(int id) {
    const auto match = filePages.find(id);

    if (match == filePages.end())
        return;

    FilePage* page = match->second.get();

    if (page == activePage) {
        return;
    }

    activePage = page;
    filePageViewport.setViewedComponent(page, false);

    resized();
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

    playButton->setTooltip("Run Rules");

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
        fileIdIncrement++;

        labelPanel->addFileLabel("rule " + juce::String(fileIdIncrement));
        labelPanel->labels.back()->fileId = fileIdIncrement;

        if (auto* parentComponent = dynamic_cast<TraversalRulesWindow*>(getParentComponent())) {
            DBG("parentComponent is creating new pages");
            parentComponent->createNewPage(fileIdIncrement);
        }
    };

    labelPanel->onLabelClicked = [this](FileLabel* label) {
        DBG("propagating label clicked");
        int fileId = label->fileId;
        propagateLabelClicked(fileId);
    };

    addAndMakeVisible(panelTitlebar);
    addAndMakeVisible(labelPanel.get());
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
