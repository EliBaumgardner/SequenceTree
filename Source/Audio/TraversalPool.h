#pragma once

#include "TraversalLogic.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
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

private:

    struct Slot
    {
        Entry entry;
        bool  active = false;
    };

public:

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

    template <bool IsConst>
    class Iterator
    {
    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type        = Entry;
        using difference_type   = std::ptrdiff_t;
        using pointer           = std::conditional_t<IsConst, const Entry*, Entry*>;
        using reference         = std::conditional_t<IsConst, const Entry&, Entry&>;

        using PoolPointer       = std::conditional_t<IsConst, const TraversalPool*, TraversalPool*>;
        using SlotReference     = std::conditional_t<IsConst, const Slot&, Slot&>;

        Iterator() = default;

        Iterator(PoolPointer owner, int startIndex) : pool(owner), index(startIndex)
        {
            skipInactive();
        }

        reference operator* () const { return  slot().entry; }
        pointer   operator->() const { return &slot().entry; }

        Iterator& operator++()
        {
            ++index;
            skipInactive();
            return *this;
        }

        bool operator==(const Iterator& other) const { return pool == other.pool && index == other.index; }
        bool operator!=(const Iterator& other) const { return pool != other.pool || index != other.index; }

        int slotIndex() const { return index; }

    private:

        SlotReference slot() const { return pool->slots[static_cast<std::size_t>(index)]; }

        void skipInactive()
        {
            const int slotCount = static_cast<int>(pool->slots.size());

            while (index < slotCount && !slot().active) {
                ++index;
            }
        }

        PoolPointer pool  = nullptr;
        int         index = 0;
    };

    using iterator       = Iterator<false>;
    using const_iterator = Iterator<true>;

    iterator begin() { return iterator(this, 0); }
    iterator end()   { return iterator(this, slotCount()); }

    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end()   const { return const_iterator(this, slotCount()); }

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

    iterator erase(iterator it)
    {
        const int index = it.slotIndex();

        slots[static_cast<std::size_t>(index)].active = false;
        --activeCount;
        ++epoch;

        return iterator(this, index + 1);
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

    bool empty() const { return activeCount == 0; }
    int  size () const { return activeCount; }

    int findRunFor(int rootId, const TraversalKey& key) const
    {
        for (const auto& [runId, instance] : *this) {
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

    std::uint64_t membershipEpoch() const { return epoch; }

    int nextRunId() { return ++runIdCounter; }

private:

    int slotCount() const { return static_cast<int>(slots.size()); }

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

    std::uint64_t epoch = 0;
};

struct DispatchContext
{
    const NodeMap&    nodes;
    TraversalPool&    traversalMap;
    juce::MidiBuffer& midiMessages;

    double sampleRate      = 44100.0;
    double tempoMultiplier = 1.0;
};
