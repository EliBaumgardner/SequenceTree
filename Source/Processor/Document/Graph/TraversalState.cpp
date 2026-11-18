#include "TraversalState.h"
#include "GraphState.h"
#include "Processor/Document/TypedProperties.h"
#include "Processor/Document/ValueTreeIdentifiers.h"

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

    TraversalProperties properties(traversalData);

    const juce::String defaultColour = juce::Colours::white.toString();

    properties.traversalId    .setValue(traversalId,      undoManager);
    properties.tempoMultiplier.setValue(defaultTempoMult, undoManager);
    properties.colour         .setValue(defaultColour,    undoManager);
    properties.channel        .setValue(defaultChannel,   undoManager);
    properties.transpose      .setValue(defaultTranspose, undoManager);
    properties.velocity       .setValue(defaultVelocity,  undoManager);

    map.addChild(traversalData, -1, undoManager);

    return traversalData;
}

void TraversalState::collectKeys(std::vector<TraversalKey>& keys) const
{
    auto appendKey = [&keys](TraversalKey key) {
        if (key.typeId <= 0) {
            return;
        }

        if (std::ranges::find(keys, key) == keys.end()) {
            keys.push_back(key);
        }
    };

    for (int i = 0; i < graphState.nodeMap.getNumChildren(); ++i) {
        const juce::ValueTree node = graphState.nodeMap.getChild(i);

        if (node.getType() == ValueTreeIdentifiers::TraversalFlagData) {
            const NodeProperties flag(node);

            int flagValue = flag.traversalFlagValue;

            if (flagValue < 0) {
                flagValue = -flagValue;
            }

            appendKey({ flagValue, flag.traversalInstance });

            continue;
        }

        const juce::ValueTree equipped = node.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);

        for (int child = 0; child < equipped.getNumChildren(); ++child) {
            const TraversalReferenceProperties reference(equipped.getChild(child));

            appendKey({ reference.traversalId, reference.traversalInstance });
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

        const TraversalReferenceProperties referenceProperties(reference);

        const TraversalKey referencedKey { referenceProperties.traversalId, referenceProperties.traversalInstance };

        if (referencedKey == key) {
            return reference;
        }
    }

    return {};
}
