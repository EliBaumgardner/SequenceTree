#pragma once

#include "TraversalLogic.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <cstddef>
#include <cstdint>
#include <ranges>
#include <utility>
#include <vector>

struct TraversalRuntime
{
    bool asFlag         = false;
    bool asCrossTree    = false;
    int  sourceNodeId   = -1;
    bool pendingRemoval = false;

    int  originRootId   = -1;

    int  repeatCount = 0;

    bool isSpawned() const { return asFlag || asCrossTree; }
};

class TraversalPool
{
public:
    struct Instance
    {
        TraversalLogic   logic;
        TraversalRuntime runtime;
    };

    using Entry = std::pair<int, Instance>;

    struct Slot
    {
        Entry entry;
        bool  active = false;
    };

    void prepare(int capacity, const TraversalRule& rule)
    {
        if (slotCount() != capacity) {
            slots.assign(static_cast<std::size_t>(capacity), Slot{});

            activeCount = 0;
            ++epoch;
        }

        for (auto& slot : slots) {
            slot.entry.second.logic.nodeState.prepare();
            slot.entry.second.logic.rule = &rule;
        }
    }

    auto entries()
    {
        return slots | std::views::filter(&Slot::active) | std::views::transform(&Slot::entry);
    }

    auto entries() const
    {
        return slots | std::views::filter(&Slot::active) | std::views::transform(&Slot::entry);
    }

    Instance* find(int runId)
    {
        const int index = findSlotIndex(runId);

        if (index == -1) {
            return nullptr;
        }

        return &slots[static_cast<std::size_t>(index)].entry.second;
    }

    const Instance* find(int runId) const
    {
        const int index = findSlotIndex(runId);

        if (index == -1) {
            return nullptr;
        }

        return &slots[static_cast<std::size_t>(index)].entry.second;
    }

    Instance* acquire(int runId, int rootId, const RTtraversal& traversal)
    {
        for (auto& slot : slots) {
            if (slot.active) {
                continue;
            }

            slot.active      = true;
            slot.entry.first = runId;
            slot.entry.second.logic.reset(rootId, traversal);
            slot.entry.second.runtime = {};

            ++activeCount;
            ++epoch;

            return &slot.entry.second;
        }

        return nullptr;
    }

    void erase(int runId)
    {
        const int index = findSlotIndex(runId);

        if (index == -1) {
            return;
        }

        slots[static_cast<std::size_t>(index)].active = false;
        --activeCount;
        ++epoch;
    }

    void clear()
    {
        for (auto& slot : slots) {
            slot.active = false;
        }

        activeCount = 0;
        ++epoch;
    }

    constexpr bool empty() const
    {
        return activeCount == 0;
    }

    int findRunFor(int rootId, const TraversalKey& key) const
    {
        for (const auto& [runId, instance] : entries()) {
            if (instance.logic.rootId == rootId && instance.logic.traversal.key == key) {
                return runId;
            }
        }

        return -1;
    }

    bool hasAllRegisteredRunsOnTree(const RTNode& rootNode) const
    {
        for (const RTtraversal& registered : rootNode.traversals) {
            const int runId = findRunFor(rootNode.nodeID, registered.key);

            if (runId == -1) {
                return false;
            }

            const Instance* const instance = find(runId);

            if (instance == nullptr || !instance->logic.shouldTraverse()) {
                return false;
            }
        }

        return true;
    }

    int nextRunId() { return ++runIdCounter; }

    std::uint64_t epoch = 0;

private:
    constexpr int slotCount() const
    {
        return static_cast<int>(slots.size());
    }

    int findSlotIndex(int runId) const
    {
        for (std::size_t i = 0; i < slots.size(); ++i) {
            if (slots[i].active && slots[i].entry.first == runId) {
                return static_cast<int>(i);
            }
        }

        return -1;
    }

    std::vector<Slot> slots;

    int activeCount = 0;

    int runIdCounter = 0;
};

struct DispatchContext
{
    const NodeMap&    nodes;
    TraversalPool&    traversalMap;
    juce::MidiBuffer& midiMessages;

    double sampleRate      = 44100.0;
    double tempoMultiplier = 1.0;
    int    transpose       = 0;
    double velocityScale   = 1.0;
};
