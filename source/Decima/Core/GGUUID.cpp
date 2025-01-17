#include "GGUUID.h"

#include "Decima/PCore/String.h"

std::string GGUUID::ToString() const {
    String string;
    Offsets::CallID<"GGUUID::ToString", bool(*)(const GGUUID *, String *)>(this, &string);
    return string.str();
}
