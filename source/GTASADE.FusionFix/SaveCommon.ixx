module;

#include "stdafx.h"

export module SaveCommon;

import Build;
import Unreal;
import Window;

export struct MissionState
{
    const uint8_t* scriptSpace = nullptr;
    const uint32_t* offset = nullptr;
    size_t scriptSize = 0;

    bool IsOnMission() const
    {
        if (!scriptSpace || !offset)
            return true;
        const auto index = *offset;
        if (!index)
            return false;
        if (scriptSize < sizeof(uint32_t) || index > scriptSize - sizeof(uint32_t))
            return true;
        uint32_t value = 0;
        std::memcpy(&value, scriptSpace + index, sizeof(value));
        return value != 0;
    }
};

export void RegisterQuicksave(std::function<bool()> canSave, std::function<void()> save)
{
    const auto state = reinterpret_cast<const int*>(GameStateAddress());
    auto held = std::make_shared<std::atomic_bool>((GetAsyncKeyState(VK_F5) & 0x8000) != 0);
    WFP::onActivateApp() += [held](bool)
    {
        // Holding F5 while switching windows must not save on reactivation.
        held->store((GetAsyncKeyState(VK_F5) & 0x8000) != 0);
    };
    WFP::onGameProcessEvent() += [held, state, canSave = std::move(canSave), save = std::move(save)]()
    {
        const bool pressed = (GetAsyncKeyState(VK_F5) & 0x8000) != 0;
        const bool previous = held->exchange(pressed);
        if (*state == GS_PLAYING_GAME && IsGameFocused() && pressed && !previous && canSave())
            save();
    };
}

