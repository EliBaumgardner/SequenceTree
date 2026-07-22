#pragma once

#include "TraversalLogic.h"

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

class TraversalPool
{
public:

    using Entry = std::pair<int, TraversalLogic>;

private:

    struct Slot
    {
        Entry entry;
        bool  active = false;
    };

public:

    void prepare(int capacity, AudioUIBridge& bridge)
    {
        if (slotCount() == capacity) {
            return;
        }

        slots.assign(static_cast<std::size_t>(capacity), Slot{});

        for (auto& slot : slots) {
            slot.entry.second.bridge = &bridge;
        }

        activeCount = 0;
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

        using PoolPointer   = std::conditional_t<IsConst, const TraversalPool*, TraversalPool*>;
        using SlotReference = std::conditional_t<IsConst, const Slot&, Slot&>;

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

        bool operator==(const Iterator& other) const { return index == other.index; }
        bool operator!=(const Iterator& other) const { return index != other.index; }

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

    iterator       find(int id)       { return iterator      (this, findSlotIndex(id)); }
    const_iterator find(int id) const { return const_iterator(this, findSlotIndex(id)); }

    TraversalLogic& at(int id) { return slots[static_cast<std::size_t>(findSlotIndex(id))].entry.second; }

    TraversalLogic* acquire(int id, int rootId, const RTtraversal& traversal)
    {
        for (auto& slot : slots) {
            if (slot.active) {
                continue;
            }

            slot.active      = true;
            slot.entry.first = id;
            slot.entry.second.reset(rootId, traversal);

            ++activeCount;

            return &slot.entry.second;
        }

        return nullptr;
    }

    iterator erase(iterator it)
    {
        const int index = it.slotIndex();

        slots[static_cast<std::size_t>(index)].active = false;
        --activeCount;

        return iterator(this, index + 1);
    }

    void clear()
    {
        for (auto& slot : slots) {
            slot.active = false;
        }

        activeCount = 0;
    }

    bool empty() const { return activeCount == 0; }
    int  size () const { return activeCount; }

private:

    int slotCount() const { return static_cast<int>(slots.size()); }

    int findSlotIndex(int id) const
    {
        for (std::size_t i = 0; i < slots.size(); ++i) {
            if (slots[i].active && slots[i].entry.first == id) {
                return static_cast<int>(i);
            }
        }

        return slotCount();
    }

    std::vector<Slot> slots;

    int activeCount = 0;
};

using TraversalMap = TraversalPool;
