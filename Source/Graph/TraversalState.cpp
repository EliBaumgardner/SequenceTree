#include "TraversalState.h"
#include "GraphState.h"
#include "ValueTreeIdentifiers.h"

#include <juce_graphics/juce_graphics.h>

#include <algorithm>
#include <unordered_set>

TraversalState::TraversalState(GraphState& graphState) : graphState(graphState)
{
    map = juce::ValueTree(ValueTreeIdentifiers::TraversalMap);
}

juce::ValueTree TraversalState::addTraversalData(int traversalId, juce::UndoManager* undoManager)
{
    juce::ValueTree existing = map.getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId);

    if (existing.isValid()) {
        return existing;
    }

    juce::ValueTree traversalData {ValueTreeIdentifiers::TraversalData};

    traversalData.setProperty(ValueTreeIdentifiers::TraversalId,        traversalId,      undoManager);
    traversalData.setProperty(ValueTreeIdentifiers::TempoMultiplier,    defaultTempoMult, undoManager);
    traversalData.setProperty(ValueTreeIdentifiers::TraversalColour,    juce::Colours::white.toString(), undoManager);
    traversalData.setProperty(ValueTreeIdentifiers::TraversalChannel,   1,                undoManager);
    traversalData.setProperty(ValueTreeIdentifiers::TraversalTranspose, 0,                undoManager);
    traversalData.setProperty(ValueTreeIdentifiers::TraversalVelocity,  1.0,              undoManager);

    map.addChild(traversalData, -1, undoManager);

    return traversalData;
}

void TraversalState::collectKeys(std::vector<TraversalKey>& keys) const
{
    auto appendKey = [&keys](TraversalKey key) {
        if (key.typeId <= 0) {
            return;
        }

        if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
            keys.push_back(key);
        }
    };

    for (int i = 0; i < graphState.nodeMap.getNumChildren(); ++i) {
        const juce::ValueTree node = graphState.nodeMap.getChild(i);

        if (node.getType() == ValueTreeIdentifiers::TraversalFlagData) {
            int flagValue = node.getProperty(ValueTreeIdentifiers::TraversalFlagValue, 0);

            if (flagValue < 0) {
                flagValue = -flagValue;
            }

            appendKey({ flagValue, (int) node.getProperty(ValueTreeIdentifiers::TraversalInstance, 0) });
            continue;
        }

        const juce::ValueTree equipped = node.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);

        for (int child = 0; child < equipped.getNumChildren(); ++child) {
            const juce::ValueTree reference = equipped.getChild(child);

            appendKey({ (int) reference.getProperty(ValueTreeIdentifiers::TraversalId),
                        (int) reference.getProperty(ValueTreeIdentifiers::TraversalInstance, 0) });
        }
    }
}

int TraversalState::unusedInstance(int traversalTypeId) const
{
    std::vector<TraversalKey> equippedKeys;
    collectKeys(equippedKeys);

    std::unordered_set<int> usedInstances;

    for (const TraversalKey& key : equippedKeys) {
        if (key.typeId == traversalTypeId) {
            usedInstances.insert(key.instance);
        }
    }

    int instance = 0;

    while (instance < TraversalKey::maxInstances - 1 && usedInstances.count(instance) > 0) {
        instance = instance + 1;
    }

    return instance;
}

juce::ValueTree TraversalState::findReference(const juce::ValueTree& references, const TraversalKey& key)
{
    for (int i = 0; i < references.getNumChildren(); ++i) {
        const juce::ValueTree reference = references.getChild(i);

        const TraversalKey referencedKey { (int) reference.getProperty(ValueTreeIdentifiers::TraversalId),
                                           (int) reference.getProperty(ValueTreeIdentifiers::TraversalInstance, 0) };

        if (referencedKey == key) {
            return reference;
        }
    }

    return {};
}
