struct GGUUID {
    uint64_t mLow;
    uint64_t mHigh;
};

struct RTTIObject;
struct RTTI;

struct RTTIObject_vtbl {
    struct RTTI *(*GetRTTI)();
    void *(*Destroy)(struct RTTIObject *, char);
};

struct RTTIObject {
    struct RTTIObject_vtbl *vtbl;
};

struct RTTIRefObject {
    struct RTTIObject base;
    struct GGUUID mObjectUUID;
    uint32_t mRefCount;
};

struct StringData {
    uint32_t mRefCount;
    uint32_t mCRC;
    uint32_t mLength;
    uint32_t mCapacity;
    const char mData[];
};

struct StringSlice {
    const char *mData;
    uint32_t mLength;
    uint32_t mCRC;
};

#ifdef RTTI_STANDALONE

static_assert(sizeof(struct GGUUID) == 0x10, "sizeof(struct GGUUID) == 0x10");
static_assert(sizeof(struct RTTIObject) == 0x8, "sizeof(struct RTTIObject) == 0x8");
static_assert(sizeof(struct RTTIRefObject) == 0x20, "sizeof(struct RTTIRefObject) == 0x20");
static_assert(sizeof(struct StringData) == 0x10, "sizeof(struct StringData) == 0x10");
static_assert(sizeof(struct StringSlice) == 0x10, "sizeof(struct StringSlice) == 0x10");

#define STRING_BASE(_Value) ((struct String *) ((char *) (_Value) - offsetof(struct String, mData)))
#define STRING_DATA const char *

#else

#define STRING_BASE(_Value) (_Value)
#define STRING_DATA const char *__shifted(struct StringData, sizeof(struct StringData))

#endif