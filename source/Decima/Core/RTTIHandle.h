#pragma once

#include "Decima/PCore/Ref.h"
#include "Decima/Core/RTTI.h"

struct RTTIHandle {
    pcRTTI TypePtr; // uint64_t StaticTypePtr
    Ref<RTTIRefObject> VirtualResource;
};