#include "json.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

enum JsonScope {
    JsonScope_DanglingName,
    JsonScope_EmptyArray,
    JsonScope_EmptyDocument,
    JsonScope_EmptyObject,
    JsonScope_NonEmptyArray,
    JsonScope_NonEmptyDocument,
    JsonScope_NonEmptyObject,
};

static void NewLine(struct JsonContext *ctx) {
    if (ctx->compact)
        return;

    putc('\n', ctx->stream);

    for (size_t i = 1; i < ctx->index; i++) {
        putc('\t', ctx->stream);
    }
}

static void WriteString(struct JsonContext *ctx, const char *string) {
    putc('"', ctx->stream);
    for (size_t i = 0, len = strlen(string); i < len; i++) {
        char ch = string[i];
        if (ch == '"' || ch == '\\')
            putc('\\', ctx->stream);
        putc(ch, ctx->stream);
    }
    putc('"', ctx->stream);
}

static void ReplaceTop(struct JsonContext *ctx, enum JsonScope scope) {
    ctx->scopes[ctx->index - 1] = scope;
}

static void Push(struct JsonContext *ctx, enum JsonScope scope) {
    ctx->scopes[ctx->index++] = scope;
}

static void BeforeName(struct JsonContext *ctx) {
    enum JsonScope scope = ctx->scopes[ctx->index - 1];

    if (scope == JsonScope_NonEmptyObject) {
        fputs(ctx->compact ? ", " : ",", ctx->stream);
    } else if (scope != JsonScope_EmptyObject) {
        assert(0 && "Nesting problem");
    }

    NewLine(ctx);
    ReplaceTop(ctx, JsonScope_DanglingName);
}

static void BeforeValue(struct JsonContext *ctx) {
    switch (ctx->scopes[ctx->index - 1]) {
        case JsonScope_EmptyDocument:
            ReplaceTop(ctx, JsonScope_NonEmptyDocument);
            break;
        case JsonScope_NonEmptyDocument:
            assert(0 && "JSON must have only one top-level value");
            break;
        case JsonScope_EmptyArray:
            ReplaceTop(ctx, JsonScope_NonEmptyArray);
            NewLine(ctx);
            break;
        case JsonScope_NonEmptyArray:
            fputs(ctx->compact ? ", " : ",", ctx->stream);
            NewLine(ctx);
            break;
        case JsonScope_DanglingName:
            ReplaceTop(ctx, JsonScope_NonEmptyObject);
            fputs(": ", ctx->stream);
            break;
        default:
            assert(0 && "Nesting problem");
            break;
    }
}

static void Open(struct JsonContext *ctx, enum JsonScope empty, int bracket) {
    BeforeValue(ctx);
    Push(ctx, empty);
    putc(bracket, ctx->stream);
}

static void Close(struct JsonContext *ctx, enum JsonScope empty, enum JsonScope nonempty, int bracket) {
    enum JsonScope scope = ctx->scopes[ctx->index - 1];

    assert(scope == empty || scope == nonempty);
    assert(ctx->name == NULL);

    ctx->index--;

    if (scope == nonempty)
        NewLine(ctx);

    putc(bracket, ctx->stream);
}

static void WriteDeferredName(struct JsonContext *ctx) {
    if (ctx->name != NULL) {
        BeforeName(ctx);
        WriteString(ctx, ctx->name);
        ctx->name = NULL;
    }
}

void JsonInit(struct JsonContext *ctx, FILE *stream) {
    ctx->stream = stream;
    ctx->compact = 0;
    ctx->index = 0;
    ctx->name = NULL;

    Push(ctx, JsonScope_EmptyDocument);
}

void JsonBeginObject(struct JsonContext *ctx) {
    WriteDeferredName(ctx);
    Open(ctx, JsonScope_EmptyObject, '{');
}

void JsonEndObject(struct JsonContext *ctx) {
    Close(ctx, JsonScope_EmptyObject, JsonScope_NonEmptyObject, '}');
}

void JsonBeginArray(struct JsonContext *ctx) {
    WriteDeferredName(ctx);
    Open(ctx, JsonScope_EmptyArray, '[');
}

void JsonEndArray(struct JsonContext *ctx) {
    Close(ctx, JsonScope_EmptyArray, JsonScope_NonEmptyArray, ']');
}

void JsonCompact(struct JsonContext *ctx, int compact) {
    ctx->compact = compact;
}

void JsonName(struct JsonContext *ctx, const char *name) {
    assert(ctx->name == NULL);
    assert(ctx->index > 0);

    ctx->name = name;
}

void JsonValue(struct JsonContext *ctx, struct JsonValue value) {
    WriteDeferredName(ctx);
    BeforeValue(ctx);

    switch (value.type) {
        case JsonType_String:
            WriteString(ctx, value.string);
            break;
        case JsonType_Integer:
            fprintf(ctx->stream, "%d", value.integer);
            break;
        case JsonType_Bool:
            fputs(value.integer ? "true" : "false", ctx->stream);
            break;
    }
}
