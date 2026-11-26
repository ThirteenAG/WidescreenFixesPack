module;

#include <stdafx.h>

export module MatrixOptions;

import ComVars;

export void InitMatrixOptions()
{
    static std::vector<std::string> list;

    hook::pattern pattern;
    nOptionsMode.SetAddress(*hook::get_pattern<uint32_t*>("8B 0D ? ? ? ? 8D 54 24 0C 51", 2));

    // The combo-box HWND offset and inline stub length differ between game versions.
    static uint32_t nHwndOffset = 0x13E8;
    static ptrdiff_t resListHookEnd = 126;
    if (!hook::pattern("8B 45 04 85 C0 74 ? A1 ? ? ? ? 89 44 24 14").empty()) // updated release
    {
        nHwndOffset = 0x1460;
        resListHookEnd = 125;
    }

    pattern = hook::pattern("8B 45 04 85 C0 74");
    struct ResListHook
    {
        void operator()(injector::reg_pack& regs)
        {
            const auto [width, height] = MatrixNativeResolution();
            const auto iniRes = std::format("{}x{}", width, height);
            list.clear();
            GetResolutionsList(list);
            const auto selected = std::find(list.begin(), list.end(), iniRes);
            const auto mode = std::distance(list.begin(), selected);
            if (selected == list.end())
                list.push_back(iniRes);

            const auto combo = *reinterpret_cast<HWND*>(regs.esi + nHwndOffset);
            SendMessageA(combo, CB_RESETCONTENT, 0, 0);
            for (const auto& resolution : list)
                SendMessageA(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(resolution.c_str()));
            SendMessageA(combo, CB_SETCURSEL, mode, 0);
            nOptionsMode = static_cast<uint32_t>(mode);
        }
    }; injector::MakeInline<ResListHook>(pattern.get_first(0), pattern.get_first(resListHookEnd)); //0x40600D, 0x40608B

    // Replace the resolution argument before the options tool writes its own profile.
    pattern = hook::pattern("8B 44 24 18 8B 35 ? ? ? ? 83 C4 0C 68 ? ? ? ? 50 68 ? ? ? ? 68 ? ? ? ? FF D6");
    static auto writeResolution = safetyhook::create_mid(pattern.get_first(18), [](SafetyHookContext& ctx)
    {
        const uint32_t mode = nOptionsMode;
        if (mode < list.size())
            ctx.eax = reinterpret_cast<uintptr_t>(list[mode].c_str());
    });
}
