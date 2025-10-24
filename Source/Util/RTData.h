/*
  ==============================================================================

    RTData.h
    Created: 12 Jul 2025 1:05:00pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

class Node;
class NodeData;
class NodeLogic;

// RT POD structures for data stored in GUI //

struct RTNote {
    
    float pitch = 0;
    float velocity = 0;
    float duration = 0;
};

struct RTNode {
    
    int nodeID = 0;
    int countLimit = 0;

    bool isNode = true;
    
    std::vector<RTNote> notes;
    std::vector<int> children;
    std::vector<int> connectors;

    int graphID = 0;

};


struct RTGraph {
    
    std::unordered_map<int, RTNode> nodeMap;
    std::atomic<bool> traversalRequested;

    int rootID = 0;
    int graphID = 0;

    RTGraph() = default;
    
    RTGraph(RTGraph&& other) noexcept : nodeMap(other.nodeMap)
    {}
    
    RTGraph& operator=(RTGraph&& other) noexcept {
        if(this != &other){
            nodeMap = std::move(other.nodeMap);
        }
        return *this;
    }
    
    RTGraph(const RTGraph&) = delete;
    RTGraph& operator=(const RTGraph&) = delete;
};

//real time safe hashMap with fixed size and mutability
//template <typename Key, typename Value>
//class RTMap
//{
//    enum class BucketState : uint8_t
//    {
//        Empty = 0,
//        Occupied = 1,
//        Tombstone = 2
//    };
//
//    struct Bucket
//    {
//        std::atomic<BucketState> state{BucketState::Empty};
//        Key key;
//        Value* valuePtr = nullptr;  // Store pointer instead of value
//
//        Bucket() noexcept = default;
//        ~Bucket() noexcept = default;
//    };
//
//    Bucket* buckets = nullptr;
//    size_t capacity = 0;
//
//    size_t hashKey(const Key& key) const noexcept
//    {
//        return std::hash<Key>{}(key) % capacity;
//    }
//
//public:
//    explicit RTMap(size_t maxCapacity)
//        : capacity(maxCapacity)
//    {
//        buckets = static_cast<Bucket*>(std::malloc(sizeof(Bucket) * capacity));
//        if (!buckets)
//            throw std::bad_alloc();
//
//        for (size_t i = 0; i < capacity; ++i)
//            new (&buckets[i]) Bucket();
//    }
//
//    ~RTMap()
//    {
//        for (size_t i = 0; i < capacity; ++i)
//            buckets[i].~Bucket();
//        std::free(buckets);
//    }
//
//    // Insert or assign pointer to value. Caller owns lifetime of *valPtr.
//    bool insert_or_assign(const Key& key, Value* valPtr) noexcept
//    {
//        size_t startIdx = hashKey(key);
//
//        for (size_t i = 0; i < capacity; ++i)
//        {
//            size_t idx = (startIdx + i) % capacity;
//            auto state = buckets[idx].state.load(std::memory_order_acquire);
//
//            if (state == BucketState::Empty || state == BucketState::Tombstone)
//            {
//                buckets[idx].valuePtr = valPtr;  // just pointer assignment, no copy
//                buckets[idx].key = key;
//                buckets[idx].state.store(BucketState::Occupied, std::memory_order_release);
//                return true;
//            }
//
//            if (state == BucketState::Occupied && buckets[idx].key == key)
//            {
//                buckets[idx].valuePtr = valPtr; // update pointer to new value
//                return true;
//            }
//        }
//
//        return false;
//    }
//
//    // Find pointer to value. Returns false if not found.
//    bool find(const Key& key, Value*& outValPtr) const noexcept
//    {
//        size_t startIdx = hashKey(key);
//
//        for (size_t i = 0; i < capacity; ++i)
//        {
//            size_t idx = (startIdx + i) % capacity;
//            auto state = buckets[idx].state.load(std::memory_order_acquire);
//
//            if (state == BucketState::Empty)
//                return false;
//
//            if (state == BucketState::Occupied && buckets[idx].key == key)
//            {
//                outValPtr = buckets[idx].valuePtr; // return pointer, no copy
//                return true;
//            }
//        }
//
//        return false;
//    }
//
//    bool remove(const Key& key) noexcept
//    {
//        size_t startIdx = hashKey(key);
//
//        for (size_t i = 0; i < capacity; ++i)
//        {
//            size_t idx = (startIdx + i) % capacity;
//            auto state = buckets[idx].state.load(std::memory_order_acquire);
//
//            if (state == BucketState::Empty)
//                return false;
//
//            if (state == BucketState::Occupied && buckets[idx].key == key)
//            {
//                buckets[idx].state.store(BucketState::Tombstone, std::memory_order_release);
//                buckets[idx].valuePtr = nullptr; // clear pointer
//                return true;
//            }
//        }
//
//        return false;
//    }
//
//    void clear() noexcept
//    {
//        for (size_t i = 0; i < capacity; ++i)
//        {
//            buckets[i].state.store(BucketState::Empty, std::memory_order_release);
//            buckets[i].valuePtr = nullptr;
//        }
//    }
//};
