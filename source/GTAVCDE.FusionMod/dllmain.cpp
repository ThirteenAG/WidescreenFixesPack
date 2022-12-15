#include "stdafx.h"
#include <assembly64.hpp>
#include <string_view>

float fHudScale = 1.0f;
float fRadarScale = 1.0f;

uint32_t* OnAMissionFlag;
uint8_t* ScriptSpace;
uint8_t* m_WideScreenOn;
bool IsPlayerOnAMission()
{
    return *OnAMissionFlag && (ScriptSpace[*OnAMissionFlag] == 1);
}

int vscanf_hook(char* Buffer, char* Format, ...)
{
    std::string temp;
    std::string_view buf(Buffer);
    if (buf.contains("WIDGET_POSITION_PLAYER_INFO") || buf.contains("WIDGET_POSITION_RADAR"))
    {
        std::string PositionAndScale(255, '\0');
        float OriginX, OriginY, ScaleX, ScaleY;
        std::string Separator(255, '\0');
        std::string Name(255, '\0');
        auto r = sscanf(Buffer, Format, PositionAndScale.data(), &OriginX, &OriginY, &ScaleX, &ScaleY, Separator.data(), Name.data());
        if (r == 7)
        {
            if (buf.contains("WIDGET_POSITION_PLAYER_INFO"))
            {
                OriginX = OriginX + (ScaleX - (ScaleX * fHudScale));
                OriginY = OriginY - (ScaleY - (ScaleY * fHudScale));
                ScaleX *= fHudScale;
                ScaleY *= fHudScale;
            }
            else if (buf.contains("WIDGET_POSITION_RADAR"))
            {
                OriginX = OriginX - (ScaleX - (ScaleX * fRadarScale));
                OriginY = OriginY + (ScaleY - (ScaleY * fRadarScale));
                ScaleX *= fRadarScale;
                ScaleY *= fRadarScale;
            }

            PositionAndScale.resize(PositionAndScale.find('\0'));
            Separator.resize(Separator.find('\0'));
            Name.resize(Name.find('\0'));
            temp += PositionAndScale;
            temp += " ";
            temp += std::to_string(OriginX);
            temp += " ";
            temp += std::to_string(OriginY);
            temp += " ";
            temp += std::to_string(ScaleX);
            temp += " ";
            temp += std::to_string(ScaleY);
            temp += " ";
            temp += Separator;
            temp += " ";
            temp += Name;
            Buffer = temp.data();
        }
    }

    int result;
    va_list arglist;
    va_start(arglist, Format);
    result = vsscanf(Buffer, Format, arglist);
    va_end(arglist);
    return result;
}

void Init()
{
    CIniReader iniReader("");
    static auto nIniSaveSlot = (int32_t)iniReader.ReadInteger("MAIN", "SaveSlot", 6) - 1;
    fHudScale = iniReader.ReadFloat("MAIN", "HudScale", 0.88f);
    fRadarScale = iniReader.ReadFloat("MAIN", "RadarScale", 0.78f);
    if (fHudScale <= 0.0f)
        fHudScale = 1.0f;
    if (fRadarScale <= 0.0f)
        fRadarScale = 1.0f;
    if (nIniSaveSlot < 1 || nIniSaveSlot > 8)
        nIniSaveSlot = 5;
    static auto bIniDisableFirstPersonAimForRifles = iniReader.ReadInteger("MAIN", "DisableFirstPersonAimForRifles", 1) != 0;

    auto pattern = hook::pattern("8B 0D ? ? ? ? 8B 84 39 ? ? ? ? 85 C9 74 0B FF C8 83 F8 01");
    OnAMissionFlag = (uint32_t*)injector::ReadRelativeOffset(pattern.get_first(2), 4, true).as_int();
    ScriptSpace = (uint8_t*)injector::ReadRelativeAddress(pattern.get_first(9), 4, true).as_int();

    pattern = hook::pattern("88 15 ? ? ? ? 89 15 ? ? ? ? 48 89 15");
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
    
    pattern = hook::pattern("45 33 C0 B9 ? ? ? ? 41 8D 50 01 E8 ? ? ? ? 45 33 C0 B9 ? ? ? ? 41 8D 50 01 E8 ? ? ? ? 45 33 C0 B9 ? ? ? ? 41 8D 50 01 E8 ? ? ? ? 45 33 C0 B9 ? ? ? ? 41 8D 50 01 E8 ? ? ? ? 45 33 C0 B9 ? ? ? ? 41 8D 50 01 E8 ? ? ? ? 45 33 C0 B9 ? ? ? ? 41 8D 50 01 E8 ? ? ? ? 45 33 C0 B9 ? ? ? ? 41 8D 50 01 E8 ? ? ? ? 45 33 C0 B9 ? ? ? ? 41 8D 50 01 E8 ? ? ? ? 45 33 C0 B9 ? ? ? ? 41 8D 50 01 E8 ? ? ? ? 48 8B 0D ? ? ? ? 48 8B 01 FF 90 ? ? ? ? 48 8B C8 B2 01 E8 ? ? ? ? 48 8B 0D ? ? ? ? E8 ? ? ? ? 0F B6 05 ? ? ? ? 48 8D 1D ? ? ? ? 48 69 C8 ? ? ? ? BA ? ? ? ? 44 8B C2");
    static auto SetHelpMessageEpilogue = injector::raw_mem(pattern.get_first(0), [](x86::Assembler& a)
    {
        a.add(rsp, 0x40);
        a.pop(rbx);
        a.ret();
    });

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

                    SetHelpMessageEpilogue.Write();
                    FESZ_WR.copy(gxt_ptr, gxt.length() + 1);
                    sub_140F19E90();
                    CHEAT2.copy(gxt_ptr, gxt.length() + 1);
                    SetHelpMessageEpilogue.Restore();
                }
            }

            bF5LastState = bF5CurState;
        }
    }; injector::MakeInline<IdleHook>(pattern.get_first(0));

    if (bIniDisableFirstPersonAimForRifles)
    {
        pattern = hook::pattern("49 BD ? ? ? ? ? ? ? ? 48 89 BC 24");
        struct ProcessPlayerWeaponHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.r13 = 0x600400000180;

                if (regs.rax)
                {
                    enum eWeaponType
                    {
                        WEAPONTYPE_M4 = 26,
                        WEAPONTYPE_RUGER = 27,
                        WEAPONTYPE_M60 = 32
                    };

                    switch (regs.rsi)
                    {
                    case WEAPONTYPE_M4:
                    case WEAPONTYPE_RUGER:
                    case WEAPONTYPE_M60:
                        regs.rax = 0;
                        break;
                    default:
                        break;
                    }
                }
            }
        }; injector::MakeInline<ProcessPlayerWeaponHook>(pattern.get_first(0), pattern.get_first(10));
    }

    if (fHudScale != 1.0f || fRadarScale != 1.0f)
    {
        pattern = hook::pattern("E8 ? ? ? ? E9 ? ? ? ? 48 8D 45 90");
        injector::MakeCALLTrampoline(pattern.get_first(0), vscanf_hook, true);
    }
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
