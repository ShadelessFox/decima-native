#pragma once

#include <cstdint>
#include <string_view>

#define ASSERT_SIZE(_Type, _Size) static_assert(sizeof(_Type) == (_Size), "sizeof(" # _Type ") == " # _Size)

enum class RTTIKind : std::uint8_t {
    Atom = 0,
    Pointer = 1,
    Container = 2,
    Enum = 3,
    Compound = 4,
    EnumFlags = 5,
    POD = 6
};

#pragma pack(push, 1)
struct RTTI {
    std::uint32_t mId;
    RTTIKind mKind;
    std::uint8_t mFactoryFlags;

    [[nodiscard]] std::string_view GetName() const;
};
#pragma pack(pop)

ASSERT_SIZE(RTTI, 0x6);

struct RTTIClass : RTTI {
    std::uint8_t mPad0[0x36];
    const char *mTypeName;
    std::uint8_t mPad48[0x50];
};

ASSERT_SIZE(RTTIClass, 0x98);