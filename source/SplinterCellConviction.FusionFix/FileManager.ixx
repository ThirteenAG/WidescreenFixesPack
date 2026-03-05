module;

#include <stdafx.h>

export module FileManager;

import ComVars;
import BlacklistControls;

namespace FFileManagerArc
{
    SafetyHookInline shLookup{};
    void* __fastcall Lookup(void* pFFileManagerArc, void* edx, const char* path)
    {
        if (bBlacklistControlScheme)
            OverrideForBlacklistControls(path);

        if (GetOverloadedFilePathA(path, nullptr/*s.data()*/, 0/*s.size()*/))
            return nullptr;
        return shLookup.fastcall<void*>(pFFileManagerArc, edx, path);
    }
}

namespace FFileManagerLinear
{
    int* dword_13A334C = nullptr;
    SafetyHookInline shLookup{};
    void* __fastcall Lookup(void* pFFileManagerLinear, void* edx, const char* path, int a3)
    {
        if (bBlacklistControlScheme)
            OverrideForBlacklistControls(path);

        if (GetOverloadedFilePathA(path, nullptr/*s.data()*/, 0/*s.size()*/))
        {
            int cur = *dword_13A334C;
            *dword_13A334C = 1;
            auto ret = shLookup.fastcall<void*>(pFFileManagerLinear, edx, path, a3);
            *dword_13A334C = cur;
            return ret;
        }
        return shLookup.fastcall<void*>(pFFileManagerLinear, edx, path, a3);
    }

    static int curdword_13A3348 = 0;
    int* dword_13A3348 = nullptr;
    static injector::hook_back<void(__fastcall*)(void*, void*, const char*)> hbsub_43A943;
    void __fastcall sub_43A943(void* _this, void* edx, const char* path)
    {
        curdword_13A3348 = *dword_13A3348;

        if (bBlacklistControlScheme)
            OverrideForBlacklistControls(path);

        if (GetOverloadedFilePathA(path, nullptr/*s.data()*/, 0/*s.size()*/))
        {
            *dword_13A3348 = 0;
        }

        return hbsub_43A943.fun(_this, edx, path);
    }
}

export void InitFileManager()
{
    //allow loading unpacked files

    ModuleList dlls;
    dlls.Enumerate(ModuleList::SearchLocation::LocalOnly);
    for (auto& e : dlls.m_moduleList)
    {
        auto m = std::get<HMODULE>(e);
        if (IsModuleUAL(m))
        {
            GetOverloadedFilePathA = (decltype(GetOverloadedFilePathA))GetProcAddress(m, "GetOverloadedFilePathA");
            break;
        }
    }

    if (GetOverloadedFilePathA)
    {
        auto pattern = hook::pattern("83 3D ? ? ? ? ? 56 8B F1 75 5C");
        FFileManagerLinear::dword_13A334C = *pattern.get_first<int*>(2);

        pattern = hook::pattern("39 1D ? ? ? ? 0F 84 ? ? ? ? 8B 86");
        FFileManagerLinear::dword_13A3348 = *pattern.get_first<int*>(2);

        pattern = hook::pattern("55 8B EC 83 EC 18 53 56 57 FF 75 08 8D 45 E8");
        FFileManagerArc::shLookup = safetyhook::create_inline(pattern.get_first(), FFileManagerArc::Lookup);

        pattern = hook::pattern("55 8B EC 83 EC 18 83 3D ? ? ? ? ? 56 8B F1 75 5C");
        FFileManagerLinear::shLookup = safetyhook::create_inline(pattern.get_first(), FFileManagerLinear::Lookup);

        pattern = hook::pattern("E8 ? ? ? ? 39 1D ? ? ? ? 0F 84 ? ? ? ? 8B 86");
        FFileManagerLinear::hbsub_43A943.fun = injector::MakeCALL(pattern.get_first(), FFileManagerLinear::sub_43A943, true).get();

        pattern = hook::pattern("8D 4D E4 E8 ? ? ? ? 8B C7 5F 5E 5B C9 C2 10 00");
        static auto restore = safetyhook::create_mid(pattern.get_first(8), [](SafetyHookContext& regs)
        {
            *FFileManagerLinear::dword_13A3348 = FFileManagerLinear::curdword_13A3348;
        });
    }
}