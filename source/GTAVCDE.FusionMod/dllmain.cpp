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

    auto pattern = hook::pattern("8B 0D ? ? ? ? 42 8B 84 31 ? ? ? ? 85 C9 74 0B FF C8 83 F8 01 0F 86");
    OnAMissionFlag = (uint32_t*)injector::ReadRelativeOffset(pattern.get_first(2), 4, true).as_int();
    ScriptSpace = (uint8_t*)injector::ReadRelativeAddress(pattern.get_first(10), 4, true).as_int();

    pattern = hook::pattern("38 1D ? ? ? ? 48 8D 3D ? ? ? ? 0F 57 FF 74 1D F3 0F 10 05 ? ? ? ? 0F 2F C7 76 10 45 33 C0");
    m_WideScreenOn = (uint8_t*)injector::ReadRelativeOffset(pattern.get_first(2), 4, true).as_int();

    pattern = hook::pattern("40 53 48 83 EC 40 33 DB 48 8D 4C 24 ? 48 89 5C 24 ? 48 89 5C 24 ? 8D 53 07");
    static auto sub_140F19E90 = (void(__fastcall*)())(pattern.get_first(0));
    static auto gxt_offset = pattern.get_first(66);
    static auto gxt_location = (char*)injector::ReadRelativeOffset(pattern.get_first(66), 4, true).as_int();
    DWORD out = 0;
    injector::UnprotectMemory(gxt_location, 7, out);
    static std::string_view gxt(gxt_location, 7);
    static auto FESZ_WR = std::string("FESZ_WR");
    static auto CHEAT2 = std::string("CHEAT2");
    static auto gxt_ptr = const_cast<char*>(gxt.data());
    
    pattern = hook::pattern("45 33 C0 B9 ? ? ? ? 41 8D 50 01 E8 ? ? ? ? 45 33 C0 B9 ? ? ? ? 41 8D 50 01 E8 ? ? ? ? 45 33 C0 B9");
    static auto ret_addr = (uintptr_t)pattern.get_first(0);

    pattern = hook::pattern("E8 ? ? ? ? 32 C0 EB 46");
    static auto SaveToSlot = (void(__fastcall*)(int))(injector::GetBranchDestination(pattern.get_first(0)).as_int());

    pattern = hook::pattern("BA ? ? ? ? E8 ? ? ? ? B0 01 C7 05 ? ? ? ? ? ? ? ? E9");
    static auto Slot = pattern.get_first(1);

    pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? EB 13");
    static auto sub_140F3BA50 = (void(__fastcall*)())(injector::GetBranchDestination(pattern.get_first(0)).as_int());
    struct IdleHook
    {
        void operator()(injector::reg_pack& regs)
        {
            sub_140F3BA50();

            static bool bF5LastState = false;
            bool bF5CurState = GetKeyState(VK_F5) & 0x8000;

            if (!bF5LastState && bF5CurState)
            {
                if (!IsPlayerOnAMission() && !*m_WideScreenOn)
                {
                    injector::WriteMemory<uint32_t>(Slot, 5, true);
                    SaveToSlot(nIniSaveSlot);
                    injector::WriteMemory<uint32_t>(Slot, 8, true);

                    injector::WriteMemory<uint32_t>(ret_addr, 0x40C48348, true);     //add     rsp, 40h
                    injector::WriteMemory<uint8_t>(ret_addr + 4, 0x5B, true);        //pop     rbx
                    injector::WriteMemory<uint8_t>(ret_addr + 5, 0xC3, true);        //ret
                    FESZ_WR.copy(gxt_ptr, gxt.length() + 1);
                    sub_140F19E90();
                    CHEAT2.copy(gxt_ptr, gxt.length() + 1);
                    injector::WriteMemory<uint32_t>(ret_addr, 0xB9C03345, true);     //xor     r8d, r8d
                    injector::WriteMemory<uint8_t>(ret_addr + 4, 0x0D, true);        //mov     ecx, 10Dh
                    injector::WriteMemory<uint8_t>(ret_addr + 5, 0x01, true);        //
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
