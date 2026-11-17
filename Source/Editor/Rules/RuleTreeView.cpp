#include "RuleTreeView.h"
#include <ArsNovaUI/Theme/IconLibrary.h>
#include "Processor/Document/TraversalRuleState.h"
#include "Processor/Document/TypedProperties.h"
#include "Processor/Document/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>
#include <ArsNovaUI/Menus/ContextMenu.h>
#include <ArsNovaUI/Menus/Dialog.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

RuleTreeView::RuleTreeView(TraversalRuleState& traversalRuleState, juce::UndoManager& undoManager)
    : traversalRuleState(traversalRuleState),
      undoManager       (undoManager),
      renameEditor      (undoManager),
      reorderGestures   (traversalRuleState, undoManager)
{
    auto nameFormat = std::make_unique<ArsNovaUI::TextFormat>(ArsNovaUI::TextFormat::labelTextLength, ArsNovaUI::TextFormat::labelCharacters);

    nameFormat->rejectsEmptyText = true;
    renameEditor.fontStyle       = ArsNovaUI::Theme::FontStyle::Regular;
    renameEditor.fontHeight      = rowFontHeight;
    renameEditor.setFormat(std::move(nameFormat));
    renameEditor.setJustification(juce::Justification::centredLeft);

    renameEditor.onEditFinished = [this] {
        renamingId = -1;

        renameEditor.setVisible(false);

        rebuild();
    };

    renameEditor.textEditor->onEscapeKey = [this] {
        renameEditor.textEditor->setText(renameEditor.boundValue.toString(), false);

        renameEditor.commitValue();
    };

    addChildComponent(renameEditor);
}

void RuleTreeView::paintOverChildren(juce::Graphics& graphics)
{
    const ArsNovaUI::Theme& theme = ArsNovaUI::CustomLookAndFeel::get(*this);

    const RuleReorderGestures::DropPlacement placement = reorderGestures.dropPlacement;

    const int dropItem = reorderGestures.dropItem;

    int lineY = -1;
    int lineX = rowInset;

    switch (placement) {
        case RuleReorderGestures::DropPlacement::Before: lineY = dropItem * rowHeight;                        break;
        case RuleReorderGestures::DropPlacement::After:  lineY = (dropItem + 1) * rowHeight;                  break;
        case RuleReorderGestures::DropPlacement::End:    lineY = static_cast<int>(items.size()) * rowHeight;  break;
        case RuleReorderGestures::DropPlacement::Inside:
        case RuleReorderGestures::DropPlacement::None:                                                        break;
    }

    if (placement == RuleReorderGestures::DropPlacement::Before || placement == RuleReorderGestures::DropPlacement::After) {
        lineX = rowInset + items[static_cast<size_t>(dropItem)].depth * indentWidth;
    }

    if (lineY >= 0) {
        const int dropLineTop   = lineY - dropLineHeight / 2;
        const int dropLineWidth = getWidth() - lineX - rowInset;

        graphics.setColour(theme.accentColour);
        graphics.fillRect(lineX, dropLineTop, dropLineWidth, dropLineHeight);
    }
}

void RuleTreeView::resized()
{
    for (int index = 0; index < static_cast<int>(items.size()); ++index) {
        const RuleTreeItem& item = items[static_cast<size_t>(index)];

        const int left = rowInset + item.depth * indentWidth + leadingWidth;

        if (renamingId >= 0 && item.kind == renamingKind && item.id == renamingId) {
            const int renameTop   = index * rowHeight;
            const int renameWidth = juce::jmax(0, getWidth() - left - rowInset);

            renameEditor.setBounds(left, renameTop, renameWidth, rowHeight);

            return;
        }
    }

    if (renamingId >= 0) {
        renamingId = -1;

        renameEditor.setVisible(false);
    }
}

void RuleTreeView::mouseDown(const juce::MouseEvent& event)
{
    renameEditor.commitValue();

    const int index = rowIndexAt(event.y);

    reorderGestures.pressedItem = index;
    reorderGestures.dragging    = false;

    if (event.mods.isPopupMenu()) {
        showRowMenu(index);

        return;
    }

    if (index < 0) {
        return;
    }

    selectedKind = items[static_cast<size_t>(index)].kind;
    selectedId   = items[static_cast<size_t>(index)].id;

    styleRows();

    repaint();
}

