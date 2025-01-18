#pragma once

#include <functional>

#include "Util/Assert.h"

template<typename T>
concept HashBucket = requires(T t) { t.mHash; t.mValue; };

template<HashBucket T, bool Const, typename PtrType = std::conditional_t<Const, const T *, T *>>
class HashContainerIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = decltype(T::mValue);
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;

    HashContainerIterator() = delete;

    explicit HashContainerIterator(PtrType inPtr, PtrType inEnd) : mPtr(inPtr), mEnd(inEnd) {
        Advance();
    }

    HashContainerIterator &operator++() {
        mPtr++;
        Advance();
        return *this;
    }

    bool operator==(const HashContainerIterator &inOther) const { return mPtr == inOther.mPtr; }

    bool operator!=(const HashContainerIterator &inOther) const { return mPtr != inOther.mPtr; }

    const_reference operator*() const { return mPtr->mValue; }

    const_pointer operator->() const { return &mPtr->mValue; }

    template<typename = void>
    requires(!Const)
    reference operator*() { return mPtr->mValue; }

    template<typename = void>
    requires(!Const)
    pointer operator->() { return &mPtr->mValue; }

private:
    PtrType mPtr;
    PtrType mEnd;

    void Advance() {
        while (mPtr != mEnd && mPtr->mHash == 0)
            mPtr++;
    }
};


template<typename Key, typename Value, typename Hash = uint32_t>
class HashMap {
private:
    struct Entry;
public:
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<const Key, Value>;
    using size_type = std::uint32_t;
    using difference_type = std::ptrdiff_t;

    using hasher = Hash;
    using reference = value_type &;
    using const_reference = const value_type &;

    using iterator = HashContainerIterator<Entry, false>;
    using const_iterator = HashContainerIterator<Entry, true>;

    [[nodiscard]] iterator begin() { return iterator(mEntries, &mEntries[mCapacity]); }

    [[nodiscard]] iterator end() { return iterator(&mEntries[mCapacity], &mEntries[mCapacity]); }

    [[nodiscard]] const_iterator begin() const { return const_iterator(mEntries, &mEntries[mCapacity]); }

    [[nodiscard]] const_iterator end() const { return const_iterator(&mEntries[mCapacity], &mEntries[mCapacity]); }

    [[nodiscard]] size_type size() const { return mSize; }

    [[nodiscard]] size_type capacity() const { return mSize; }

private:
    struct Entry {
        value_type mValue;
        hasher mHash;
    };

    Entry *mEntries;
    size_type mSize;
    size_type mCapacity;
};

template<typename Key, typename Hash = uint32_t>
class HashSet {
private:
    struct Entry;
public:
    using key_type = Key;
    using value_type = Key;
    using size_type = std::uint32_t;
    using difference_type = std::ptrdiff_t;

    using hasher = Hash;
    using reference = value_type &;
    using const_reference = const value_type &;

    using iterator = HashContainerIterator<Entry, false>;
    using const_iterator = HashContainerIterator<Entry, true>;

    iterator begin() { return iterator(mEntries, &mEntries[mCapacity]); }

    iterator end() { return iterator(&mEntries[mCapacity], &mEntries[mCapacity]); }

    const_iterator begin() const { return const_iterator(mEntries, &mEntries[mCapacity]); }

    const_iterator end() const { return const_iterator(&mEntries[mCapacity], &mEntries[mCapacity]); }

private:
    struct Entry {
        hasher mHash;
        value_type mValue;
    };

    Entry *mEntries;
    size_type mSize;
    size_type mCapacity;
};