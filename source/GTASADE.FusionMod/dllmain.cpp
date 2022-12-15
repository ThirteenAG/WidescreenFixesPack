#include "stdafx.h"
#include <assembly64.hpp>

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

    auto pattern = hook::pattern("8B 05 ? ? ? ? 85 C0 74 0E 4C 8D 05 ? ? ? ? 42 83 3C 00");
    OnAMissionFlag = (uint32_t*)injector::ReadRelativeOffset(pattern.get_first(2), 4, true).as_int();
    ScriptSpace = (uint8_t*)injector::ReadRelativeOffset(pattern.get_first(13), 4, true).as_int();

    pattern = hook::pattern("80 3D ? ? ? ? ? 48 8D 1D ? ? ? ? 0F 94 05 ? ? ? ? 66 90");
    m_WideScreenOn = (uint8_t*)injector::ReadRelativeOffset(pattern.get_first(2), 4, true).as_int();

    pattern = hook::pattern("BA ? ? ? ? 48 89 5C 24 ? 48 8D 4C 24 ? 48 89 5C 24 ? E8 ? ? ? ? 8B 7C 24 58 83 C7 08");
    static auto SetHelpMessagePrologue = injector::raw_mem(pattern.get_first(0), [](x86::Assembler& a)
    {
        a.push(rbx);
        a.sub(rsp, 0x40);
        a.xor_(rbx, rbx);
    }, true);
    static auto sub_140F7A567 = (void(__fastcall*)())(pattern.get_first(-SetHelpMessagePrologue.Size()));
    static auto gxt_offset = pattern.get_first(64);
    static auto gxt_location = (char*)injector::ReadRelativeOffset(gxt_offset, 4, true).as_int();
    injector::UnprotectMemory(gxt_location, 7);
    static std::string_view gxt(gxt_location, 7);
    static auto FESZ_WR = std::string("FESZ_WR");
    static auto USJ_ALL = std::string("USJ_ALL");
    static auto gxt_ptr = const_cast<char*>(gxt.data());
    
    pattern = hook::pattern("48 8D 15 ? ? ? ? E8 ? ? ? ? 48 85 C0 74 1E 41 B8 ? ? ? ? C7 44 24");
    static auto SetHelpMessageEpilogue = injector::raw_mem(pattern.get_first(0), [](x86::Assembler& a)
    {
        a.add(rsp, 0x40);
        a.pop(rbx);
        a.ret();
    });

    pattern = hook::pattern("40 53 48 83 EC 20 45 33 C0 8B DA E8 ? ? ? ? 0F B6 D3 E8 ? ? ? ? 48 83 C4 20 5B E9");
    static auto SaveToSlot = (void(__fastcall*)(int, int))(pattern.get_first(0));

    pattern = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 0F B6 15 ? ? ? ? F3 0F 10 05 ? ? ? ? F3 0F 59 05 ? ? ? ? 48 8B 7C 24");
    static auto DoGameSpecificStuffBeforeSaveTime = pattern.get_first(1);

    pattern = hook::pattern("E8 ? ? ? ? 4C 89 25 ? ? ? ? E8 ? ? ? ? EB 3C");
    static auto sub_140FC7ED0 = (void(__fastcall*)())(injector::GetBranchDestination(pattern.get_first(0)).as_int());
    struct IdleHook
    {
        void operator()(injector::reg_pack& regs)
        {
            sub_140FC7ED0();

            static bool bF5LastState = false;
            bool bF5CurState = GetKeyState(VK_F5) & 0x8000;

            if (!bF5LastState && bF5CurState)
            {
                if (!IsPlayerOnAMission() && !*m_WideScreenOn)
                {
                    injector::WriteMemory<uint32_t>(DoGameSpecificStuffBeforeSaveTime, 0, true);
                    SaveToSlot(0x708, nIniSaveSlot);
                    injector::WriteMemory<uint32_t>(DoGameSpecificStuffBeforeSaveTime, 360, true);

                    SetHelpMessagePrologue.Write();
                    SetHelpMessageEpilogue.Write();
                    FESZ_WR.copy(gxt_ptr, gxt.length() + 1);
                    sub_140F7A567();
                    USJ_ALL.copy(gxt_ptr, gxt.length() + 1);
                    SetHelpMessageEpilogue.Restore();
                    SetHelpMessagePrologue.Restore();
                }
            }

            bF5LastState = bF5CurState;
        }
    }; injector::MakeInline<IdleHook>(pattern.get_first(0));

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
        CallbackHandler::RegisterCallback(Init, hook::pattern("E8 ? ? ? ? 4C 89 25 ? ? ? ? E8 ? ? ? ? EB 3C"));
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
