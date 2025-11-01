#include "stdafx.h"
#include <shlobj.h>
#include <filesystem>

struct INI
{
    bool FixAspectRatio;
    bool FixMenu;
    bool FixLowFramerate;
    bool FixSavePath;
    bool BorderlessWindowed;
} IniFile;

SafetyHookInline shSHGetFolderPathA = {};
HRESULT WINAPI SHGetFolderPathAHook(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath)
{
    auto r = shSHGetFolderPathA.unsafe_stdcall<HRESULT>(hwnd, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, hToken, dwFlags, pszPath);

    static bool once = false;
    if (!once)
    {
        std::error_code ec;
        auto dest = std::filesystem::path(std::string(pszPath) + "\\Monolith Productions\\Condemned\\");
        if (!std::filesystem::exists(dest, ec))
        {
            CHAR szPath[MAX_PATH];
            if (SUCCEEDED(shSHGetFolderPathA.unsafe_stdcall<HRESULT>(NULL, CSIDL_COMMON_DOCUMENTS | CSIDL_FLAG_CREATE, NULL, 0, szPath)))
            {
                auto src = std::filesystem::path(std::string(szPath) + "\\Monolith Productions\\Condemned\\");
                std::filesystem::create_directories(dest, ec);
                std::filesystem::copy(src, dest, std::filesystem::copy_options::recursive, ec);
            }
        }
        once = true;
    }

    return r;
}

void __fastcall sub_4059F0(float* _this, uint32_t edx, float* a2)
{
    _this[59] = 0.0f;
    _this[60] = 0.0f;
    _this[61] = (a2[2] < 0.0f) ? 0.0f : 1.0f;
    _this[62] = (a2[3] < 0.0f) ? 0.0f : 1.0f;

    _this[59] = (_this[59] < _this[61]) ? _this[59] : _this[61];
    _this[60] = (_this[60] < _this[62]) ? _this[60] : _this[62];
    _this[61] = (_this[61] < _this[59]) ? _this[59] : _this[61];
    _this[62] = (_this[62] < _this[60]) ? _this[60] : _this[62];
}

uint32_t gc_16C900;
uint32_t gc_16DD00;
void Init()
{
    if (IniFile.FixLowFramerate)
    {
        //DI8DEVCLASS_DEVICE (tanks framerate)
        static constexpr uint8_t DI8DEVCLASS_GAMECTRL = 4;
        auto pattern = hook::pattern("6A 01 57 FF 51 10 85 C0");
        injector::WriteMemory<uint8_t>(pattern.get_first(1), DI8DEVCLASS_GAMECTRL, true);
    }

    if (IniFile.FixAspectRatio)
    {
        auto pattern = hook::pattern("E8 ? ? ? ? 8B C6 5E 83 C4 10 C3");
        injector::MakeCALL(pattern.get_first(0), sub_4059F0, true);
        pattern = hook::pattern("8B 44 24 08 50 E8 ? ? ? ? C3");
        injector::MakeCALL(pattern.get_first(5), sub_4059F0, true);
    }

    if (IniFile.FixMenu)
    {
        static auto exe_567CC4 = *hook::pattern("8B 0D ? ? ? ? 85 C9 74 05 E9 ? ? ? ? C3").count(4).get(1).get<uint32_t>(2);
        static auto exe_567C78 = *hook::pattern("68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 10 C3").count(41).get(40).get<uint32_t>(1);
        static auto exe_567CF8 = *hook::get_pattern<uint32_t>("8B 0D ? ? ? ? 85 C9 74 05 E8 ? ? ? ? 8B 0D ? ? ? ? 85 C9 74 05 E8 ? ? ? ? 8B 0D ? ? ? ? 85 C9", 2) + 0x28;
        static auto exe_5701F8 = *hook::get_pattern<uint32_t>("56 8B F1 8D 46 0C 68 ? ? ? ? 50", 7);

        auto pattern = hook::pattern("8B C3 D9 5C 24 24 F3 A5 5F 5E 83 C4 38 C3");
        struct HudHook
        {
            void operator()(injector::reg_pack& regs)
            {
                float f = 0.0f;
                regs.eax = regs.ebx;
                _asm {fstp dword ptr[f]}
                *(float*)(regs.esp + 0x24) = f;

                auto w = *(uint32_t*)(regs.esp + 0x08);
                auto h = *(uint32_t*)(regs.esp + 0x0C);

                uint32_t arr[10];
                for (size_t i = regs.esp, j = 0; j < 9; i += 4)
                {
                    if ((*(size_t*)i > (size_t)GetModuleHandle(NULL) && *(size_t*)i < (size_t)GetModuleHandle(NULL) + 0x176004) || (*(size_t*)i > (size_t)GetModuleHandle(L"GameClient") && *(size_t*)i < (size_t)GetModuleHandle(L"GameClient") + 0x172AD8))
                    {
                        arr[j] = *(size_t*)i;
                        ++j;
                    }

                    if (i > regs.esp + 0x100)
                        break;
                }

                if ((arr[0] == exe_567CC4 && arr[1] == exe_5701F8 && arr[6] == arr[7] && arr[7] == arr[8]) ||
                    (arr[0] == exe_567CC4 && arr[1] == exe_5701F8 && arr[6] == arr[7] && ((arr[7] == gc_16C900 || (arr[7] == gc_16DD00)))))
                {
                    *(float*)(regs.esp + 0x24) *= (((float)w / (float)h) / (4.0f / 3.0f)); //y fix
                    *(float*)(regs.esp + 0x2C) *= (((float)w / (float)h) / (4.0f / 3.0f)); //y fix
                }
                else
                {
                    if (arr[0] == exe_567CC4 && arr[1] == exe_5701F8 && arr[6] == exe_567CF8 && arr[7] == exe_567C78)
                    {
                        if (*(uint32_t*)(regs.esp + 0xC4) == 3 && *(uint32_t*)(regs.esp + 0x114) > 0 && *(uint32_t*)(regs.esp + 0x114) <= 10)
                        {
                            *(float*)(regs.esp + 0x24) *= (((float)w / (float)h) / (4.0f / 3.0f)); //y fix
                            *(float*)(regs.esp + 0x2C) *= (((float)w / (float)h) / (4.0f / 3.0f)); //y fix
                        }
                    }
                }
            }
        }; injector::MakeInline<HudHook>(pattern.get_first(0), pattern.get_first(6)); //0x504C89
    }

    if (IniFile.BorderlessWindowed)
    {
        auto pattern = hook::pattern("68 ? ? ? ? 8D 4C 24 0C 03 C7 51");
        injector::WriteMemory(pattern.get_first(1), WS_POPUP & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU), true);
        pattern = hook::pattern("68 ? ? ? ? 52 68 ? ? ? ? 53");
        injector::WriteMemory(pattern.get_first(1), WS_POPUP & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU), true);
    }
}

