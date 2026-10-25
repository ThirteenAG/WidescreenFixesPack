#include "stdafx.h"
#include "../Shared/PatchPreflight.hpp"
#include "FixSettings.hpp"
#include "NativeMenu.hpp"
#include "WindowMode.hpp"
#include <shlobj.h>
#include <filesystem>
#include <FunctionHookMinHook.hpp>

import Xidi;

struct INI
{
    bool FixAspectRatio;
    bool FixMenu;
    bool FixLowFramerate;
    bool FixSavePath;
    int WindowMode;
    bool SkipIntro;
} IniFile;

std::unique_ptr<FunctionHookMinHook> mhSHGetFolderPathA = { nullptr };
HRESULT WINAPI SHGetFolderPathAHook(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath)
{
    HRESULT r;
    if ((csidl & ~CSIDL_FLAG_CREATE) == CSIDL_COMMON_DOCUMENTS)
    {
        r = mhSHGetFolderPathA->get_original<decltype(SHGetFolderPathA)>()(hwnd, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, hToken, dwFlags, pszPath);

        if (FAILED(r))
            return r;

        static bool once = false;
        if (!once)
        {
            std::error_code ec;
            auto dest = std::filesystem::path(std::string(pszPath) + "\\Monolith Productions\\Condemned\\");
            if (!std::filesystem::exists(dest, ec))
            {
                CHAR szPath[MAX_PATH];
                if (SUCCEEDED(mhSHGetFolderPathA->get_original<decltype(SHGetFolderPathA)>()(NULL, CSIDL_COMMON_DOCUMENTS | CSIDL_FLAG_CREATE, NULL, 0, szPath)))
                {
                    auto src = std::filesystem::path(std::string(szPath) + "\\Monolith Productions\\Condemned\\");
                    std::filesystem::create_directories(dest, ec);
                    std::filesystem::copy(src, dest, std::filesystem::copy_options::recursive, ec);
                }
            }
            once = true;
        }
    }
    else
    {
        r = mhSHGetFolderPathA->get_original<decltype(SHGetFolderPathA)>()(hwnd, csidl, hToken, dwFlags, pszPath);
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

[[noreturn]] void WINAPI ExitCondemned(UINT code)
{
    // The game has already closed its engine when the CRT requests process exit.
    // Late game-DLL destructors still reference that engine. Keep the existing
    // forced-exit workaround at this boundary, before DLL detach begins.
    TerminateProcess(GetCurrentProcess(), code);
    ExitProcess(code);
}

void InstallProcessExit()
{
    auto crt = GetModuleHandleW(L"msvcr71.dll");
    HMODULE own = nullptr;
    if (!crt || !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&ExitCondemned), &own)) return;
    const auto originals = IATHook::Replace(crt, "KERNEL32.dll",
        std::pair{ "ExitProcess", reinterpret_cast<void*>(&ExitCondemned) });
    if (!originals.contains("ExitProcess"))
        OutputDebugStringA("Condemned Widescreen Fix: process exit workaround unavailable.\n");
}

