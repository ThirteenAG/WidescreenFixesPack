#include "stdafx.h"
#include <assembly64.hpp>

uint32_t* OnAMissionFlag;
uint8_t* ScriptSpace;
uint8_t* m_WideScreenOn;
bool IsPlayerOnAMission()
{
    return *OnAMissionFlag && (ScriptSpace[*OnAMissionFlag] == 1);
}

void Init()
{
    CIniReader iniReader("");
    static auto nIniSaveSlot = (int32_t)iniReader.ReadInteger("MAIN", "SaveSlot", 6) - 1;
    if (nIniSaveSlot < 1 || nIniSaveSlot > 8)
        nIniSaveSlot = 5;

    auto pattern = hook::pattern("8B 05 ? ? ? ? 85 C0 74 0F 42 83 BC 20");
    OnAMissionFlag = (uint32_t*)injector::ReadRelativeOffset(pattern.get_first(2), 4, true).as_int();
    ScriptSpace = (uint8_t*)injector::ReadRelativeAddress(pattern.get_first(14), 4, true).as_int();

    pattern = hook::pattern("0F B6 05 ? ? ? ? 88 05 ? ? ? ? E8 ? ? ? ? 48 83 C4 28");
    m_WideScreenOn = (uint8_t*)injector::ReadRelativeOffset(pattern.get_first(3), 4, true).as_int();

    pattern = hook::pattern("40 53 48 83 EC 40 33 DB 48 8D 4C 24 ? 48 89 5C 24 ? 48 89 5C 24 ? 8D 53 07");
    static auto sub_140F07930 = (void(__fastcall*)())(pattern.get_first(0));
    static auto gxt_offset = pattern.get_first(66);
    static auto gxt_location = (char*)injector::ReadRelativeOffset(pattern.get_first(66), 4, true).as_int();
    DWORD out = 0;
    injector::UnprotectMemory(gxt_location, 7, out);
    static std::string_view gxt(gxt_location, 7);
    static auto FESZ_L1 = std::string("FESZ_L1");
    static auto CHEAT3 = std::string("CHEAT3");
    static auto gxt_ptr = const_cast<char*>(gxt.data());
    
    pattern = hook::pattern("0F B6 05 ? ? ? ? 48 8D 0D ? ? ? ? 48 69 C0 ? ? ? ? 48 8B 04 08 C7 80 ? ? ? ? ? ? ? ? E8");
    static auto ret_addr = (uintptr_t)pattern.get_first(0);
    static uint8_t ret_code[6] = { 0x48, 0x83, 0xC4, 0x40, 0x5B, 0xC3 };
    static uint8_t og_code[6] = { 0 };
    injector::ReadMemoryRaw(ret_addr, og_code, sizeof(og_code), true);

    pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? 8B 15 ? ? ? ? E8");
    static auto SaveToSlot = (void(__fastcall*)(int))(injector::GetBranchDestination(pattern.get_first(0)).as_int());

    pattern = hook::pattern("BA ? ? ? ? 89 1D ? ? ? ? E8 ? ? ? ? 48 8D 15 ? ? ? ? C7 05");
    static auto Slot = pattern.get_first(1);

    pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 40 38 2D ? ? ? ? 75 13 48 8D 0D ? ? ? ? C6 05");
    static auto sub_140F01E00 = (void(__fastcall*)())(injector::GetBranchDestination(pattern.get_first(0)).as_int());
    struct IdleHook
    {
        void operator()(injector::reg_pack& regs)
        {
            sub_140F01E00();

            static bool bF5LastState = false;
            bool bF5CurState = GetKeyState(VK_F5) & 0x8000;

            if (!bF5LastState && bF5CurState)
            {
                if (!IsPlayerOnAMission() && !*m_WideScreenOn)
                {
                    injector::WriteMemory<uint32_t>(Slot, 5, true);
                    SaveToSlot(nIniSaveSlot);
                    injector::WriteMemory<uint32_t>(Slot, 8, true);

                    injector::WriteMemoryRaw(ret_addr, ret_code, sizeof(ret_code), true);
                    FESZ_L1.copy(gxt_ptr, gxt.length() + 1);
                    sub_140F07930();
                    CHEAT3.copy(gxt_ptr, gxt.length() + 1);
                    injector::WriteMemoryRaw(ret_addr, og_code, sizeof(og_code), true);
                }
            }

            bF5LastState = bF5CurState;
        }
    }; injector::MakeInline<IdleHook>(pattern.get_first(0));
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("E8 ? ? ? ? 0F B7 C5 41 B8"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
