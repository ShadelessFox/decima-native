#pragma once

#include <cstdint>
#include <array>
#include <intsafe.h>
#include <string>

class GGUUID final {
public:
    [[nodiscard]] std::string ToString() const;

public:
    uint8_t mData0;
    uint8_t mData1;
    uint8_t mData2;
    uint8_t mData3;
    uint8_t mData4;
    uint8_t mData5;
    uint8_t mData6;
    uint8_t mData7;
    uint8_t mData8;
    uint8_t mData9;
    uint8_t mData10;
    uint8_t mData11;
    uint8_t mData12;
    uint8_t mData13;
    uint8_t mData14;
    uint8_t mData15;
};
