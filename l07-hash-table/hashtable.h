#ifndef HASHTABLE_H
#define HASHTABLE_H

// Project Identifier: 2C4A3C53CD5AD45A7BEA3AE5356A3B1622C9D04B

// INSTRUCTIONS:
// fill out the methods in the class below.

// You may assume that the key and value types can be copied and have default
// constructors.

// You can also assume that the key type has (==) and (!=) methods.

// You may assume that Hasher is a functor type with a
// size_t operator()(const Key&) overload.

// The key/value types aren't guaranteed to support any other operations.

// Do not add, remove, or change any of the class's member variables.
// The num_deleted counter is *optional*. You might find it helpful, but it
// is not required to pass the lab assignment.

// Do not change the Bucket type.

// SUBMISSION INSTRUCTIONS:
// Submit this file, by itself, in a .tar.gz.
// Other files will be ignored.

#include <cstdint>
#include <functional> // where std::hash lives
#include <vector>
#include <cassert> // useful for debugging!

// A bucket's status tells you whether it's empty, occupied, 
// or contains a deleted element.
enum class Status : uint8_t {
    Empty,
    Occupied,
    Deleted
};

template<typename K, typename V, typename Hasher = std::hash<K>>
class HashTable {
    // used by the autograder; do not change/remove.
    friend class Verifier;
public:
    // A bucket has a status, a key, and a value.
    struct Bucket {
        // Do not modify Bucket.
        Status status = Status::Empty;
        K key;
        V val;
    };

    HashTable() {
        buckets.resize(20);
    }

    size_t size() const {
        return num_elements;
    }

    // returns a reference to the value in the bucket with the key, if it
    // already exists. Otherwise, insert it with a default value, and return
    // a reference to the resulting bucket.
    V& operator[](const K& key) {
        size_t idx = find_key(key);
        if (idx == buckets.size())
            idx = insert_new(key, V{});
        return buckets[idx].val;
    }

    // insert returns whether inserted successfully
    // (if the key already exists in the table, do nothing and return false).
    bool insert(const K& key, const V& val) {
        if (find_key(key) != buckets.size())
            return false;
        insert_new(key, val);
        return true;
    }
    // erase returns the number of items remove (0 or 1)
    size_t erase(const K& key) {
        size_t idx = find_key(key);
        if (idx == buckets.size())
            return 0;
        buckets[idx].status = Status::Deleted;
        --num_elements;
        ++num_deleted;
        return 1;
    }

private:
    size_t num_elements = 0;
    size_t num_deleted = 0; // OPTIONAL: you don't need to use num_deleted to pass
    std::vector<Bucket> buckets;

    // Rebuilds the table without deleted markers. Doubles the bucket count
    // when live elements fill at least a quarter of the table; otherwise
    // keeps the same size (the rehash was caused by deleted markers).
    void rehash_and_grow() {
        size_t new_size = buckets.empty() ? 20 : buckets.size();
        if (num_elements * 4 >= new_size)
            new_size *= 2;

        std::vector<Bucket> old(new_size);
        old.swap(buckets);
        num_elements = 0;
        num_deleted = 0;
        for (const Bucket &b : old) {
            if (b.status == Status::Occupied) {
                buckets[probe_free(b.key)] = b;
                ++num_elements;
            }
        }
    }

    // Returns the index of the occupied bucket holding key,
    // or buckets.size() if the key is not in the table.
    size_t find_key(const K& key) const {
        size_t n = buckets.size();
        if (n == 0)
            return 0;
        Hasher hasher;
        size_t idx = hasher(key) % n;
        for (size_t i = 0; i < n; ++i) {
            const Bucket &b = buckets[idx];
            if (b.status == Status::Empty)
                return n;
            if (b.status == Status::Occupied && b.key == key)
                return idx;
            idx = (idx + 1) % n;
        }
        return n;
    }

    // Returns the first empty or deleted bucket on key's probe sequence.
    // Only call when key is not already in the table.
    size_t probe_free(const K& key) const {
        Hasher hasher;
        size_t idx = hasher(key) % buckets.size();
        while (buckets[idx].status == Status::Occupied)
            idx = (idx + 1) % buckets.size();
        return idx;
    }

    // Inserts a key that is known to be absent, growing first if needed.
    // Returns the index of the new bucket.
    size_t insert_new(const K& key, const V& val) {
        // keep (live + deleted) load factor at or below 0.5
        if ((num_elements + num_deleted + 1) * 2 > buckets.size())
            rehash_and_grow();
        size_t idx = probe_free(key);
        if (buckets[idx].status == Status::Deleted)
            --num_deleted;
        buckets[idx].status = Status::Occupied;
        buckets[idx].key = key;
        buckets[idx].val = val;
        ++num_elements;
        return idx;
    }
};

#endif // HASHTABLE_H