uint32_t gc_16C900;
uint32_t gc_16DD00;
void Init()
{
    hook::pattern pattern;
    InstallProcessExit();
    if (IniFile.FixLowFramerate)
    {
        if (wfp::PatchPreflight("Condemned controller enumeration: signatures unavailable; feature disabled.\n",
        {
            { "6A 01 57 FF 51 10 85 C0", 1 },
        }))
        {
            //DI8DEVCLASS_DEVICE (tanks framerate)
            static constexpr uint8_t DI8DEVCLASS_GAMECTRL = 4;
            pattern = hook::pattern("6A 01 57 FF 51 10 85 C0");
            ProtectedGameRef<uint8_t>{pattern.get_first<uint8_t>(1)} = DI8DEVCLASS_GAMECTRL;
        }


    }

    if (IniFile.FixAspectRatio)
    {
        if (wfp::PatchPreflight("Condemned aspect ratio: signatures unavailable; feature disabled.\n",
        {
            { "E8 ? ? ? ? 8B C6 5E 83 C4 10 C3", 1 },
            { "8B 44 24 08 50 E8 ? ? ? ? C3", 1 },
        }))
        {
            pattern = hook::pattern("E8 ? ? ? ? 8B C6 5E 83 C4 10 C3");
            injector::MakeCALL(pattern.get_first(0), sub_4059F0, true);
            pattern = hook::pattern("8B 44 24 08 50 E8 ? ? ? ? C3");
            injector::MakeCALL(pattern.get_first(5), sub_4059F0, true);
        }


    }

    if (IniFile.FixMenu)
    {
        if (wfp::PatchPreflight("Condemned menu backgrounds: signatures unavailable; feature disabled.\n",
        {
            { "8B 0D ? ? ? ? 85 C9 74 05 E9 ? ? ? ? C3", 4 },
            { "68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 10 C3", 41 },
            { "8B 0D ? ? ? ? 85 C9 74 05 E8 ? ? ? ? 8B 0D ? ? ? ? 85 C9 74 05 E8 ? ? ? ? 8B 0D ? ? ? ? 85 C9", 1 },
            { "56 8B F1 8D 46 0C 68 ? ? ? ? 50", 1 },
            { "8B C3 D9 5C 24 24 F3 A5 5F 5E 83 C4 38 C3", 1 },
        }))
        {
            static auto exe_567CC4 = *hook::pattern("8B 0D ? ? ? ? 85 C9 74 05 E9 ? ? ? ? C3").count_hint(4).get(1).get<uint32_t>(2);
            static auto exe_567C78 = *hook::pattern("68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 10 C3").count_hint(41).get(40).get<uint32_t>(1);
            static auto exe_567CF8 = *hook::get_pattern<uint32_t>("8B 0D ? ? ? ? 85 C9 74 05 E8 ? ? ? ? 8B 0D ? ? ? ? 85 C9 74 05 E8 ? ? ? ? 8B 0D ? ? ? ? 85 C9", 2) + 0x28;
            static auto exe_5701F8 = *hook::get_pattern<uint32_t>("56 8B F1 8D 46 0C 68 ? ? ? ? 50", 7);

            pattern = hook::pattern("8B C3 D9 5C 24 24 F3 A5 5F 5E 83 C4 38 C3");
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

                    uint32_t arr[10]{};
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


    }

    CondemnedWindow::Install(IniFile.WindowMode);

    // Controller Support
    if (wfp::PatchPreflight("Condemned controller cursor: signatures unavailable; feature disabled.\n",
    {
        { "83 E8 00 56 74 ? 48 75 ? 8B 35 ? ? ? ? 6A 01 FF D6 85 C0 7C ? 33 C0 5E C3", 1 },
        { "E8 ? ? ? ? 89 7E 04 5F 33 C0", 1 },
    }))
    {
        pattern = hook::pattern("83 E8 00 56 74 ? 48 75 ? 8B 35 ? ? ? ? 6A 01 FF D6 85 C0 7C ? 33 C0 5E C3");
        static auto CursorHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            bShowingCursor = regs.eax != 0;
            UpdateXidi();
        });

        pattern = hook::pattern("E8 ? ? ? ? 89 7E 04 5F 33 C0");
        static auto CursorHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            bShowingCursor = regs.edi != 0;
            UpdateXidi();
        });
    }


}

// Player camera FOV: vertical FOV in radians in, horizontal and vertical out.
float* (__thiscall* ComputeCameraFovOriginal)(void* camera, float* out, float fovY, float aspectScale) = nullptr;
float* __fastcall ComputeCameraFov(void* camera, void* edx, float* out, float fovY, float aspectScale)
{
    static const auto& factor = FixSettings().Float("MAIN.FOVFactor");
    if (std::isfinite(factor) && factor != 1.0f && std::isfinite(fovY) && fovY > 0.0f)
        fovY = std::min(fovY * std::clamp(factor, 0.5f, 2.0f), 2.8f);
    return ComputeCameraFovOriginal(camera, out, fovY, aspectScale);
}