void RuleTreeView::mouseDrag(const juce::MouseEvent& event)
{
    juce::Viewport* viewport = findParentComponentOfClass<juce::Viewport>();

    const int index = rowIndexAt(event.y);

    float fraction = 0.0f;

    if (reorderGestures.pressedItem < 0 || filterText.isNotEmpty() || event.mods.isPopupMenu()) {
        return;
    }

    if (! reorderGestures.dragging && event.getDistanceFromDragStart() < RuleReorderGestures::dragThreshold) {
        return;
    }

    if (index >= 0) {
        const int rowOffset = event.y - index * rowHeight;

        fraction = static_cast<float>(rowOffset) / static_cast<float>(rowHeight);
    }

    reorderGestures.dragging = true;
    hoveredRow               = -1;

    reorderGestures.updateDrop(items, index, fraction);

    styleRows();

    if (viewport != nullptr) {
        const juce::Point<int> position = event.getEventRelativeTo(viewport).getPosition();

        viewport->autoScroll(position.x, position.y, autoScrollEdge, autoScrollSpeed);
    }

    repaint();
}

void RuleTreeView::mouseUp(const juce::MouseEvent& event)
{
    const int index   = rowIndexAt(event.y);
    const int pressed = reorderGestures.pressedItem;

    RuleTreeItem item;

    if (reorderGestures.dragging) {
        if (reorderGestures.dropPlacement == RuleReorderGestures::DropPlacement::Inside) {
            collapsedFolders.erase(items[static_cast<size_t>(reorderGestures.dropItem)].id);
        }

        reorderGestures.moveDraggedItem(items);
        reorderGestures.reset();

        styleRows();

        repaint();

        return;
    }

    reorderGestures.reset();

    if (pressed < 0 || index != pressed || event.mods.isPopupMenu() || event.getNumberOfClicks() > 1) {
        return;
    }

    item = items[static_cast<size_t>(index)];

    if (item.kind == RuleItemKind::Rule) {
        onRuleOpened(item.id);

        return;
    }

    if (filterText.isNotEmpty()) {
        return;
    }

    if (collapsedFolders.erase(item.id) == 0) {
        collapsedFolders.insert(item.id);
    }

    rebuild();
}

void RuleTreeView::mouseDoubleClick(const juce::MouseEvent& event)
{
    const int index = rowIndexAt(event.y);

    RuleTreeItem item;

    if (index < 0 || event.mods.isPopupMenu()) {
        return;
    }

    item = items[static_cast<size_t>(index)];

    if (item.kind == RuleItemKind::Folder) {
        collapsedFolders.erase(item.id);
    }

    beginRename(item.kind, item.id);
}

void RuleTreeView::rebuild()
{
    rows.clear();
    items.clear();

    addItems(0, 0);

    reorderGestures.reset();

    styleRows();

    layoutRows();

    resized();
}

void RuleTreeView::revealRule(int ruleId)
{
    juce::Viewport* viewport = findParentComponentOfClass<juce::Viewport>();

    int folderId = traversalRuleState.folderOfRule(ruleId);

    for (int step = 0; step <= traversalRuleState.folders.getNumChildren() && folderId > 0; ++step) {
        collapsedFolders.erase(folderId);

        folderId = traversalRuleState.parentOfFolder(folderId);
    }

    selectedKind = RuleItemKind::Rule;
    selectedId   = ruleId;

    rebuild();

    for (int index = 0; viewport != nullptr && index < static_cast<int>(items.size()); ++index) {
        const int top    = index * rowHeight;
        const int viewY  = viewport->getViewPositionY();
        const int height = viewport->getMaximumVisibleHeight();

        if (items[static_cast<size_t>(index)].kind != RuleItemKind::Rule || items[static_cast<size_t>(index)].id != ruleId) {
            continue;
        }

        if (top < viewY) {
            viewport->setViewPosition(0, top);
        }
        else if (top + rowHeight > viewY + height) {
            const int alignedViewY = top + rowHeight - height;

            viewport->setViewPosition(0, alignedViewY);
        }

        return;
    }
}

void RuleTreeView::beginRename(RuleItemKind kind, int id)
{
    juce::ValueTree tree = traversalRuleState.folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, id);

    juce::Identifier property = ValueTreeIdentifiers::FolderName;

    if (kind == RuleItemKind::Rule) {
        tree     = traversalRuleState.rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, id);
        property = ValueTreeIdentifiers::RuleName;
    }

    renameEditor.commitValue();

    renamingKind = kind;
    renamingId   = id;

    rebuild();

    if (renamingId < 0) {
        return;
    }

    renameEditor.bindEditor(tree, property);
    renameEditor.setVisible(true);
    renameEditor.beginEditing();
}

