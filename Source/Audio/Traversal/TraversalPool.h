#pragma once

#include "TraversalLogic.h"

#include <cstddef>
#include <cstdint>
#include <ranges>
#include <utility>
#include <vector>

/*
    The container for a group of running traversals: a fixed number of slots, each holding one
    traversal under its run id, reserved ahead of time so a traversal can start during playback
    without allocating.

    prepare reserves the slots. acquire puts a new traversal in a free slot under a run id from
    nextRunId and sets it up with the pool's choices, mode, script and shared variables, so the
    same type serves playback and note preview. erase removes one, and clear removes them all.
    find and findRunFor look a traversal up by run id, or by its root and traversal key. Next to
    each traversal, a TraversalRuntime records whether a traversal flag or a jump from another tree
    started it, where it came from, and whether it is waiting to be removed. epoch goes up whenever
    a traversal starts or stops, so TraversalSession::syncWithGraph can skip its work when nothing
    changed.

    TraversalSession owns two: traversals for playback and previewTraversals for note preview. Each
    pool has its own shared script variables (ScriptGlobals), reset by clear. Before a jump starts
    traversals on another tree, TraversalDispatcher calls hasAllRegisteredRunsOnTree and skips the
    trees whose traversals are all running already.
*/

struct TraversalRuntime
{
    bool asFlag      = false;
    bool asCrossTree = false;

    int sourceNodeId = -1;
    int originRootId = -1;

    bool pendingRemoval = false;

    int repeatCount = 0;

    ScriptNote scriptNote;

    bool isSpawned() const { return asFlag || asCrossTree; }
};

class TraversalPool
{
public:
    struct Instance;

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

            if (choices == Choices::Fixed) {
                slot.entry.second.logic.selectionRandom = 0;
            }

            slot.entry.second.logic.mode              = mode;
            slot.entry.second.logic.previewVisitCount = previewVisitCount;
            slot.entry.second.logic.script            = script;
            slot.entry.second.logic.sharedGlobals     = &globals;

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

        activeCount    = 0;
        globals.script = nullptr;
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

    struct Instance
    {
        TraversalLogic   logic;
        TraversalRuntime runtime;
    };

    using Entry = std::pair<int, Instance>;

    enum class Choices { Random, Fixed };

    struct Slot
    {
        Entry entry;
        bool  active = false;
    };

    std::uint64_t epoch = 0;

    Choices choices = Choices::Random;

    TraversalLogic::Mode mode              = TraversalLogic::Mode::Live;
    int                  previewVisitCount = 1;

    const RTScript* script = nullptr;

    ScriptGlobals globals;

    int runIdCounter = 0;

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
};
