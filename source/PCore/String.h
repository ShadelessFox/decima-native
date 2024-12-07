#pragma once

#include "Util/Offsets.h"
#include "MemoryPool.h"

class String {
public:
    String() {
        static auto sEmptyBuffer = Offsets::ResolveID<"String::sEmptyBuffer", Buffer *>();

        set(sEmptyBuffer);
    }

    explicit String(const char *inCStr) {
        auto len = strlen(inCStr);
        auto buf = static_cast<Buffer *>(MemoryPool::Instance().Alloc(sizeof(Buffer) + len + 1));
        buf->mRefCount = 1;
        buf->mCrc = -1;
        buf->mLength = len;
        buf->mCapacity = len;
        strcpy(buf->mData, inCStr);
        set(buf);
    }

    ~String() {
        static auto sEmptyBuffer = Offsets::ResolveID<"String::sEmptyBuffer", Buffer *>();

        if (auto &buf = buffer(); &buf != sEmptyBuffer) {
            if (auto refs = _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&buf.mRefCount), -1) & 0x7FFFFFFF; refs == 1) {
                Offsets::CallID<"String::Buffer::~Buffer", void(*)(Buffer &)>(buf);
            }
        }
    }

    /*
    String(const String &rhs) : String() {
        Offsets::CallID<"String::FromString", String *(*)(String *, const String &)>(this, rhs);
    }

    String &operator=(const String &rhs) {
        if (this == &rhs)
            return *this;
        Offsets::CallID<"String::FromString", String *(*)(String *, const String &)>(this, rhs);
        return *this;
    };
     */

    bool operator==(const char *rhs) const {
        return strcmp(mData, rhs) == 0;
    }

    bool operator!=(const char *rhs) const {
        return strcmp(mData, rhs) != 0;
    }

    [[nodiscard]] const char *c_str() const { return mData; }

    [[nodiscard]] std::string str() const { return c_str(); }

    [[nodiscard]] std::string_view view() const { return c_str(); }

    [[nodiscard]] size_t size() const { return buffer().mLength; }

    [[nodiscard]] size_t capacity() const { return buffer().mCapacity; }

private:
    struct Buffer {
        uint32_t mRefCount;
        uint32_t mCrc;
        uint32_t mLength;
        uint32_t mCapacity;
        char mData[];
    };

    [[nodiscard]] Buffer &buffer() const {
        return *reinterpret_cast<Buffer *>(reinterpret_cast<uintptr_t>(mData - sizeof(Buffer)));
    }

    void set(Buffer *inBuffer) {
        mData = reinterpret_cast<const char *>(reinterpret_cast<uintptr_t>(inBuffer) + sizeof(Buffer));
    }

    const char *mData{nullptr};
};
