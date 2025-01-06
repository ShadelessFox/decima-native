#pragma once

#include <functional>

template<typename Key, typename Hash>
class HashContainerBase {
public:
    using key_type = Key;
    using value_type = Key;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using hasher = Hash;

    using reference = value_type &;
    using const_reference = const value_type &;

    using pointer = void;
    using const_pointer = void;

    HashContainerBase() = delete;
    HashContainerBase(const HashContainerBase &) = delete;

    [[nodiscard]] std::size_t size() const { return mCount; }

    [[nodiscard]] std::size_t capacity() const { return mCapacity; }

    [[nodiscard]] bool empty() const { return mCount == 0; }

public:
    struct Bucket {
        value_type mValue;
        hasher mHash;
    };

    Bucket *mEntries{nullptr};
    uint32_t mCount{0};
    uint32_t mCapacity{0};
};

template<typename Key, typename Value, typename Hash = uint32_t>
class HashMap : public HashContainerBase<std::pair<Value, Key>, Hash> {
public:
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<Value, Key>;

    using reference = value_type &;
    using const_reference = const value_type &;
};