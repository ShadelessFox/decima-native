#pragma once

#include "Offsets.h"

class String {
public:
    String() {
        Offsets::CallID<"String::FromCString", String *(*)(String *, const char*)>(this, "");
    }

    String(const char *rhs) : mData{nullptr} {
        Offsets::CallID<"String::FromCString", String *(*)(String *, const char*)>(this, rhs);
    }

    String(const String &rhs) : String() {
        Offsets::CallID<"String::FromString", String *(*)(String*, const String&)>(this, rhs);
    }

    ~String() {
        Offsets::CallID<"String::~String", void *(*)(String *)>(this);
    }

    String &operator=(const String &rhs) {
        if (this == &rhs)
            return *this;
        Offsets::CallID<"String::FromString", String *(*)(String*, const String&)>(this, rhs);
        return *this;
    };

    bool operator==(const char* rhs) const {
        return strcmp(mData, rhs) == 0;
    }

    [[nodiscard]] const char *c_str() const { return mData; }

    [[nodiscard]] std::string_view view() const { return c_str(); }

    [[nodiscard]] size_t size() const { return data().mLength; }

private:
    struct Data {
        uint32_t mRefCount;
        uint32_t mCrc;
        uint32_t mLength;
        uint32_t mCapacity;
        char mData[];
    };

    [[nodiscard]] const Data &data() const {
        return *reinterpret_cast<const Data *>(reinterpret_cast<ptrdiff_t>(mData - sizeof(Data)));
    }

    const char *mData;
};