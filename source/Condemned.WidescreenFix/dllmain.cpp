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

HRESULT SHGetFolderPathAHook(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath)
{
    auto r = SHGetFolderPathA(hwnd, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, hToken, dwFlags, pszPath);

    CHAR szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_DOCUMENTS | CSIDL_FLAG_CREATE, NULL, 0, szPath)))
    {
        auto src = std::filesystem::path(std::string(szPath) + "\\Monolith Productions\\Condemned\\");
        auto dest = std::filesystem::path(std::string(pszPath) + "\\Monolith Productions\\Condemned\\");
        if (!std::filesystem::exists(dest))
        {
            std::filesystem::create_directories(dest);
            std::filesystem::copy(src, dest, std::filesystem::copy_options::recursive);
        }
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

                if ((*(float*)(regs.esp + 0x50) == 1.0f && *(uint32_t*)(regs.esp + 0x54) == 0 && *(uint32_t*)(regs.esp + 0x98) == 1) ||
                    (*(float*)(regs.esp + 0x2C) == 1.0f && *(uint32_t*)(regs.esp + 0x30) == 0 && *(uint32_t*)(regs.esp + 0xE4) == 1))
                {
                    if (*(uint32_t*)(regs.esp + 0xC4) >= 37570176 && (*(uint32_t*)(regs.esp + 0x114) == 0 || *(uint32_t*)(regs.esp + 0x114) == 2 ||
                        *(uint32_t*)(regs.esp + 0x110) == 0 || *(uint32_t*)(regs.esp + 0x110) == 2 || *(float*)(regs.esp + 0x110) == -0.5f ||
                        (*(float*)(regs.esp + 0x11C) == -0.5f && *(float*)(regs.esp + 0x50) == 1.0f)))
                    {
                        //*(float*)(regs.esp + 0x10) /= (((float)w / (float)h) / (4.0f / 3.0f)); //x fix
                        //*(float*)(regs.esp + 0x1C) /= (((float)w / (float)h) / (4.0f / 3.0f)); //x fix

                        *(float*)(regs.esp + 0x24) *= (((float)w / (float)h) / (4.0f / 3.0f)); //y fix
                        *(float*)(regs.esp + 0x2C) *= (((float)w / (float)h) / (4.0f / 3.0f)); //y fix
                    }
                    else if (*(uint32_t*)(regs.esp + 0xC4) == 3 && *(uint32_t*)(regs.esp + 0x114) > 0 && *(uint32_t*)(regs.esp + 0x114) <= 10)
                    {
                        *(float*)(regs.esp + 0x24) *= (((float)w / (float)h) / (4.0f / 3.0f)); //y fix
                        *(float*)(regs.esp + 0x2C) *= (((float)w / (float)h) / (4.0f / 3.0f)); //y fix
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

void InitSavePath()
{
    if (IniFile.FixSavePath)
    {
        static auto pF = &SHGetFolderPathAHook;
        auto pattern = hook::pattern("FF 15 ? ? ? ? 85 C0 7C 4F 56 57 BE ? ? ? ? 56");
        injector::WriteMemory(pattern.get_first(2), &pF, true);
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

            CallbackHandler::RegisterCallback(Init, hook::pattern("E8 ? ? ? ? 8B C6 5E 83 C4 10 C3"));
            CallbackHandler::RegisterCallback(InitConfig, hook::pattern("0F 85 ? ? ? ? 83 7C 24 2C 16 0F 85 ? ? ? ? 6A 7F"));
            CallbackHandler::RegisterCallback(InitSavePath, hook::pattern("FF 15 ? ? ? ? 85 C0 7C 4F 56 57 BE ? ? ? ? 56"));
            CallbackHandler::RegisterCallback(L"GameClient.dll", InitGameClient);
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