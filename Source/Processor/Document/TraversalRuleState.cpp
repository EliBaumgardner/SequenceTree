#include "TraversalRuleState.h"
#include "TypedProperties.h"
#include "ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>
#include "Core/Script/ScriptCompiler.h"

TraversalRuleState::TraversalRuleState()
{
    rules   = juce::ValueTree(ValueTreeIdentifiers::TraversalRules);
    folders = juce::ValueTree(ValueTreeIdentifiers::TraversalRuleFolders);

    activeRuleId.referTo(rules, ValueTreeIdentifiers::ActiveRuleId, nullptr, noActiveRuleId);
}

juce::ValueTree TraversalRuleState::addRule(int folderId, juce::UndoManager* undoManager)
{
    juce::ValueTree rule {ValueTreeIdentifiers::TraversalRuleData};

    RuleProperties properties(rule);

    ++ruleIdIncrement;

    const juce::String ruleName = "rule " + juce::String(ruleIdIncrement);

    rule.setProperty(ArsNovaUI::CanvasIdentifiers::Id, ruleIdIncrement, undoManager);

    properties.ruleName  .setValue(ruleName,                       undoManager);
    properties.ruleSource.setValue(defaultTraversalScriptSource(), undoManager);
    properties.folderId  .setValue(folderId,                       undoManager);

    rules.addChild(rule, -1, undoManager);

    return rule;
}

juce::ValueTree TraversalRuleState::addFolder(int parentFolderId, juce::UndoManager* undoManager)
{
    juce::ValueTree folder {ValueTreeIdentifiers::TraversalRuleFolder};

    FolderProperties properties(folder);

    ++folderIdIncrement;

    const juce::String folderName = "folder " + juce::String(folderIdIncrement);

    properties.folderId      .setValue(folderIdIncrement, undoManager);
    properties.folderName    .setValue(folderName,        undoManager);
    properties.parentFolderId.setValue(parentFolderId,    undoManager);

    folders.addChild(folder, -1, undoManager);

    return folder;
}

void TraversalRuleState::removeRule(int ruleId, juce::UndoManager* undoManager)
{
    juce::ValueTree rule = rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, ruleId);

    int replacementId = noActiveRuleId;

    if (! rule.isValid()) {
        return;
    }

    rules.removeChild(rule, undoManager);

    if (activeRuleId != ruleId) {
        return;
    }

    const juce::ValueTree replacement = rules.getChild(0);

    if (replacement.isValid()) {
        replacementId = replacement.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    }

    activeRuleId.setValue(replacementId, undoManager);
}

void TraversalRuleState::removeFolder(int folderId, juce::UndoManager* undoManager)
{
    const juce::ValueTree folder = folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, folderId);

    const FolderProperties properties(folder);

    const int liveRuleId = activeRuleId;
    const int parentId   = properties.parentFolderId;

    std::vector<int> removedFolderIds;
    std::vector<int> removedRuleIds;

    if (! folder.isValid()) {
        return;
    }

    for (int i = 0; i < folders.getNumChildren(); ++i) {
        const FolderProperties child(folders.getChild(i));

        const int id = child.folderId;

        if (isInsideFolder(id, folderId)) {
            removedFolderIds.push_back(id);
        }
    }

    for (int i = 0; i < rules.getNumChildren(); ++i) {
        juce::ValueTree rule = rules.getChild(i);

        RuleProperties ruleProperties(rule);

        const int ruleId       = rule.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
        const int ruleFolderId = ruleProperties.folderId;

        if (! isInsideFolder(ruleFolderId, folderId)) {
            continue;
        }

        if (ruleId == liveRuleId) {
            ruleProperties.folderId.setValue(parentId, undoManager);

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

    RuleProperties properties(rule);

    if (rule.isValid()) {
        properties.ruleSource.setValue(source, undoManager);
    }
}

void TraversalRuleState::setRuleName(int ruleId, const juce::String& name, juce::UndoManager* undoManager)
{
    juce::ValueTree rule = rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, ruleId);

    RuleProperties properties(rule);

    if (rule.isValid()) {
        properties.ruleName.setValue(name, undoManager);
    }
}

void TraversalRuleState::setFolderName(int folderId, const juce::String& name, juce::UndoManager* undoManager)
{
    juce::ValueTree folder = folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, folderId);

    FolderProperties properties(folder);

    if (folder.isValid()) {
        properties.folderName.setValue(name, undoManager);
    }
}

