#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <ArsNovaUI/RowList.h>
#include <ArsNovaUI/Editors/ValueEditor.h>

#include <set>

#include "RuleReorderGestures.h"

class RuleTreeView : public ArsNovaUI::RowList
{
public:

    enum class FolderDisplay;

    RuleTreeView(TraversalRuleState& traversalRuleState, juce::UndoManager& undoManager);

    void paintOverChildren(juce::Graphics& graphics) override;
    void resized () override;

    void mouseDown       (const juce::MouseEvent& event) override;
    void mouseDrag       (const juce::MouseEvent& event) override;
    void mouseUp         (const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

    void rebuild ();
    void revealRule (int ruleId);
    void beginRename(RuleItemKind kind, int id);
    void addFolder  (int parentFolderId);
    void showFolders(FolderDisplay display);

    int folderForNewItem() const;

    enum class FolderDisplay { Expanded, Collapsed };

    static constexpr int dropLineHeight  = 2;
    static constexpr int autoScrollEdge  = 20;
    static constexpr int autoScrollSpeed = 6;

    std::function<void(int)> onRuleOpened;
    std::function<void(int)> onRuleAdded;
    std::function<void(int)> onRuleRemoved;
    std::function<void(int)> onFolderRemoved;
    std::function<void(int)> onRuleMadeLive;

    juce::String filterText;

    int viewedRuleId = -1;
    int liveRuleId   = -1;

private:

    void addItems      (int parentId, int depth);
    void styleRows ();
    void showRowMenu   (int itemIndex);
    void confirmRemoval(const RuleTreeItem& item);

    bool folderHasMatch(int folderId) const;

    struct ItemRef
    {
        bool operator==(const ItemRef&) const = default;

        RuleItemKind kind = RuleItemKind::Rule;

        int id = -1;
    };

    TraversalRuleState& traversalRuleState;
    juce::UndoManager&  undoManager;

    ArsNovaUI::ValueEditor renameEditor;
    RuleReorderGestures    reorderGestures;

    std::vector<RuleTreeItem> items;

    std::set<int> collapsedFolders;

    ItemRef selected;
    ItemRef renaming;
};
