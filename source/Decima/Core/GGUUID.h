#pragma once

#include <cstdint>
#include <intsafe.h>
#include <format>

class GGUUID final {
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

template<>
struct std::formatter<GGUUID> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    auto format(const GGUUID &value, std::format_context &ctx) const {
        return std::format_to(
            ctx.out(),
            "{:02X}{:02X}{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}",
            value.mData3, value.mData2, value.mData1, value.mData0,
            value.mData5, value.mData4,
            value.mData7, value.mData6,
            value.mData8, value.mData9,
            value.mData10, value.mData11, value.mData12, value.mData13, value.mData14, value.mData15);
    }
};
