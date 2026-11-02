module;

#include <stdafx.h>
#include <d3d9.h>
#include <format>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Misc;

import ComVars;
import Frontend;
import WindowMode;
import Camera;
import Cinematics;
import Movies;
import Hud;
import Renderer;
import Xidi;

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
    RWGTA::UpdateCheck::OnExit();
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
        OutputDebugStringA("Condemned Fusion Fix: process exit workaround unavailable.\n");
}

void InstallInit()
{
    hook::pattern pattern;
    InstallProcessExit();
    //DI8DEVCLASS_DEVICE (tanks framerate)
    static constexpr uint8_t DI8DEVCLASS_GAMECTRL = 4;
    pattern = hook::pattern("6A 01 57 FF 51 10 85 C0");
    injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(1), DI8DEVCLASS_GAMECTRL, true);

    pattern = hook::pattern("E8 ? ? ? ? 8B C6 5E 83 C4 10 C3");
    injector::MakeCALL(pattern.get_first(0), sub_4059F0, true);
    pattern = hook::pattern("8B 44 24 08 50 E8 ? ? ? ? C3");
    injector::MakeCALL(pattern.get_first(5), sub_4059F0, true);

    static auto exe_567CC4 = *hook::pattern("8B 0D ? ? ? ? 85 C9 74 05 E8 ? ? ? ? 8B 0D ? ? ? ? 85 C9 74 05 E8 ? ? ? ? 8B 0D ? ? ? ? 85 C9 74 05 E9 ? ? ? ? C3").get_first<uint32_t>(32);
    static auto exe_567C78 = []()
    {
        const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const auto pe = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        const auto sections = IMAGE_FIRST_SECTION(pe);
        const auto data = std::find_if(sections, sections + pe->FileHeader.NumberOfSections,
            [](const IMAGE_SECTION_HEADER& section) { return std::memcmp(section.Name, ".rdata", 6) == 0; });
        const auto start = base + data->VirtualAddress;
        const auto name = reinterpret_cast<uintptr_t>(hook::range_pattern(start, start + data->Misc.VirtualSize,
            "49 4C 54 44 72 61 77 50 72 69 6D 2E 44 65 66 61 75 6C 74 00").get_first());
        const auto registration = std::format("6A 00 68 {:02X} {:02X} {:02X} {:02X} 68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 10 C3",
            name & 0xFF, (name >> 8) & 0xFF, (name >> 16) & 0xFF, (name >> 24) & 0xFF);
        return *hook::pattern(registration).get_first<uint32_t>(8);
    }();
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

    CondemnedWindow::Install(IniFile.WindowMode);
    CondemnedRenderer::Init(IniFile.WindowMode);

    // Controller Support
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

// Player camera FOV: vertical FOV in radians in, horizontal and vertical out.
float* (__thiscall* ComputeCameraFovOriginal)(void* camera, float* out, float fovY, float aspectScale) = nullptr;
float* __fastcall ComputeCameraFov(void* camera, void* edx, float* out, float fovY, float aspectScale)
{
    static const auto& factor = FixSettings().Float("MAIN.FOVFactor");
    const auto viewport = reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(camera) + 0x18);
    fovY = CondemnedCamera::GameplayFov(fovY, factor, viewport[0], viewport[1], aspectScale);
    return ComputeCameraFovOriginal(camera, out, fovY, aspectScale);
}

void InstallGameClient()
{
    hook::pattern pattern;
    auto gameClient = GetModuleHandle(L"GameClient");
    CondemnedMenuNative::Init(gameClient);
    if (IniFile.WindowMode)
        CondemnedWindow::InstallClient(gameClient);
    CondemnedCinematics::Init(gameClient);
    CondemnedMovies::Init(gameClient);

    if (IniFile.SkipIntro)
    {
        // The splash movie player's "skip all" argument, as when the player skips them.
        auto pattern = hook::pattern(gameClient, "51 55 56 8B F1 8B 0D ? ? ? ? 8B 01 FF 90 84 01 00 00 8B E8 85 ED 0F 84 ? ? ? ? F6 86 ? ? ? ? 04");
        static auto SplashMoviesHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(uint8_t*)(regs.esp + 4) = 1;
        });
    }

    // Gameplay camera FOV: the call sites that pass the FovY console variable; scripted cameras and scopes keep theirs.
    {
        std::vector<void*> callSites;
        auto collect = [&](hook::pattern pattern, ptrdiff_t offset)
        {
            pattern.get_first();
            pattern.for_each_result([&](hook::pattern_match match) { callSites.push_back(match.get<void>(offset)); });
        };
        collect(hook::pattern(gameClient, "51 8B 0D ? ? ? ? D8 0D ? ? ? ? 8D 54 24 ? D9 1C 24 52 E8"), 21);
        collect(hook::pattern(gameClient, "D8 0D ? ? ? ? 8D 4C 24 10 D9 1C 24 51 8B 0D ? ? ? ? E8"), 20);
        ComputeCameraFovOriginal = reinterpret_cast<decltype(ComputeCameraFovOriginal)>(injector::GetBranchDestination(callSites.at(0)).as_int());
        for (auto site : callSites)
            injector::MakeCALL(site, ComputeCameraFov, true);
    }

    auto unk_10169F30 = *hook::module_pattern(GetModuleHandle(L"GameClient"), "C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? 6A 04").get_first<void**>(6);
    gc_16C900 = (uint32_t)(unk_10169F30 + 0xA74);
    gc_16DD00 = (uint32_t)(unk_10169F30 + 0xF74);

    // Interface rectangle (HUD): the whole screen or a centred box instead of the 16:9 design box
    CondemnedHud::Init(gameClient);

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

export void InitConfig()
{
    hook::pattern pattern;
    //unlocking all resolutions
    pattern = hook::pattern("0F 85 ? ? ? ? 83 7C 24 2C 16 0F 85 ? ? ? ? 6A 7F");
    injector::MakeNOP(pattern.get_first(0), 17);
}


class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []() { InstallInit(); };
        WFP::onGameInitEvent() += []() { InstallGameClient(); };
    }
} Misc;
