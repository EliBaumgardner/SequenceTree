#include "RuleTreePanel.h"
#include <ArsNovaUI/Menus/ContextMenu.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

RuleTreePanel::RuleTreePanel(TraversalRuleState& traversalRuleState, juce::UndoManager& undoManager)
    : treeView(traversalRuleState, undoManager)
{
    addButton.icon     = &ArsNovaUI::CustomLookAndFeel::drawAddIcon;
    moreButton.icon    = &ArsNovaUI::CustomLookAndFeel::drawMoreIcon;
    addButton.onClick  = [this] { treeView.onRuleAdded(treeView.folderForNewItem()); };
    moreButton.onClick = [this] { showMoreMenu(); };

    addButton.setTooltip("New rule");
    moreButton.setTooltip("More actions");

    filterEditor.onTextChange = [this] {
        treeView.filterText = filterEditor.getText().trim();

        treeView.rebuild();
    };

    filterEditor.setColour(juce::TextEditor::backgroundColourId,     juce::Colours::transparentBlack);
    filterEditor.setColour(juce::TextEditor::outlineColourId,        juce::Colours::transparentBlack);
    filterEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    filterEditor.setJustification(juce::Justification::centredLeft);
    filterEditor.setIndents(0, 0);
    filterEditor.setEscapeAndReturnKeysConsumed(false);

    viewport.setViewedComponent(&treeView, false);
    viewport.setScrollBarsShown(true, false);

    addAndMakeVisible(addButton);
    addAndMakeVisible(moreButton);
    addAndMakeVisible(filterEditor);
    addAndMakeVisible(viewport);
}

void RuleTreePanel::paint(juce::Graphics& graphics)
{
    const int separatorX     = ArsNovaUI::RowList::rowInset / 2;
    const int separatorWidth = getWidth() - ArsNovaUI::RowList::rowInset;

    graphics.setColour(ArsNovaUI::CustomLookAndFeel::get(*this).borderColour);
    graphics.fillRect(separatorX, filterEditor.getBottom(), separatorWidth, separatorHeight);
}

void RuleTreePanel::resized()
{
    auto bounds    = getLocalBounds();
    auto header    = bounds.removeFromTop(ArsNovaUI::Theme::barHeight).reduced(headerInset);
    auto filterRow = bounds.removeFromTop(ArsNovaUI::RowList::rowHeight);

    const int buttonSize   = header.getHeight();
    const int filterIndent = ArsNovaUI::RowList::rowInset + ArsNovaUI::RowList::leadingWidth;

    moreButton.setBounds(header.removeFromRight(buttonSize));

    header.removeFromRight(ArsNovaUI::Theme::buttonGap);

    addButton.setBounds(header.removeFromRight(buttonSize));

    bounds.removeFromTop(separatorHeight);

    filterEditor.setBounds(filterRow.withTrimmedLeft(filterIndent).withTrimmedRight(ArsNovaUI::RowList::rowInset));
    viewport.setBounds(bounds);

    treeView.minimumHeight = viewport.getMaximumVisibleHeight();

    const int treeHeight = juce::jmax(treeView.minimumHeight, treeView.getHeight());

    treeView.setSize(viewport.getMaximumVisibleWidth(), treeHeight);

    treeView.rebuild();
}

void RuleTreePanel::lookAndFeelChanged()
{
    const ArsNovaUI::Theme& theme = ArsNovaUI::CustomLookAndFeel::get(*this);

    filterEditor.setColour(juce::TextEditor::textColourId, theme.softTextColour);
    filterEditor.applyColourToAllText(theme.softTextColour);
    filterEditor.applyFontToAllText(theme.font(ArsNovaUI::Theme::FontStyle::Regular, ArsNovaUI::RowList::rowFontHeight));
    filterEditor.setTextToShowWhenEmpty("Filter", theme.mutedTextColour);
}

void RuleTreePanel::showMoreMenu()
{
    ArsNovaUI::ContextMenu menu;

    menu.addItem("new rule", ArsNovaUI::ContextMenu::ItemKind::Action, [this] { treeView.onRuleAdded(treeView.folderForNewItem()); });
    menu.addItem("new folder", ArsNovaUI::ContextMenu::ItemKind::Action, [this] { treeView.addFolder(treeView.folderForNewItem()); });
    menu.addSeparator();
    menu.addItem("expand all folders", ArsNovaUI::ContextMenu::ItemKind::Action, [this] { treeView.showFolders(RuleTreeView::FolderDisplay::Expanded); });
    menu.addItem("collapse all folders", ArsNovaUI::ContextMenu::ItemKind::Action, [this] { treeView.showFolders(RuleTreeView::FolderDisplay::Collapsed); });

    menu.show(moreButton);
}
