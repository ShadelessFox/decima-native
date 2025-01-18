#pragma once

#include <cstdint>
#include <format>
#include <string>

#include "Util/Assert.h"

struct MurmurHashValue {
    uint8_t Data0;
    uint8_t Data1;
    uint8_t Data2;
    uint8_t Data3;
    uint8_t Data4;
    uint8_t Data5;
    uint8_t Data6;
    uint8_t Data7;
    uint8_t Data8;
    uint8_t Data9;
    uint8_t Data10;
    uint8_t Data11;
    uint8_t Data12;
    uint8_t Data13;
    uint8_t Data14;
    uint8_t Data15;
};

assert_size(MurmurHashValue, 0x10);

template<>
struct std::formatter<MurmurHashValue> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    auto format(const MurmurHashValue &value, std::format_context &ctx) const {
        return std::format_to(
            ctx.out(),
            "{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
            value.Data0,
            value.Data1,
            value.Data2,
            value.Data3,
            value.Data4,
            value.Data5,
            value.Data6,
            value.Data7,
            value.Data8,
            value.Data9,
            value.Data10,
            value.Data11,
            value.Data12,
            value.Data13,
            value.Data14,
            value.Data15
        );
    }
};
