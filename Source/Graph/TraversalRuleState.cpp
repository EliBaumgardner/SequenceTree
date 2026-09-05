#include "TraversalRuleState.h"
#include "ValueTreeIdentifiers.h"
#include "../Script/ScriptCompiler.h"

TraversalRuleState::TraversalRuleState()
{
    rules = juce::ValueTree(ValueTreeIdentifiers::TraversalRules);
}

void TraversalRuleState::replaceState(const juce::ValueTree& restoredRules)
{
    rules.removeAllChildren(nullptr);
    rules.removeAllProperties(nullptr);

    for (int i = 0; i < restoredRules.getNumChildren(); ++i) {
        rules.addChild(restoredRules.getChild(i).createCopy(), -1, nullptr);
    }

    if (restoredRules.hasProperty(ValueTreeIdentifiers::ActiveRuleId)) {
        rules.setProperty(ValueTreeIdentifiers::ActiveRuleId,
                          restoredRules.getProperty(ValueTreeIdentifiers::ActiveRuleId),
                          nullptr);
    }

    ruleIdIncrement = 0;

    for (int i = 0; i < rules.getNumChildren(); ++i) {
        const int id = rules.getChild(i).getProperty(ValueTreeIdentifiers::Id);

        if (id > ruleIdIncrement) {
            ruleIdIncrement = id;
        }
    }
}

juce::ValueTree TraversalRuleState::addRule(juce::UndoManager* undoManager)
{
    ++ruleIdIncrement;

    juce::ValueTree rule {ValueTreeIdentifiers::TraversalRuleData};

    rule.setProperty(ValueTreeIdentifiers::Id,         ruleIdIncrement, undoManager);
    rule.setProperty(ValueTreeIdentifiers::RuleName,   "rule " + juce::String(ruleIdIncrement), undoManager);
    rule.setProperty(ValueTreeIdentifiers::RuleSource, defaultTraversalScriptSource(), undoManager);

    rules.addChild(rule, -1, undoManager);

    return rule;
}

void TraversalRuleState::removeRule(int ruleId, juce::UndoManager* undoManager)
{
    juce::ValueTree rule = rules.getChildWithProperty(ValueTreeIdentifiers::Id, ruleId);

    if (! rule.isValid()) {
        return;
    }

    rules.removeChild(rule, undoManager);

    if ((int) rules.getProperty(ValueTreeIdentifiers::ActiveRuleId, -1) != ruleId) {
        return;
    }

    int replacementId = -1;

    const juce::ValueTree replacement = rules.getChild(0);

    if (replacement.isValid()) {
        replacementId = replacement.getProperty(ValueTreeIdentifiers::Id);
    }

    rules.setProperty(ValueTreeIdentifiers::ActiveRuleId, replacementId, undoManager);
}

void TraversalRuleState::setRuleSource(int ruleId, const juce::String& source,
                                       juce::UndoManager* undoManager)
{
    juce::ValueTree rule = rules.getChildWithProperty(ValueTreeIdentifiers::Id, ruleId);

    if (rule.isValid()) {
        rule.setProperty(ValueTreeIdentifiers::RuleSource, source, undoManager);
    }
}

juce::String TraversalRuleState::activeRuleSource() const
{
    const juce::ValueTree rule =
        rules.getChildWithProperty(ValueTreeIdentifiers::Id,
                                   rules.getProperty(ValueTreeIdentifiers::ActiveRuleId, -1));

    if (! rule.isValid()) {
        return {};
    }

    return rule.getProperty(ValueTreeIdentifiers::RuleSource).toString();
}

void TraversalRuleState::ensureDefaultRule()
{
    if (rules.getNumChildren() == 0) {
        addRule(nullptr);
    }

    const juce::ValueTree activeRule =
        rules.getChildWithProperty(ValueTreeIdentifiers::Id,
                                   rules.getProperty(ValueTreeIdentifiers::ActiveRuleId, -1));

    if (! activeRule.isValid()) {
        rules.setProperty(ValueTreeIdentifiers::ActiveRuleId,
                          rules.getChild(0).getProperty(ValueTreeIdentifiers::Id),
                          nullptr);
    }
}
