#pragma once

template<typename T>
class Ref {
public:
    [[nodiscard]] T *get() const {
        return m_Ptr;
    }

    explicit operator bool() const {
        return get() != nullptr;
    }

    operator const T&() const {
        return get();
    }

    T *operator->() const {
        return get();
    }

    const T &operator*() const {
        return *get();
    }

    T &operator*() {
        return *get();
    }

private:
    T *m_Ptr;
};
