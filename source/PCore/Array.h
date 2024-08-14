#include <cstdint>
#include <functional>

template<typename T>
class Array {
public:
    const T &operator[](size_t index) const {
        return m_Entries[index];
    }

    [[nodiscard]] std::size_t size() const { return m_Count; }

    [[nodiscard]] std::size_t capacity() const { return m_Capacity; }

    [[nodiscard]] bool empty() const { return m_Count == 0; }

private:
    uint32_t m_Count;
    uint32_t m_Capacity;
    T *m_Entries;
};