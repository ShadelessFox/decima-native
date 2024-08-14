#pragma once

#include <array>
#include <stdexcept>

template<typename T, size_t Digits = sizeof(T) * 2>
constexpr static uint32_t ParseHex(const char *Hex) {
    auto charToByte = [](char ch) constexpr -> uint32_t {
        if (ch >= 'A' && ch <= 'F')
            return ch - 'A' + 10;
        else if (ch >= 'a' && ch <= 'f')
            return ch - 'a' + 10;
        else if (ch >= '0' && ch <= '9')
            return ch - '0';

        throw std::invalid_argument("Invalid hexadecimal digit");
    };

    T value{};

    for (size_t i = 0; i < Digits; i++)
        value |= charToByte(Hex[i]) << (4 * (Digits - i - 1));

    return value;
}

class GGUUID {
public:
    GGUUID() : mAll{0} {
    }

    template <size_t Length_>
    constexpr GGUUID(const char (&UUID)[Length_]) : GGUUID(Parse(UUID)) {
    }

    bool operator==(const GGUUID &Other) const {
        return mAll == Other.mAll;
    }

    bool operator!=(const GGUUID &Other) const {
        return mAll != Other.mAll;
    }

    template <size_t Length_>
    constexpr static GGUUID Parse(const char (&UUID)[Length_]) {
        constexpr auto Length = Length_ - 1;
        //
        // Parse as:
        // 40e36691-5fd0-4a79-b3b3-87b2a3d13e9c
        // 40E36691-5FD0-4A79-B3B3-87B2A3D13E9C
        // {40E36691-5FD0-4A79-B3B3-87B2A3D13E9C}
        //
        const size_t add = (Length == 38) ? 1 : 0;

        if (Length != 36 && Length != 38)
            throw std::invalid_argument("Invalid GUID length specified");

        if (add && (UUID[0] != '{' || UUID[Length - 1] != '}'))
            throw std::invalid_argument("Invalid bracket pair used");

        GGUUID id{};
        id.mData1 = ParseHex<uint32_t>(UUID + 0 + add);
        id.mData2 = ParseHex<uint16_t>(UUID + 9 + add);
        id.mData3 = ParseHex<uint16_t>(UUID + 14 + add);
        id.mData4[0] = ParseHex<uint8_t>(UUID + 19 + add);
        id.mData4[1] = ParseHex<uint8_t>(UUID + 21 + add);

        for (int i = 0; i < 6; i++)
            id.mData4[i + 2] = ParseHex<uint8_t>(UUID + 24 + (i * 2) + add);

        return id;
    }

private:
    union {
        struct {
            uint32_t mData1;
            uint16_t mData2;
            uint16_t mData3;
            uint8_t mData4[8];
        };
        std::array<std::uint8_t, 16> mAll;
    };
};