void InitGameClient()
{
    hook::pattern pattern;
    auto gameClient = GetModuleHandle(L"GameClient");
    CondemnedMenuNative::Init(gameClient);

    if (IniFile.SkipIntro)
    {
        // The splash movie player's "skip all" argument, as when the player skips them.
        auto pattern = hook::pattern(gameClient, "51 55 56 8B F1 8B 0D ? ? ? ? 8B 01 FF 90 84 01 00 00 8B E8 85 ED 0F 84 ? ? ? ? F6 86 ? ? ? ? 04");
        if (!pattern.empty())
        {
            static auto SplashMoviesHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                *(uint8_t*)(regs.esp + 4) = 1;
            });
        }
    }

    // Gameplay camera FOV: the call sites that pass the FovY console variable; scripted cameras and scopes keep theirs.
    {
        std::vector<void*> callSites;
        auto collect = [&](hook::pattern pattern, ptrdiff_t offset)
        {
            pattern.for_each_result([&](hook::pattern_match match) { callSites.push_back(match.get<void>(offset)); });
        };
        collect(hook::pattern(gameClient, "51 8B 0D ? ? ? ? D8 0D ? ? ? ? 8D 54 24 ? D9 1C 24 52 E8"), 21);
        collect(hook::pattern(gameClient, "D8 0D ? ? ? ? 8D 4C 24 10 D9 1C 24 51 8B 0D ? ? ? ? E8"), 20);
        collect(hook::pattern(gameClient, "8B 0D ? ? ? ? D8 0D ? ? ? ? 8D 44 24 24 D9 1C 24 50 E8"), 20);
        if (!callSites.empty())
        {
            auto target = injector::GetBranchDestination(callSites.front()).as_int();
            bool sameTarget = std::all_of(callSites.begin(), callSites.end(), [&](void* site) { return injector::GetBranchDestination(site).as_int() == target; });
            if (sameTarget)
            {
                ComputeCameraFovOriginal = reinterpret_cast<decltype(ComputeCameraFovOriginal)>(target);
                for (auto site : callSites)
                    injector::MakeCALL(site, ComputeCameraFov, true);
            }
        }
    }

    if (IniFile.FixMenu)
    {
        if (wfp::PatchPreflight(gameClient, "Condemned menu identity: signatures unavailable; feature disabled.\n",
        {
            { "C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? 6A 04", 1 },
        }))
        {
            auto unk_10169F30 = *hook::module_pattern(GetModuleHandle(L"GameClient"), "C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? 6A 04").get_first<void**>(6);
            gc_16C900 = (uint32_t)(unk_10169F30 + 0xA74);
            gc_16DD00 = (uint32_t)(unk_10169F30 + 0xF74);
        }


    }

    if (IniFile.FixAspectRatio)
    {
        if (wfp::PatchPreflight(gameClient, "Condemned HUD and key assignment: signatures unavailable; feature disabled.\n",
        {
            { "C7 04 24 ? ? ? ? 74 1E 8B 15 ? ? ? ? 8B 01 52", 1 },
            { "57 50 E8 ? ? ? ? 8B F8 68", 1 },
            { "E8 ? ? ? ? 8B 78 0C 8B AC 24 E4 0A 00 00", 1 },
        }))
        {
            //Hud
            pattern = hook::pattern(GetModuleHandle(L"GameClient"), "C7 04 24 ? ? ? ? 74 1E 8B 15 ? ? ? ? 8B 01 52");
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

            pattern = hook::pattern(GetModuleHandle(L"GameClient"), "57 50 E8 ? ? ? ? 8B F8 68");
            static auto KeyAssignHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                bKeyAssignmentInProgress = true;
            });

            pattern = hook::pattern(GetModuleHandle(L"GameClient"), "E8 ? ? ? ? 8B 78 0C 8B AC 24 E4 0A 00 00");
            static auto KeyAssignHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                bKeyAssignmentInProgress = false;
            });
        }


    }
}

void InitConfig()
{
    hook::pattern pattern;
    //unlocking all resolutions
    if (IniFile.FixAspectRatio)
    {
        if (wfp::PatchPreflight("Condemned configuration modes: signatures unavailable; feature disabled.\n",
        {
            { "0F 85 ? ? ? ? 83 7C 24 2C 16 0F 85 ? ? ? ? 6A 7F", 1 },
        }))
        {
            pattern = hook::pattern("0F 85 ? ? ? ? 83 7C 24 2C 16 0F 85 ? ? ? ? 6A 7F");
            injector::MakeNOP(pattern.get_first(0), 17);
        }


    }
}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        WindowedModeWrapper::EnableDpiAwareness();
        auto& settings = FixSettings();
        asi_settings::LiveLink::Start();
        IniFile.FixAspectRatio = settings.Bool("MAIN.FixAspectRatio");
        IniFile.FixMenu = settings.Bool("MAIN.FixMenu");
        IniFile.FixLowFramerate = settings.Bool("MAIN.FixLowFramerate");
        IniFile.FixSavePath = settings.Bool("MAIN.FixSavePath");
        IniFile.WindowMode = settings.Int("MAIN.WindowMode");
        IniFile.SkipIntro = settings.Bool("MAIN.SkipIntro");

        auto game = hook::pattern("E8 ? ? ? ? 8B C6 5E 83 C4 10 C3");
        auto config = hook::pattern("0F 85 ? ? ? ? 83 7C 24 2C 16 0F 85 ? ? ? ? 6A 7F");
        const bool isGame = !game.empty();
        if (isGame || !config.empty())
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(isGame ? Init : InitConfig, isGame ? game : config);
        else
            OutputDebugStringA("Condemned Widescreen Fix: unsupported executable.\n");
        CallbackHandler::RegisterModuleLoadCallback(L"GameClient.dll", InitGameClient);
        CallbackHandler::RegisterModuleLoadCallback(L"Xidi.32.dll", InitXidi);

        if (IniFile.FixSavePath)
        {
            mhSHGetFolderPathA = std::make_unique<FunctionHookMinHook>((uintptr_t)SHGetFolderPathA, (uintptr_t)SHGetFolderPathAHook);
            mhSHGetFolderPathA->create();
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
