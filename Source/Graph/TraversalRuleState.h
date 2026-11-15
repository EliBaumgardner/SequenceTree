#pragma once

#include <juce_data_structures/juce_data_structures.h>

/*
    The store of traversal rule scripts, kept apart from the graph. A traversal rule is a script,
    compiled by compileTraversalScript, that chooses which child a traversal moves to next. rules
    holds one TraversalRuleData tree per script with its id, name, source text and the folder it
    sits in, in the order the user arranged them, and its ActiveRuleId property names the one
    script playback uses. folders holds one TraversalRuleFolder tree per folder with its id, name
    and parent folder; folder 0 is the top level, and a rule or folder whose folder is missing
    counts as top level.

    addRule appends a script holding defaultTraversalScriptSource to a folder, and addFolder adds a
    folder inside another. removeRule deletes one script and, if it was active, makes the first
    remaining script active. removeFolder deletes a folder, the folders inside it and their
    scripts, except the active script, which moves to the removed folder's parent. setRuleSource,
    setRuleName and setFolderName replace a text. moveRule and moveFolder put an item in a folder,
    before another item or last; a folder never moves into itself or a folder inside it, which
    isInsideFolder tells. folderOfRule and parentOfFolder give the folder an item sits in, with a
    missing folder counted as the top level, countRulesInside counts the scripts in a folder and
    the folders inside it, and pathOf lists a script's folder names from the top level down,
    ending with its own name. ensureDefaultRule guarantees at least one script exists and one is
    active. replaceState loads saved scripts and folders, and activeRuleSource returns the active
    script's text.

    It is kept apart from GraphState and saved alongside it. The processor owns it,
    TraversalRulesWindow edits it, and AudioSnapshotPublisher compiles activeRuleSource and hands
    the result to the audio thread.
*/

class TraversalRuleState {

public:

    TraversalRuleState();

    juce::ValueTree addRule  (int folderId, juce::UndoManager* undoManager);
    juce::ValueTree addFolder(int parentFolderId, juce::UndoManager* undoManager);

    void removeRule  (int ruleId, juce::UndoManager* undoManager);
    void removeFolder(int folderId, juce::UndoManager* undoManager);

    void setRuleSource(int ruleId, const juce::String& source, juce::UndoManager* undoManager);
    void setRuleName  (int ruleId, const juce::String& name, juce::UndoManager* undoManager);
    void setFolderName(int folderId, const juce::String& name, juce::UndoManager* undoManager);

    void moveRule  (int ruleId, int folderId, int beforeRuleId, juce::UndoManager* undoManager);
    void moveFolder(int folderId, int parentFolderId, int beforeFolderId, juce::UndoManager* undoManager);

    void ensureDefaultRule ();
    void replaceState(const juce::ValueTree& restoredRules, const juce::ValueTree& restoredFolders);

    bool isInsideFolder(int folderId, int ancestorId) const;

    int folderOfRule    (int ruleId) const;
    int parentOfFolder  (int folderId) const;
    int countRulesInside(int folderId) const;

    juce::String activeRuleSource() const;

    juce::StringArray pathOf(int ruleId) const;

    juce::ValueTree rules;
    juce::ValueTree folders;

private:

    int ruleIdIncrement   = 0;
    int folderIdIncrement = 0;
};