void InitGameClient()
{
    if (IniFile.FixMenu)
    {
        auto unk_10169F30 = *hook::module_pattern(GetModuleHandle(L"GameClient"), "C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? 6A 04").get_first<void**>(6);
        gc_16C900 = (uint32_t)(unk_10169F30 + 0xA74);
        gc_16DD00 = (uint32_t)(unk_10169F30 + 0xF74);
    }

    if (IniFile.FixAspectRatio)
    {
        //Hud
        auto pattern = hook::pattern(GetModuleHandle(L"GameClient"), "C7 04 24 ? ? ? ? 74 1E 8B 15 ? ? ? ? 8B 01 52");
        struct HudPosHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x00) = (float)regs.eax / (float)regs.edx;
            }
        }; injector::MakeInline<HudPosHook>(pattern.get_first(0), pattern.get_first(7));

        //pattern = hook::pattern(GetModuleHandle(L"GameClient"), "D8 0D ? ? ? ? 8B 8C 24 ? ? ? ? 85 C9");
        //struct HudHook2
        //{
        //    void operator()(injector::reg_pack& regs)
        //    {
        //        auto ret = *(uint32_t*)(regs.esp + 0x9C);
        //        auto x = *(int32_t*)(regs.esp + 0x90 - 4);
        //        auto y = *(int32_t*)(regs.esp + 0x94 - 4);
        //
        //        x = (float)x / ((16.0f / 9.0f) / (4.0f / 3.0f));
        //
        //        //auto v5 = (double)*(int32_t*)(regs.esp + 0x94);
        //        //if (*(int32_t*)(regs.esp + 0x90))
        //        //    v5 = v5 + 4294967300.0;
        //
        //        *(float*)(regs.esi + 0x10) = (double)x * 0.0015625f;
        //        *(float*)(regs.esi + 0x14) = (double)y * 0.0020833334f;
        //        *(int32_t*)(regs.esi + 0x18) = x;
        //        *(int32_t*)(regs.esi + 0x1C) = y;
        //
        //    }
        //}; //injector::MakeInline<HudHook2>(pattern.get_first(0), pattern.get_first(48));

        //pos
        //auto x = -1.0f / ((16.0f / 9.0f) / (4.0f / 3.0f));
        //injector::WriteMemory<float>(0x504C28 + 4, x, true);
    }
}

void InitConfig()
{
    //unlocking all resolutions
    if (IniFile.FixAspectRatio)
    {
        auto pattern = hook::pattern("0F 85 ? ? ? ? 83 7C 24 2C 16 0F 85 ? ? ? ? 6A 7F");
        injector::MakeNOP(pattern.get_first(0), 17);
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CIniReader iniReader("");
        IniFile.FixAspectRatio = iniReader.ReadInteger("MAIN", "FixAspectRatio", 1) != 0;
        IniFile.FixMenu = iniReader.ReadInteger("MAIN", "FixMenu", 1) != 0;
        IniFile.FixLowFramerate = iniReader.ReadInteger("MAIN", "FixLowFramerate", 1) != 0;
        IniFile.FixSavePath = iniReader.ReadInteger("MAIN", "FixSavePath", 1) != 0;
        IniFile.BorderlessWindowed = iniReader.ReadInteger("MAIN", "BorderlessWindowed", 1) != 0;

        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("E8 ? ? ? ? 8B C6 5E 83 C4 10 C3"));
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitConfig, hook::pattern("0F 85 ? ? ? ? 83 7C 24 2C 16 0F 85 ? ? ? ? 6A 7F"));
        CallbackHandler::RegisterCallback(L"GameClient.dll", InitGameClient);

        if (IniFile.FixSavePath)
        {
            shSHGetFolderPathA = safetyhook::create_inline(SHGetFolderPathA, SHGetFolderPathAHook);
        }
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