#include "TraversalRuleState.h"
#include "ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>
#include "../Script/ScriptCompiler.h"

TraversalRuleState::TraversalRuleState()
{
    rules   = juce::ValueTree(ValueTreeIdentifiers::TraversalRules);
    folders = juce::ValueTree(ValueTreeIdentifiers::TraversalRuleFolders);
}

juce::ValueTree TraversalRuleState::addRule(int folderId, juce::UndoManager* undoManager)
{
    juce::ValueTree rule {ValueTreeIdentifiers::TraversalRuleData};

    ++ruleIdIncrement;

    rule.setProperty(ArsNovaUI::CanvasIdentifiers::Id,   ruleIdIncrement, undoManager);
    rule.setProperty(ValueTreeIdentifiers::RuleName,     "rule " + juce::String(ruleIdIncrement), undoManager);
    rule.setProperty(ValueTreeIdentifiers::RuleSource,   defaultTraversalScriptSource(), undoManager);
    rule.setProperty(ValueTreeIdentifiers::RuleFolderId, folderId, undoManager);

    rules.addChild(rule, -1, undoManager);

    return rule;
}

juce::ValueTree TraversalRuleState::addFolder(int parentFolderId, juce::UndoManager* undoManager)
{
    juce::ValueTree folder {ValueTreeIdentifiers::TraversalRuleFolder};

    ++folderIdIncrement;

    folder.setProperty(ValueTreeIdentifiers::FolderId,       folderIdIncrement, undoManager);
    folder.setProperty(ValueTreeIdentifiers::FolderName,     "folder " + juce::String(folderIdIncrement), undoManager);
    folder.setProperty(ValueTreeIdentifiers::FolderParentId, parentFolderId, undoManager);

    folders.addChild(folder, -1, undoManager);

    return folder;
}

void TraversalRuleState::removeRule(int ruleId, juce::UndoManager* undoManager)
{
    juce::ValueTree rule = rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, ruleId);

    if (! rule.isValid()) {
        return;
    }

    rules.removeChild(rule, undoManager);

    if (static_cast<int>(rules.getProperty(ValueTreeIdentifiers::ActiveRuleId, -1)) != ruleId) {
        return;
    }

    int replacementId = -1;

    const juce::ValueTree replacement = rules.getChild(0);

    if (replacement.isValid()) {
        replacementId = replacement.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    }

    rules.setProperty(ValueTreeIdentifiers::ActiveRuleId, replacementId, undoManager);
}

void TraversalRuleState::removeFolder(int folderId, juce::UndoManager* undoManager)
{
    const juce::ValueTree folder = folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, folderId);

    const int liveRuleId = rules.getProperty(ValueTreeIdentifiers::ActiveRuleId, -1);
    const int parentId   = folder.getProperty(ValueTreeIdentifiers::FolderParentId, 0);

    std::vector<int> removedFolderIds;
    std::vector<int> removedRuleIds;

    if (! folder.isValid()) {
        return;
    }

    for (int i = 0; i < folders.getNumChildren(); ++i) {
        const int id = folders.getChild(i).getProperty(ValueTreeIdentifiers::FolderId);

        if (isInsideFolder(id, folderId)) {
            removedFolderIds.push_back(id);
        }
    }

    for (int i = 0; i < rules.getNumChildren(); ++i) {
        juce::ValueTree rule = rules.getChild(i);

        const int ruleId       = rule.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
        const int ruleFolderId = rule.getProperty(ValueTreeIdentifiers::RuleFolderId, 0);

        if (! isInsideFolder(ruleFolderId, folderId)) {
            continue;
        }

        if (ruleId == liveRuleId) {
            rule.setProperty(ValueTreeIdentifiers::RuleFolderId, parentId, undoManager);

            continue;
        }

        removedRuleIds.push_back(ruleId);
    }

    for (int removedId : removedRuleIds) {
        rules.removeChild(rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, removedId), undoManager);
    }

    for (int removedId : removedFolderIds) {
        folders.removeChild(folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, removedId), undoManager);
    }
}

void TraversalRuleState::setRuleSource(int ruleId, const juce::String& source,
                                       juce::UndoManager* undoManager)
{
    juce::ValueTree rule = rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, ruleId);

    if (rule.isValid()) {
        rule.setProperty(ValueTreeIdentifiers::RuleSource, source, undoManager);
    }
}

void TraversalRuleState::setRuleName(int ruleId, const juce::String& name, juce::UndoManager* undoManager)
{
    juce::ValueTree rule = rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, ruleId);

    if (rule.isValid()) {
        rule.setProperty(ValueTreeIdentifiers::RuleName, name, undoManager);
    }
}

void TraversalRuleState::setFolderName(int folderId, const juce::String& name, juce::UndoManager* undoManager)
{
    juce::ValueTree folder = folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, folderId);

    if (folder.isValid()) {
        folder.setProperty(ValueTreeIdentifiers::FolderName, name, undoManager);
    }
}

void TraversalRuleState::moveRule(int ruleId, int folderId, int beforeRuleId, juce::UndoManager* undoManager)
{
    juce::ValueTree       rule   = rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, ruleId);
    const juce::ValueTree anchor = rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, beforeRuleId);

    int targetIndex = rules.getNumChildren() - 1;

    if (! rule.isValid() || ruleId == beforeRuleId) {
        return;
    }

    if (anchor.isValid()) {
        targetIndex = rules.indexOf(anchor);

        if (rules.indexOf(rule) < targetIndex) {
            --targetIndex;
        }
    }

    rule.setProperty(ValueTreeIdentifiers::RuleFolderId, folderId, undoManager);

    rules.moveChild(rules.indexOf(rule), targetIndex, undoManager);
}

