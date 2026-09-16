module;

#include <stdafx.h>

export module MatrixOptions;

import ComVars;

export void InitMatrixOptions()
{
    static std::vector<std::string> list;
    GetResolutionsList(list);

    static auto dwordOptionsMode = *hook::get_pattern<uint32_t*>("8B 0D ? ? ? ? 8D 54 24 0C 51", 2);

    // The combo-box HWND offset and inline stub length differ between game versions.
    static uint32_t nHwndOffset = 0x13E8;
    static ptrdiff_t resListHookEnd = 126;
    if (!hook::pattern("8B 45 04 85 C0 74 6A A1 F8").empty()) // updated release
    {
        nHwndOffset = 0x1460;
        resListHookEnd = 125;
    }

    auto pattern = hook::pattern("8B 45 04 85 C0 74");
    struct ResListHook
    {
        void operator()(injector::reg_pack& regs)
        {
            char iniPath[MAX_PATH];
            GetModuleFileNameA(NULL, iniPath, MAX_PATH);
            *strrchr(iniPath, '\\') = '\0';
            strcat_s(iniPath, "\\MatrixConfig.ini");
            CIniReader iniReader(iniPath);
            int32_t iniResX = 0, iniResY = 0;
            std::tie(iniResX, iniResY) = GetDesktopRes();
            char defVal[20];
            sprintf_s(defVal, "%dx%d", iniResX, iniResY);
            auto iniRes = iniReader.ReadString("RENDERING", "RESOLUTION", defVal);
            //sscanf_s(iniRes, "%dx%d", &iniResX, &iniResY);
            int32_t i = 0, mode = 0;
            for (auto& res : list)
            {
                SendMessageA(*((HWND*)(regs.esi + nHwndOffset)), 0x143u, 0, (LPARAM)(res.c_str()));

                if (res == iniRes)
                    mode = i;

                ++i;
            }
            SendMessageA(*((HWND*)(regs.esi + nHwndOffset)), 0x14Eu, mode, 0);
            *dwordOptionsMode = mode;
        }
    }; injector::MakeInline<ResListHook>(pattern.get_first(0), pattern.get_first(resListHookEnd)); //0x40600D, 0x40608B

    pattern = hook::pattern("68 ? ? ? ? 50 68 ? ? ? ? 68 ? ? ? ? FF D6 8D 4C 24 0C 51");
    struct MatrixOptionsHook
    {
        void operator()(injector::reg_pack& regs)
        {
            char iniPath[MAX_PATH];
            GetModuleFileNameA(NULL, iniPath, MAX_PATH);
            *strrchr(iniPath, '\\') = '\0';
            strcat_s(iniPath, "\\MatrixConfig.ini");
            CIniReader iniReader(iniPath);
            iniReader.WriteString("RENDERING", "RESOLUTION", (char*)list[*dwordOptionsMode].c_str());
        }
    }; injector::MakeInline<MatrixOptionsHook>(pattern.get_first(0), pattern.get_first(18)); //0x406B9F, 0x406BB1
}