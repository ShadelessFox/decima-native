#pragma once

#include "Decima/Core/EntityUpdateListener.h"
#include "Decima/Core/NetMessageListener.h"
#include "Decima/Core/NetSubSystemListener.h"
#include "Decima/Core/GameWorldTimeState.h"
#include "Decima/Core/Module.h"

class GameModule : public Module, public EntityUpdaterListener, public NetMessageListener, public NetSubSystemListener {
public:
    static GameModule *Get() {
        return *Offsets::ResolveID<"GameModule::Instance", GameModule **>();
    }

    uint8_t mUnk40[0x18];
    GameWorldTimeState *mGameWorldTimeState;
};
