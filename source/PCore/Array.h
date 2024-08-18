#include <cstdint>
#include <functional>
#include <iterator>

template<typename T, bool Const, typename PtrType = std::conditional_t<Const, const T *, T *>>
class ArrayIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using reference = T &;
    using const_reference = const T &;
    using pointer = T *;
    using const_pointer = const T *;

    ArrayIterator() = delete;

    explicit ArrayIterator(PtrType inPtr) : mPtr(inPtr) {}

    ArrayIterator &operator++() {
        mPtr++;
        return *this;
    }

    bool operator==(const ArrayIterator &inOther) const { return mPtr == inOther.mPtr; }

    bool operator!=(const ArrayIterator &inOther) const { return mPtr != inOther.mPtr; }

    const_reference operator*() const { return *mPtr; }

    const_pointer operator->() const { return mPtr; }

    template<typename = void>
    requires(!Const)
    reference operator*() { return *mPtr; }

    template<typename = void>
    requires(!Const)
    pointer operator->() { return mPtr; }

private:
    PtrType mPtr;
};

template<typename T>
class Array {
public:
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using size_type = std::size_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = ArrayIterator<T, false>;
    using const_iterator = ArrayIterator<T, true>;

    T &operator[](size_t index) { return m_Entries[index]; }

    const T &operator[](size_t index) const { return m_Entries[index]; }

    iterator begin() { return iterator(&m_Entries[0]); }

    iterator end() { return iterator(&m_Entries[m_Count]); }

    const_iterator begin() const { return const_iterator(&m_Entries[0]); }

    const_iterator end() const { return const_iterator(&m_Entries[m_Count]); }

    [[nodiscard]] std::size_t size() const { return m_Count; }

    [[nodiscard]] std::size_t capacity() const { return m_Capacity; }

    [[nodiscard]] bool empty() const { return m_Count == 0; }

private:
    uint32_t m_Count;
    uint32_t m_Capacity;
    T *m_Entries;
};