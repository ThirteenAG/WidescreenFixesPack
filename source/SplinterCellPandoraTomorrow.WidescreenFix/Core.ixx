module;

#include <stdafx.h>

export module Core;

import ComVars;

SafetyHookInline shappFromAnsi = {};
wchar_t* __cdecl appFromAnsi(const char* a1)
{
    bool bReqPtr = false;
    std::vector<int> offsets = { 0, 24, 48, 74, 101 };
    for (size_t i = 0; i < ResList.size(); i++)
    {
        const auto& r = ResList[i].first;
        std::string str(r.begin(), r.end());
        if (std::string_view(a1 + offsets[i]) == str)
            bReqPtr = true;
        else
        {
            bReqPtr = false;
            break;
        }
    }

    if (bReqPtr)
    {
        for (size_t i = 0; i < ResList.size(); i++)
        {
            const auto& r = ResList[i].second;
            std::string str(r.begin(), r.end());
            injector::scoped_unprotect p{ const_cast<char*>(a1 + offsets[i]), str.size() + 1 };
            strcpy_s(const_cast<char*>(a1 + offsets[i]), str.size() + 1, str.c_str());
        }
    }

    return shappFromAnsi.unsafe_ccall<wchar_t*>(a1);
}

namespace UObject
{
    wchar_t* (__fastcall* GetFullName)(void*, void*, wchar_t*) = nullptr;

    void* pPlayerObj = nullptr;
    SafetyHookInline shGotoState = {};
    int __fastcall GotoState(void* uObject, void* edx, int StateID)
    {
        wchar_t buffer[256];
        auto objName = std::wstring_view(GetFullName(uObject, edx, buffer));

        if (objName.starts_with(L"EPlayerController "))
        {
            EPlayerControllerState = StateID;
        }
        else if (objName.starts_with(L"EchelonMainHUD "))
        {
            EchelonMainHUDState = StateID;
        }
        else if (objName.starts_with(L"EGameInteraction "))
        {
            EGameInteractionState = StateID;
        }
        else if (objName.starts_with(L"EElevatorPanel "))
        {
            bKeyPad = false;
            bElevatorPanel = true;
        }
        else if (objName.starts_with(L"EKeyPad "))
        {
            bKeyPad = true;
            bElevatorPanel = false;
        }
        else if (objName.starts_with(L"EDoorMarker "))
        {
            EDoorMarkerState = StateID;
        }

        return shGotoState.unsafe_fastcall<int>(uObject, edx, StateID);
    }
}

export void InitCore()
{
    auto pattern = find_module_pattern(GetModuleHandle(L"Core"), "C7 85 D4 F1 FF FF 00 00 00 00", "C7 85 ? ? ? ? ? ? ? ? EB ? 8B 8D ? ? ? ? 83 C1 ? 89 8D ? ? ? ? 81 BD ? ? ? ? ? ? ? ? 0F 83"); //0x1000CE5E
    uint32_t pfappInit = (uint32_t)pattern.get_first();

    auto rpattern = hook::range_pattern(pfappInit, pfappInit + 0x900, "80 02 00 00");
    injector::WriteMemory<uint16_t>(rpattern.count(2).get(0).get<uint32_t>(-6), 0x7EEB, true); //pfappInit + 0x5FC
    injector::WriteMemory(rpattern.count(2).get(1).get<uint32_t>(0), Screen.Width, true);  //pfappInit + 0x67E + 0x1
    rpattern = hook::range_pattern(pfappInit, pfappInit + 0x900, "E0 01 00 00");
    injector::WriteMemory(rpattern.count(2).get(1).get<uint32_t>(0), Screen.Height, true); //pfappInit + 0x69F + 0x1

    // set player speed to max on game start
    pattern = hook::module_pattern(GetModuleHandle(L"Core"), "8B D9 C1 E9 02 83 E3 03 F3 A5 8B CB F3 A4 5B 5F 5E 59 8B F0");
    if (!pattern.empty())
    {
        static auto UObjectInitPropertiesHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            EchelonGameInfoPtrs.push_back(regs.edi - 0x2C);
        });
    }
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Core"), "8D 47 ? 50 8D 42");
        static auto UObjectInitPropertiesHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            EchelonGameInfoPtrs.push_back(regs.esi);
        });
    }

    pattern = find_module_pattern(GetModuleHandle(L"Core"), "55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 51 83 EC ? 53 56 57 89 65 ? C7 45 ? ? ? ? ? 83 7D ? ? 75 ? 33 C0 E9 ? ? ? ? E8 ? ? ? ? 89 45 ? 8B 45 ? 89 45 ? 33 C9 74 ? EB ? C7 45 ? ? ? ? ? EB ? 8B 55 ? 83 C2 ? 89 55 ? 81 7D ? ? ? ? ? 7D ? 8B 45 ? 03 45",
        "55 8B EC 83 EC ? 83 7D ? ? 75 ? 33 C0 EB ? E8 ? ? ? ? 89 45 ? C7 45 ? ? ? ? ? EB ? 8B 45 ? 83 C0 ? 89 45 ? 81 7D ? ? ? ? ? 7D ? 8B 4D ? 03 4D");
    shappFromAnsi = safetyhook::create_inline(pattern.get_first(), appFromAnsi);

    pattern = find_module_pattern(GetModuleHandle(L"Core"), "55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 83 EC ? 53 56 8B 75 ? 85 F6 57 89 65 ? 8B F9 C7 45 ? ? ? ? ? 75 ? E8 ? ? ? ? 8B F0 85 FF", "55 8B EC 53 56 8B 75 ? 8B D9 85 F6 75");
    UObject::GetFullName = (decltype(UObject::GetFullName))pattern.get_first();

    pattern = find_module_pattern(GetModuleHandle(L"Core"), "55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 83 EC ? 53 56 57 8B F1 8B 46 ? 33 FF 3B C7 89 65 ? 89 7D ? 75 ? 33 C0", "55 8B EC 83 EC ? 53 56 57 8B F9 8B 47");
    UObject::shGotoState = safetyhook::create_inline(pattern.get_first(), UObject::GotoState);

    pattern = hook::module_pattern(GetModuleHandle(L"Core"), "33 C0 0F B7 0B");
    if (!pattern.empty())
    {
        static auto OverwriteResolutions = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            auto str = std::wstring_view((const wchar_t*)regs.ebx);

            for (const auto& it : ResList)
            {
                if (str == it.first)
                {
                    wcscpy_s((wchar_t*)regs.ebx, it.second.size() + 1, it.second.c_str());
                    break;
                }
            }
        });
    }
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"Core"), "33 C0 66 85 C9");
        static auto OverwriteResolutions = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            auto str = std::wstring_view((const wchar_t*)regs.esi);

            for (const auto& it : ResList)
            {
                if (str == it.first)
                {
                    wcscpy_s((wchar_t*)regs.esi, it.second.size() + 1, it.second.c_str());
                    break;
                }
            }
        });
    }
}