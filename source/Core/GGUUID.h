#pragma once

#include <cstdint>
#include <array>
#include <intsafe.h>
#include <string>

struct GGUUID {
    std::array<uint8_t, 16> mData {};

    [[nodiscard]] std::string ToString() const;
};
