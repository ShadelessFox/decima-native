#include "MurmurHashValue.h"

#include "PCore/String.h"

std::string MurmurHashValue::ToString() {
    String string;
    Offsets::CallID<"MurmurHashValue::ToString", bool(*)(MurmurHashValue *, String *)>(this, &string);
    return string.str();
}