void RuleTreeView::addFolder(int parentFolderId)
{
    undoManager.beginNewTransaction();

    const juce::ValueTree folder = traversalRuleState.addFolder(parentFolderId, &undoManager);

    const FolderProperties properties(folder);

    collapsedFolders.erase(parentFolderId);

    selectedKind = RuleItemKind::Folder;
    selectedId   = properties.folderId;

    beginRename(RuleItemKind::Folder, selectedId);
}

void RuleTreeView::showFolders(FolderDisplay display)
{
    const juce::ValueTree& folders = traversalRuleState.folders;

    collapsedFolders.clear();

    for (int i = 0; display == FolderDisplay::Collapsed && i < folders.getNumChildren(); ++i) {
        const FolderProperties folder(folders.getChild(i));

        const int folderId = folder.folderId;

        collapsedFolders.insert(folderId);
    }

    rebuild();
}

int RuleTreeView::folderForNewItem() const
{
    const bool folderExists = traversalRuleState.folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, selectedId).isValid();

    if (selectedKind == RuleItemKind::Folder && folderExists) {
        return selectedId;
    }

    if (selectedKind == RuleItemKind::Rule) {
        return traversalRuleState.folderOfRule(selectedId);
    }

    return 0;
}

void RuleTreeView::addItems(int parentId, int depth)
{
    const juce::ValueTree& folders = traversalRuleState.folders;
    const juce::ValueTree& rules   = traversalRuleState.rules;

    const bool filtering = filterText.isNotEmpty();

    if (depth > folders.getNumChildren()) {
        return;
    }

    for (int i = 0; i < folders.getNumChildren(); ++i) {
        const FolderProperties folder(folders.getChild(i));

        const int id = folder.folderId;

        RuleTreeItem item;

        Row row;

        if (traversalRuleState.parentOfFolder(id) != parentId || (filtering && ! folderHasMatch(id))) {
            continue;
        }

        item.kind     = RuleItemKind::Folder;
        item.id       = id;
        item.parentId = parentId;
        item.depth    = depth;
        item.expanded = filtering || collapsedFolders.count(id) == 0;
        row.depth     = depth;
        row.icon      = &ArsNovaUI::IconLibrary::get().chevronRight;

        if (item.expanded) {
            row.icon = &ArsNovaUI::IconLibrary::get().chevronDown;
        }

        if (renamingKind != RuleItemKind::Folder || renamingId != id) {
            row.text = folder.folderName;
        }

        items.push_back(item);
        rows.push_back(row);

        if (item.expanded) {
            const int childDepth = depth + 1;

            addItems(id, childDepth);
        }
    }

    for (int i = 0; i < rules.getNumChildren(); ++i) {
        const juce::ValueTree rule = rules.getChild(i);

        const RuleProperties properties(rule);

        const int id = rule.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        RuleTreeItem item;

        Row row;

        if (traversalRuleState.folderOfRule(id) != parentId || (filtering && ! properties.ruleName.get().containsIgnoreCase(filterText))) {
            continue;
        }

        item.id       = id;
        item.parentId = parentId;
        item.depth    = depth;
        row.depth     = depth;
        row.marked    = id == liveRuleId;

        if (renamingKind != RuleItemKind::Rule || renamingId != id) {
            row.text = properties.ruleName;
        }

        items.push_back(item);
        rows.push_back(row);
    }
}

void RuleTreeView::styleRows()
{
    for (int index = 0; index < static_cast<int>(items.size()); ++index) {
        const RuleTreeItem& item = items[static_cast<size_t>(index)];

        Row& row = rows[static_cast<size_t>(index)];

        const bool isSelected   = item.kind == selectedKind && item.id == selectedId;
        const bool isDropTarget = reorderGestures.dropPlacement == RuleReorderGestures::DropPlacement::Inside && index == reorderGestures.dropItem;
        const bool isRenaming   = renamingId >= 0 && item.kind == renamingKind && item.id == renamingId;
        const bool isViewed     = item.kind == RuleItemKind::Rule && item.id == viewedRuleId;

        row.style    = RowStyle::Item;
        row.fill     = RowFill::None;
        row.outlined = isDropTarget || isRenaming;

        if (isDropTarget) {
            row.fill = RowFill::Accent;
        }
        else if (isRenaming || isSelected) {
            row.fill = RowFill::Selected;
        }

        if (isSelected || isDropTarget || isViewed) {
            row.style = RowStyle::Emphasised;
        }
    }
}

