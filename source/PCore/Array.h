#include <cstdint>

template<typename T>
class Array {
public:
    const T &operator[](size_t index) const {
        return m_Entries[index];
    }

    [[nodiscard]] std::size_t size() const { return m_Count; }

private:
    uint32_t m_Count;
    uint32_t m_Capacity;
    T *m_Entries;
};