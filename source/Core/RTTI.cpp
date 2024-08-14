#include "RTTI.h"

String RTTI::GetName() const {
    String out;
    Offsets::CallID<"RTTI::GetName", String*(*)(const RTTI*, String*)>(this, &out);
    return out;
}

String RTTI::ToString(const void* value) const {
    String out;
    Offsets::CallID<"RTTI::ToString", String*(*)(const RTTI*, const void*, String*)>(this, value, &out);
    return out;
}