void RuleTreeView::showRowMenu(int itemIndex)
{
    ArsNovaUI::ContextMenu menu;

    RuleTreeItem item;

    int targetFolder = 0;

    if (itemIndex >= 0) {
        item         = items[static_cast<size_t>(itemIndex)];
        targetFolder = item.parentId;
    }

    if (itemIndex >= 0 && item.kind == RuleItemKind::Folder) {
        targetFolder = item.id;
    }

    if (itemIndex >= 0 && item.kind == RuleItemKind::Rule) {
        menu.addItem("open", ArsNovaUI::ContextMenu::ItemKind::Action, [this, item] { onRuleOpened(item.id); });
        menu.addItem("make live", ArsNovaUI::ContextMenu::ItemKind::Action, [this, item] { onRuleMadeLive(item.id); }, item.id != liveRuleId);
        menu.addSeparator();
    }

    menu.addItem("new rule", ArsNovaUI::ContextMenu::ItemKind::Action, [this, targetFolder] { onRuleAdded(targetFolder); });
    menu.addItem("new folder", ArsNovaUI::ContextMenu::ItemKind::Action, [this, targetFolder] { addFolder(targetFolder); });

    if (itemIndex >= 0) {
        menu.addSeparator();
        menu.addItem("rename", ArsNovaUI::ContextMenu::ItemKind::Action, [this, item] { beginRename(item.kind, item.id); });
        menu.addItem("delete...", ArsNovaUI::ContextMenu::ItemKind::Action, [this, item] { confirmRemoval(item); });
    }

    menu.show(*this);
}

void RuleTreeView::confirmRemoval(const RuleTreeItem& item)
{
    const juce::Component::SafePointer<RuleTreeView> safeThis(this);

    const RuleItemKind kind = item.kind;

    const int id = item.id;

    int ruleCount = 0;

    const bool holdsLiveRule = kind == RuleItemKind::Folder && traversalRuleState.isInsideFolder(traversalRuleState.folderOfRule(liveRuleId), id);

    const RuleProperties rule(traversalRuleState.rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, id));

    const FolderProperties folder(traversalRuleState.folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, id));

    juce::String name = rule.ruleName;
    juce::String noun = "rules";
    juce::String message;

    ArsNovaUI::Dialog dialog;

    if (kind == RuleItemKind::Folder) {
        name      = folder.folderName;
        ruleCount = traversalRuleState.countRulesInside(id);
    }

    if (holdsLiveRule) {
        --ruleCount;
    }

    if (kind == RuleItemKind::Folder && ruleCount == 0 && ! holdsLiveRule) {
        onFolderRemoved(id);

        return;
    }

    if (ruleCount == 1) {
        noun = "rule";
    }

    message = "Delete the rule \"" + name + "\"?";

    if (kind == RuleItemKind::Folder) {
        message = "Delete the folder \"" + name + "\" and the " + juce::String(ruleCount) + " " + noun + " in it?";
    }

    if (holdsLiveRule) {
        message += " The live rule moves out of the folder instead of being deleted.";
    }

    dialog.title   = "Delete";
    dialog.message = message;

    dialog.addButton("Delete", [safeThis, kind, id]() {
        if (safeThis == nullptr) {
            return;
        }

        if (kind == RuleItemKind::Rule) {
            safeThis->onRuleRemoved(id);
        }
        else {
            safeThis->onFolderRemoved(id);
        }
    });

    dialog.addButton("Cancel", nullptr);
    dialog.show(*this);
}

bool RuleTreeView::folderHasMatch(int folderId) const
{
    const juce::ValueTree& rules = traversalRuleState.rules;

    for (int i = 0; i < rules.getNumChildren(); ++i) {
        const juce::ValueTree rule = rules.getChild(i);

        const int ruleId = rule.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        const RuleProperties properties(rule);

        const juce::String name = properties.ruleName;

        if (name.containsIgnoreCase(filterText)
            && traversalRuleState.isInsideFolder(traversalRuleState.folderOfRule(ruleId), folderId)) {
            return true;
        }
    }

    return false;
}