void TraversalRuleState::moveFolder(int folderId, int parentFolderId, int beforeFolderId, juce::UndoManager* undoManager)
{
    juce::ValueTree       folder = folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, folderId);
    const juce::ValueTree anchor = folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, beforeFolderId);

    int targetIndex = folders.getNumChildren() - 1;

    if (! folder.isValid() || folderId == beforeFolderId || isInsideFolder(parentFolderId, folderId)) {
        return;
    }

    if (anchor.isValid()) {
        targetIndex = folders.indexOf(anchor);

        if (folders.indexOf(folder) < targetIndex) {
            --targetIndex;
        }
    }

    folder.setProperty(ValueTreeIdentifiers::FolderParentId, parentFolderId, undoManager);

    folders.moveChild(folders.indexOf(folder), targetIndex, undoManager);
}

void TraversalRuleState::ensureDefaultRule()
{
    if (rules.getNumChildren() == 0) {
        addRule(0, nullptr);
    }

    const juce::ValueTree activeRule =
        rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, rules.getProperty(ValueTreeIdentifiers::ActiveRuleId, -1));

    if (! activeRule.isValid()) {
        rules.setProperty(ValueTreeIdentifiers::ActiveRuleId, rules.getChild(0).getProperty(ArsNovaUI::CanvasIdentifiers::Id), nullptr);
    }
}

void TraversalRuleState::replaceState(const juce::ValueTree& restoredRules, const juce::ValueTree& restoredFolders)
{
    rules.removeAllChildren(nullptr);
    rules.removeAllProperties(nullptr);

    folders.removeAllChildren(nullptr);

    for (int i = 0; i < restoredFolders.getNumChildren(); ++i) {
        folders.addChild(restoredFolders.getChild(i).createCopy(), -1, nullptr);
    }

    for (int i = 0; i < restoredRules.getNumChildren(); ++i) {
        rules.addChild(restoredRules.getChild(i).createCopy(), -1, nullptr);
    }

    if (restoredRules.hasProperty(ValueTreeIdentifiers::ActiveRuleId)) {
        rules.setProperty(ValueTreeIdentifiers::ActiveRuleId, restoredRules.getProperty(ValueTreeIdentifiers::ActiveRuleId), nullptr);
    }

    ruleIdIncrement = 0;

    for (int i = 0; i < rules.getNumChildren(); ++i) {
        const int id = rules.getChild(i).getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        if (id > ruleIdIncrement) {
            ruleIdIncrement = id;
        }
    }

    folderIdIncrement = 0;

    for (int i = 0; i < folders.getNumChildren(); ++i) {
        const int id = folders.getChild(i).getProperty(ValueTreeIdentifiers::FolderId);

        if (id > folderIdIncrement) {
            folderIdIncrement = id;
        }
    }
}

bool TraversalRuleState::isInsideFolder(int folderId, int ancestorId) const
{
    int current = folderId;

    for (int step = 0; step <= folders.getNumChildren() && current > 0; ++step) {
        if (current == ancestorId) {
            return true;
        }

        current = folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, current).getProperty(ValueTreeIdentifiers::FolderParentId, 0);
    }

    return false;
}

int TraversalRuleState::folderOfRule(int ruleId) const
{
    const int folderId = rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, ruleId).getProperty(ValueTreeIdentifiers::RuleFolderId, 0);

    if (folderId > 0 && folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, folderId).isValid()) {
        return folderId;
    }

    return 0;
}

int TraversalRuleState::parentOfFolder(int folderId) const
{
    const int parentId = folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, folderId).getProperty(ValueTreeIdentifiers::FolderParentId, 0);

    if (parentId > 0 && folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, parentId).isValid()) {
        return parentId;
    }

    return 0;
}

int TraversalRuleState::countRulesInside(int folderId) const
{
    int count = 0;

    for (int i = 0; i < rules.getNumChildren(); ++i) {
        const int ruleId = rules.getChild(i).getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        if (isInsideFolder(folderOfRule(ruleId), folderId)) {
            ++count;
        }
    }

    return count;
}

juce::String TraversalRuleState::activeRuleSource() const
{
    const juce::ValueTree rule =
        rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, rules.getProperty(ValueTreeIdentifiers::ActiveRuleId, -1));

    if (! rule.isValid()) {
        return {};
    }

    return rule.getProperty(ValueTreeIdentifiers::RuleSource).toString();
}

juce::StringArray TraversalRuleState::pathOf(int ruleId) const
{
    const juce::ValueTree rule = rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, ruleId);

    juce::StringArray segments;

    int folderId = folderOfRule(ruleId);

    if (! rule.isValid()) {
        return segments;
    }

    segments.add(rule.getProperty(ValueTreeIdentifiers::RuleName).toString());

    for (int step = 0; step <= folders.getNumChildren() && folderId > 0; ++step) {
        segments.insert(0, folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, folderId).getProperty(ValueTreeIdentifiers::FolderName).toString());

        folderId = parentOfFolder(folderId);
    }

    return segments;
}
