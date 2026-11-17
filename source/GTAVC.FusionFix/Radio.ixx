module;

#include <stdafx.h>

export module Radio;

// The mouse wheel turns the radio both ways: with the next station on one wheel direction, the other direction
// tunes back to the previous station.
namespace Radio
{
    constexpr int ChangeRadioStationAction = 18;
    constexpr int MouseWheelUp = 4;
    constexpr int MouseWheelDown = 5;
    constexpr int Stations = 11;            // nine stations, the MP3 player and off
    constexpr uint8_t MP3Player = 9;
    constexpr uint8_t RadioOff = 10;

    struct CMouseControllerState
    {
        bool LMB, RMB, MMB, WHEELUP, WHEELDN, MXB1, MXB2;
    };

    CMouseControllerState* NewMouseState = nullptr;
    CMouseControllerState* OldMouseState = nullptr;
    void* ControlsManager = nullptr;
    int (__fastcall* GetMouseButtonAssociatedWithAction)(void* manager, void* edx, int action) = nullptr;
    bool (__fastcall* ChangeStationJustDown)(void* pad, void* edx) = nullptr;
    bool (__fastcall* IsMP3RadioChannelAvailable)(void* audio, void* edx) = nullptr;
    void* DMAudio = nullptr;
    int32_t* NumRetunePresses = nullptr;
    bool Previous = false;

    bool PreviousStationJustDown()
    {
        switch (GetMouseButtonAssociatedWithAction(ControlsManager, nullptr, ChangeRadioStationAction))
        {
        case MouseWheelUp:
            return NewMouseState->WHEELDN && !OldMouseState->WHEELDN;
        case MouseWheelDown:
            return NewMouseState->WHEELUP && !OldMouseState->WHEELUP;
        default:
            return false;
        }
    }

    bool __fastcall ChangeStation(void* pad, void* edx)
    {
        Previous = false;
        if (ChangeStationJustDown(pad, edx))
            return true;

        // CPad::m_bDisablePlayerControls, as the pad's own check reads it
        Previous = !*reinterpret_cast<uint16_t*>(uintptr_t(pad) + 0xF0) && PreviousStationJustDown();
        return Previous;
    }
}

class RadioInit
{
public:
    RadioInit()
    {
        WFP::onInitEvent() += []()
        {
            using namespace Radio;

            auto pattern = hook::pattern("80 3D ? ? ? ? 00 74 12 31 C0 80 3D ? ? ? ? 00 75 01 40");
            NewMouseState = *pattern.get_first<CMouseControllerState*>(2);
            OldMouseState = *pattern.get_first<CMouseControllerState*>(13);

            pattern = hook::pattern("E8 ? ? ? ? B9 ? ? ? ? 89 C6 6A 08 E8");
            ControlsManager = *pattern.get_first<void*>(6);
            GetMouseButtonAssociatedWithAction = reinterpret_cast<decltype(GetMouseButtonAssociatedWithAction)>(injector::GetBranchDestination(pattern.get_first()).as_int());

            // cMusicManager::GetNextCarTuning
            pattern = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00 0F B6 83 3C 02 00 00");
            DMAudio = *pattern.get_first<void*>(1);
            IsMP3RadioChannelAvailable = reinterpret_cast<decltype(IsMP3RadioChannelAvailable)>(injector::GetBranchDestination(pattern.get_first(5)).as_int());

            // cMusicManager::ServiceGameMode
            pattern = hook::pattern("89 C1 E8 ? ? ? ? 84 C0 0F 84 ? ? ? ? 85 F6 0F 84");
            ChangeStationJustDown = reinterpret_cast<decltype(ChangeStationJustDown)>(injector::GetBranchDestination(pattern.get_first(2)).as_int());
            injector::MakeCALL(pattern.get_first(2), ChangeStation, true);

            // A press counts one station on; a previous station press counts the whole round less one. Without
            // the MP3 player the station name display steps over its slot forwards, so step over it backwards here
            pattern = hook::pattern("FF 05 ? ? ? ? C7 05 ? ? ? ? 14 00 00 00 C6 05");
            NumRetunePresses = *pattern.get_first<int32_t*>(2);
            static auto RetuneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (!Previous)
                    return;

                // The player's vehicle, with its station at +0x23C
                uint8_t station = *reinterpret_cast<uint8_t*>(regs.esi + 0x23C);
                if (station > RadioOff)
                    station = RadioOff;

                int presses = Stations - 1;
                if ((station + *NumRetunePresses + presses) % Stations == MP3Player && !IsMP3RadioChannelAvailable(DMAudio, nullptr))
                    --presses;

                *NumRetunePresses = (*NumRetunePresses + presses - 1) % Stations;
            });
        };
    }
} RadioInit;
