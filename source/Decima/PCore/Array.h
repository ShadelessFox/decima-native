#pragma once

#include <cstdint>
#include <functional>
#include <iterator>

#include "Util/Offsets.h"

template<typename T>
class ArrayIterator {
public:
    using iterator_category = std::contiguous_iterator_tag;
    using iterator_concept = std::contiguous_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::remove_cv_t<T>;
    using pointer = T *;
    using reference = T &;

    explicit ArrayIterator(pointer ptr) : mPtr(ptr) {
    }

    // std::weakly_incrementable<I>
    ArrayIterator &operator++() {
        ++mPtr;
        return *this;
    }

    ArrayIterator operator++(int) {
        ArrayIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    ArrayIterator() : mPtr(nullptr/*&mArray[0]*/) {
    } // TODO: Unsure which is correct!

    // std::input_or_output_iterator<I>
    reference operator*() { return *mPtr; }

    // std::indirectly_readable<I>
    friend reference operator*(const ArrayIterator &it) { return *(it.mPtr); }

    // std::input_iterator<I>
    // No actions were needed here!

    // std::forward_iterator<I>
    // In C++20, 'operator==' implies 'operator!='
    bool operator==(const ArrayIterator &it) const { return mPtr == it.mPtr; }

    // std::bidirectional_iterator<I>
    ArrayIterator &operator--() {
        --mPtr;
        return *this;
    }

    ArrayIterator operator--(int) {
        ArrayIterator tmp = *this;
        --(*this);
        return tmp;
    }

    // std::random_access_iterator<I>
    //     std::totally_ordered<I>
    std::weak_ordering operator<=>(const ArrayIterator &it) const {
        return std::compare_three_way{}(mPtr, it.mPtr);
        // alternatively: `return mPtr <=> it.mPtr;`
    }

    //     std::sized_sentinel_for<I, I>
    difference_type operator-(const ArrayIterator &it) const { return mPtr - it.mPtr; }
    //     std::iter_difference<I> operators
    ArrayIterator &operator+=(difference_type diff) {
        mPtr += diff;
        return *this;
    }

    ArrayIterator &operator-=(difference_type diff) {
        mPtr -= diff;
        return *this;
    }

    ArrayIterator operator+(difference_type diff) const { return ArrayIterator(mPtr + diff); }
    ArrayIterator operator-(difference_type diff) const { return ArrayIterator(mPtr - diff); }

    friend ArrayIterator operator+(difference_type diff, const ArrayIterator &it) {
        return it + diff;
    }

    friend ArrayIterator operator-(difference_type diff, const ArrayIterator &it) {
        return it - diff;
    }

    reference operator[](difference_type diff) const { return mPtr[diff]; }

    // std::contiguous_iterator<I>
    pointer operator->() const { return mPtr; }
    using element_type = T;

private:
    pointer mPtr;
};

template<typename T>
class Array {
public:
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using size_type = std::size_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = ArrayIterator<T>;

    Array() = default;

    Array(const Array &) = delete;

    Array(Array &&) = default;

    ~Array() {
        for (auto &item: *this)
            item.~T();
        Offsets::CallID<"gMemFree", void(*)(void *)>(mEntries);
        mCount = 0;
        mCapacity = 0;
        mEntries = nullptr;
    }

    T &operator[](size_t index) { return mEntries[index]; }

    const T &operator[](size_t index) const { return mEntries[index]; }

    iterator begin() const { return iterator(&mEntries[0]); }

    iterator end() const { return iterator(&mEntries[mCount]); }

    [[nodiscard]] std::size_t size() const { return mCount; }

    [[nodiscard]] std::size_t capacity() const { return mCapacity; }

    [[nodiscard]] bool empty() const { return mCount == 0; }

private:
    uint32_t mCount{};
    uint32_t mCapacity{};
    T *mEntries{nullptr};
};
