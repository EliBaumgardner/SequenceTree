#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include <span>

/*
    The store of traversal rule scripts. A traversal rule is a script, compiled by
    compileTraversalScript, that chooses which child a traversal moves to next. rules holds one
    TraversalRuleData tree per script with its id, name and source text, in the order the user
    arranged them, and its ActiveRuleId property names the one script playback uses.

    addRule appends a script holding defaultTraversalScriptSource. removeRule deletes one and, if
    it was active, makes the first remaining script active. setRuleSource replaces a script's text
    and reorderRules puts the scripts in the given order. ensureDefaultRule guarantees at least one
    script exists and one is active. replaceState loads saved scripts, and activeRuleSource returns
    the active script's text.

    It is kept apart from GraphState and saved alongside it. The processor owns it,
    TraversalRulesWindow edits it, and AudioSnapshotPublisher compiles activeRuleSource and hands
    the result to the audio thread.
*/

class TraversalRuleState {

public:

    TraversalRuleState();

    juce::ValueTree addRule(juce::UndoManager* undoManager);

    void removeRule   (int ruleId, juce::UndoManager* undoManager);
    void setRuleSource(int ruleId, const juce::String& source, juce::UndoManager* undoManager);
    void reorderRules (std::span<const int> ruleIds, juce::UndoManager* undoManager);

    void ensureDefaultRule();
    void replaceState     (const juce::ValueTree& restoredRules);

    juce::String activeRuleSource() const;

    juce::ValueTree rules;

private:

    int ruleIdIncrement = 0;
};
