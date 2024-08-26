#pragma once

#include <cstdint>
#include <array>
#include <string>

struct MurmurHashValue {
    std::array<uint8_t, 16> mData {};

    std::string ToString();
};
