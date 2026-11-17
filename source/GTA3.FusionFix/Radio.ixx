module;

#include <stdafx.h>

export module Radio;

// The mouse wheel turns the radio both ways: with the next station on one wheel direction, the other direction
// tunes back to the previous station.
namespace Radio
{
    constexpr int ChangeRadioStationAction = 16;
    constexpr int MouseWheelUp = 4;
    constexpr int MouseWheelDown = 5;

    struct CMouseControllerState
    {
        bool LMB, RMB, MMB, WHEELUP, WHEELDN, MXB1, MXB2;
    };

    CMouseControllerState* NewMouseState = nullptr;
    CMouseControllerState* OldMouseState = nullptr;
    void* ControlsManager = nullptr;
    int (__fastcall* GetMouseButtonAssociatedWithAction)(void* manager, void* edx, int action) = nullptr;
    bool (__fastcall* ChangeStationJustDown)(void* pad, void* edx) = nullptr;
    bool (__fastcall* IsMP3RadioChannelAvailable)(void* sampleManager, void* edx) = nullptr;
    void* SampleManager = nullptr;
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
        Previous = !*reinterpret_cast<uint8_t*>(uintptr_t(pad) + 0xDF) && PreviousStationJustDown();
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

            // cMusicManager::ServiceGameMode
            pattern = hook::pattern("89 C1 E8 ? ? ? ? 84 C0 74 72 80 3D ? ? ? ? 01 74 69 E8");
            ChangeStationJustDown = reinterpret_cast<decltype(ChangeStationJustDown)>(injector::GetBranchDestination(pattern.get_first(2)).as_int());
            injector::MakeCALL(pattern.get_first(2), ChangeStation, true);

            // A press counts one station on; a previous station press counts the whole round less one
            pattern = hook::pattern("C7 05 ? ? ? ? 1E 00 00 00 FF 05 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? E8");
            NumRetunePresses = *pattern.get_first<int32_t*>(12);
            SampleManager = *pattern.get_first<void*>(22);
            IsMP3RadioChannelAvailable = reinterpret_cast<decltype(IsMP3RadioChannelAvailable)>(injector::GetBranchDestination(pattern.get_first(26)).as_int());
            static auto RetuneHook = safetyhook::create_mid(pattern.get_first(10), [](SafetyHookContext& regs)
            {
                if (!Previous)
                    return;

                // Ten stations and off; the MP3 player makes it eleven
                const int stations = IsMP3RadioChannelAvailable(SampleManager, nullptr) ? 11 : 10;
                *NumRetunePresses = (*NumRetunePresses + stations - 2) % stations;
            });
        };
    }
} RadioInit;
