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

struct String {
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
static_assert(sizeof(struct String) == 0x10, "sizeof(struct String) == 0x10");
static_assert(sizeof(struct StringSlice) == 0x10, "sizeof(struct StringSlice) == 0x10");

#define STRING_BASE(_String) ((struct String *) ((char *) (_String) - offsetof(struct String, mData)))
#define STRING_PTR(_String) const char *

#else

#define STRING_BASE(_String) _String
#define STRING_PTR(_String) _String *__shifted(_String, sizeof(_String))

#endif