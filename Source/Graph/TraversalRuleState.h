#pragma once

#include <juce_data_structures/juce_data_structures.h>

class TraversalRuleState {

public:

    TraversalRuleState();

    juce::ValueTree addRule   (juce::UndoManager* undoManager);
    void            removeRule(int ruleId, juce::UndoManager* undoManager);

    void setRuleSource(int ruleId, const juce::String& source, juce::UndoManager* undoManager);

    juce::String activeRuleSource() const;

    void ensureDefaultRule();

    void replaceState(const juce::ValueTree& restoredRules);

    juce::ValueTree rules;

private:

    int ruleIdIncrement = 0;
};
