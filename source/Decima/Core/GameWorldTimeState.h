#pragma once

#include "Decima/Core/RTTIRefObject.h"

class GameWorldTimeState : public RTTIRefObject {
public:
    float mTimeOfDay;
    float mUnk24;
    uint32_t mDay;
    bool mIsDayNightCycleEnabled;
    bool mIsPaused;

    uint32_t mUnk30;
    uint32_t mUnk34;
    float mDayNightCycleDuration;

    void SetTimeOfDay(float inTimeOfDay, float inTransitionDuration) {
        static const auto func = Offsets::ResolveID<"GameWorldTimeState::SetTimeOfDay", void(*)(GameWorldTimeState *, float, float)>();
        func(this, inTimeOfDay, inTransitionDuration);
    }
};

assert_size(GameWorldTimeState, 0x40);