void TraversalRuleState::moveRule(int ruleId, int folderId, int beforeRuleId, juce::UndoManager* undoManager)
{
    juce::ValueTree       rule   = rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, ruleId);
    const juce::ValueTree anchor = rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, beforeRuleId);

    RuleProperties properties(rule);

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

    properties.folderId.setValue(folderId, undoManager);

    rules.moveChild(rules.indexOf(rule), targetIndex, undoManager);
}

void TraversalRuleState::moveFolder(int folderId, int parentFolderId, int beforeFolderId, juce::UndoManager* undoManager)
{
    juce::ValueTree       folder = folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, folderId);
    const juce::ValueTree anchor = folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, beforeFolderId);

    FolderProperties properties(folder);

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

    properties.parentFolderId.setValue(parentFolderId, undoManager);

    folders.moveChild(folders.indexOf(folder), targetIndex, undoManager);
}

void TraversalRuleState::ensureDefaultRule()
{
    if (rules.getNumChildren() == 0) {
        addRule(0, nullptr);
    }

    const int liveRuleId  = activeRuleId;
    const int firstRuleId = rules.getChild(0).getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    const juce::ValueTree activeRule = rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, liveRuleId);

    if (! activeRule.isValid()) {
        activeRuleId.setValue(firstRuleId, nullptr);
    }
}

void TraversalRuleState::replaceState(const juce::ValueTree& restoredRules, const juce::ValueTree& restoredFolders)
{
    juce::ValueTree restoredRulesTree = restoredRules;

    rules.removeAllChildren(nullptr);
    rules.removeAllProperties(nullptr);

    folders.removeAllChildren(nullptr);

    for (int i = 0; i < restoredFolders.getNumChildren(); ++i) {
        folders.addChild(restoredFolders.getChild(i).createCopy(), -1, nullptr);
    }

    for (int i = 0; i < restoredRules.getNumChildren(); ++i) {
        rules.addChild(restoredRules.getChild(i).createCopy(), -1, nullptr);
    }

    const juce::CachedValue<int> restoredActiveRuleId(restoredRulesTree, ValueTreeIdentifiers::ActiveRuleId, nullptr, noActiveRuleId);

    if (! restoredActiveRuleId.isUsingDefault()) {
        activeRuleId.setValue(restoredActiveRuleId, nullptr);
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
        const FolderProperties folder(folders.getChild(i));

        const int id = folder.folderId;

        if (id > folderIdIncrement) {
            folderIdIncrement = id;
        }
    }
}

bool TraversalRuleState::isInsideFolder(int folderId, int ancestorId) const
{
    int current = folderId;

    for (int step = 0; step <= folders.getNumChildren() && current > 0; ++step) {
        const FolderProperties folder(folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, current));

        if (current == ancestorId) {
            return true;
        }

        current = folder.parentFolderId;
    }

    return false;
}

int TraversalRuleState::folderOfRule(int ruleId) const
{
    const RuleProperties rule(rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, ruleId));

    const int folderId = rule.folderId;

    if (folderId > 0 && folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, folderId).isValid()) {
        return folderId;
    }

    return 0;
}

int TraversalRuleState::parentOfFolder(int folderId) const
{
    const FolderProperties folder(folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, folderId));

    const int parentId = folder.parentFolderId;

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
    const int liveRuleId = activeRuleId;

    const juce::ValueTree rule = rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, liveRuleId);

    const RuleProperties properties(rule);

    if (! rule.isValid()) {
        return {};
    }

    return properties.ruleSource;
}

juce::StringArray TraversalRuleState::pathOf(int ruleId) const
{
    const juce::ValueTree rule = rules.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, ruleId);

    const RuleProperties properties(rule);

    juce::StringArray segments;

    int folderId = folderOfRule(ruleId);

    if (! rule.isValid()) {
        return segments;
    }

    segments.add(properties.ruleName);

    for (int step = 0; step <= folders.getNumChildren() && folderId > 0; ++step) {
        const FolderProperties folder(folders.getChildWithProperty(ValueTreeIdentifiers::FolderId, folderId));

        segments.insert(0, folder.folderName);

        folderId = parentOfFolder(folderId);
    }

    return segments;
}
