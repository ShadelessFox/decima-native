#ifndef DECIMA_NATIVE_JSON_H
#define DECIMA_NATIVE_JSON_H

#include <stdio.h>
#include <stdint.h>

#define JsonValueStr(_Ctx, _Value) JsonValue(_Ctx, (struct JsonValue) {.type = JsonType_String, .string = (_Value)})
#define JsonValueNum(_Ctx, _Value) JsonValue(_Ctx, (struct JsonValue) {.type = JsonType_Integer, .integer = (_Value)})
#define JsonValueBool(_Ctx, _Value) JsonValue(_Ctx, (struct JsonValue) {.type = JsonType_Bool, .integer = (_Value)})

#define JsonBeginCompactObject(_Ctx) do { JsonBeginObject(_Ctx); JsonCompact(_Ctx, 1); } while (0)
#define JsonEndCompactObject(_Ctx) do { JsonEndObject(_Ctx); JsonCompact(_Ctx, 0); } while (0)
#define JsonBeginCompactArray(_Ctx) do { JsonBeginArray(_Ctx); JsonCompact(_Ctx, 1); } while (0)
#define JsonEndCompactArray(_Ctx) do { JsonEndArray(_Ctx); JsonCompact(_Ctx, 0); } while (0)

#define JsonNameValueStr(_Ctx, _Name, _Value) \
    do {                                      \
        JsonName(_Ctx, _Name);                \
        JsonValueStr(_Ctx, _Value);           \
    } while (0)

#define JsonNameValueNum(_Ctx, _Name, _Value) \
    do {                                      \
        JsonName(_Ctx, _Name);                \
        JsonValueNum(_Ctx, _Value);           \
    } while (0)

#define JsonNameValueBool(_Ctx, _Name, _Value) \
    do {                                       \
        JsonName(_Ctx, _Name);                 \
        JsonValueBool(_Ctx, _Value);           \
    } while (0)

#define JsonNameObject(_Ctx, _Name) \
    do {                            \
        JsonName(_Ctx, _Name);      \
        JsonBeginObject(_Ctx);      \
    } while (0)

#define JsonNameArray(_Ctx, _Name) \
    do {                           \
        JsonName(_Ctx, _Name);     \
        JsonBeginArray(_Ctx);      \
    } while (0)

#define JsonNameCompactObject(_Ctx, _Name) \
    do {                                   \
        JsonName(_Ctx, _Name);             \
        JsonBeginCompactObject(_Ctx);      \
    } while (0)

#define JsonNameCompactArray(_Ctx, _Name) \
    do {                                  \
        JsonName(_Ctx, _Name);            \
        JsonBeginCompactArray(_Ctx);      \
    } while (0)

struct JsonContext {
    FILE *stream;
    int compact;
    const char *name;
    size_t index;
    int scopes[32];
};

enum JsonType {
    JsonType_String,
    JsonType_Integer,
    JsonType_Bool
};

struct JsonValue {
    enum JsonType type;
    union {
        const char *string;
        int integer;
    };
};

void JsonInit(struct JsonContext *ctx, FILE *);

void JsonBeginObject(struct JsonContext *ctx);

void JsonEndObject(struct JsonContext *ctx);

void JsonBeginArray(struct JsonContext *ctx);

void JsonEndArray(struct JsonContext *ctx);

void JsonName(struct JsonContext *ctx, const char *name);

void JsonValue(struct JsonContext *ctx, struct JsonValue value);

void JsonCompact(struct JsonContext *ctx, int compact);

#endif //DECIMA_NATIVE_JSON_H
