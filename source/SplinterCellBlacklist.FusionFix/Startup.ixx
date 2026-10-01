module;

#include <stdafx.h>

export module Startup;

import ComVars;

export void InitStartup()
{
    CIniReader iniReader("");
    static auto bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    static auto bSkipPressAnyKeyScreen = iniReader.ReadInteger("MAIN", "SkipPressAnyKeyScreen", 1) != 0;

    // skip systemdetection
    auto pattern = hook::pattern("0F 84 ? ? ? ? 68 ? ? ? ? 89 B5");
    injector::WriteMemory<uint16_t>(pattern.get_first(), 0xE990, true); // jz -> jmp

    // UnrealMain
    pattern = hook::pattern("0F 84 ? ? ? ? 56 56 56 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 18 50 8D 8D");
    injector::WriteMemory<uint16_t>(pattern.get_first(), 0xE990, true); // jz -> jmp

    // NetOnlineManager::NetOnlineManager
    pattern = hook::pattern("C7 45 ? ? ? ? ? 8D 64 24 00 53");
    injector::WriteMemory<uint32_t>(pattern.get_first(3), 4372, true);

    // InitBootVideos
    if (bSkipIntro)
    {
        auto pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 8B E5");
        injector::MakeNOP(pattern.get_first(5), 5, true);
    }

    // UI::SceneStart: "Press any key" is state 2 (+568h). A key press signs the player in (UWindowsViewport input, sub_9316D0: the profile
    // manager's user +14h is -1, sub_739D10 and sub_675150(controller)), then PressStartHandler (bound to the PressStart element) sets the
    // started flag and goes to state 3, the scene goes on to the profile and the main menu from there.
    // With a checkpoint to go back to, it asks to resume at it or return to the Paladin (sub_19949E0, the message box result is handled by
    // sub_197FD00: resume is sub_197F790). Both are done the first time the scene gets to them.
    if (bSkipPressAnyKeyScreen)
    {
        pattern = find_pattern("B0 01 A2 ? ? ? ? 83 B9 68 05 00 00 02 75 0A C7 81 68 05 00 00 03 00 00 00 C2 08 00", "83 B9 68 05 00 00 02 B0 01 A2 ? ? ? ? 75 0A C7 81 68 05 00 00 03 00 00 00 C2 08 00");
        static auto PressStartHandler = reinterpret_cast<char(__fastcall*)(uintptr_t scene, void* edx, int, int)>(pattern.get_first());

        pattern = hook::pattern("E8 ? ? ? ? 8B 10 8B C8 8B 82 F0 01 00 00 FF D0 8B F0 E8");
        static auto GetEngine = reinterpret_cast<uintptr_t(__cdecl*)()>(injector::GetBranchDestination(pattern.get_first()).as_int());

        pattern = hook::pattern("83 7E 14 FF 75 11 8B CE E8 ? ? ? ? 6A 00 53 8B CE E8");
        static auto PrepareSignIn = reinterpret_cast<void(__fastcall*)(uintptr_t, void*)>(injector::GetBranchDestination(pattern.get_first(8)).as_int());
        static auto SignIn = reinterpret_cast<char(__fastcall*)(uintptr_t, void*, int, char)>(injector::GetBranchDestination(pattern.get_first(18)).as_int());

        // the input sign in waits for the engine to be ready (+2ECh & 2) and the tick counter to be past 60
        pattern = hook::pattern("F6 80 EC 02 00 00 02 74 ? 83 3D ? ? ? ? 00 7C ? 7F ? 83 3D ? ? ? ? 3C");
        static auto TickCounter = *pattern.get_first<int64_t*>(22);

        // UI::SceneStart sets a flag that makes the next Escape key release quit the game (sub_19AF330), the first key release clears it.
        // With no key pressed it would stay set until Escape is pressed in the game.
        pattern = hook::pattern("A3 ? ? ? ? 38 1D ? ? ? ? 0F 84");
        static auto pEscapeQuits = *pattern.get_first<uint8_t*>(7);

        // UI::SceneStart::SceneTick, after it waited for the movie to be ready, esi is the scene
        pattern = find_pattern("83 3D ? ? ? ? 00 56 8B F1 7E 0A FF 0D ? ? ? ? 5E C2 04 00 53 C6 86 66 05 00 00 01", "83 3D ? ? ? ? 00 56 8B F1 7E 0A FF 0D ? ? ? ? 5E C2 04 00 53 57 56 C6 86 66 05 00 00 01");
        static auto SceneStartTick = safetyhook::create_mid(pattern.get_first(22), [](SafetyHookContext& regs)
        {
            static bool pressed = false;
            if (pressed && *reinterpret_cast<int32_t*>(regs.esi + 0x568) != 2)
                *pEscapeQuits = 0; // not on a later "Press any key" (quit to title screen), Escape quits there as usual
            if (pressed || *reinterpret_cast<int32_t*>(regs.esi + 0x568) != 2)
                return;
            auto engine = GetEngine();
            if (!engine || (*reinterpret_cast<uint8_t*>(engine + 0x2EC) & 2) == 0 || *TickCounter <= 60)
                return;
            pressed = true;
            auto profileManager = engine ? reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t)>((*reinterpret_cast<uintptr_t**>(engine))[0x1F0 / 4])(engine) : 0;
            if (profileManager && *reinterpret_cast<int32_t*>(profileManager + 0x14) == -1)
            {
                PrepareSignIn(profileManager, nullptr);
                SignIn(profileManager, nullptr, 0, 0);
            }
            PressStartHandler(regs.esi, nullptr, 0, 0);
            *pEscapeQuits = 0;
        });

        // sub_19949E0: the resume message box is made after the state is set to 12, the jnz goes to the end of the function.
        // Resume (sub_197F790) is thiscall in the DX11 exe, stdcall in the DX9 one.
        static void(__fastcall* ResumeThiscall)(uintptr_t scene, void* edx) = nullptr;
        static void(__stdcall* ResumeStdcall)(uintptr_t scene) = nullptr;
        pattern = hook::pattern("38 9E 85 05 00 00 74 ? 8B CE E8");
        if (!pattern.empty())
            ResumeThiscall = reinterpret_cast<decltype(ResumeThiscall)>(injector::GetBranchDestination(pattern.get_first(10)).as_int());
        else
        {
            pattern = hook::pattern("38 9E 85 05 00 00 74 ? 56 E8");
            ResumeStdcall = reinterpret_cast<decltype(ResumeStdcall)>(injector::GetBranchDestination(pattern.get_first(9)).as_int());
        }

        pattern = hook::pattern("C7 86 68 05 00 00 0C 00 00 00 38 9E 5C 05 00 00 75 ? 3B FB");
        static auto ResumeMessageBoxEnd = reinterpret_cast<uintptr_t>(pattern.get_first(18)) + *pattern.get_first<int8_t>(17);
        auto messageBox = pattern.get_first<uint8_t>(20);
        messageBox += (*messageBox == 0x74) ? 2 : 6; // jz short / near
        static auto ResumeMessageBox = safetyhook::create_mid(messageBox, [](SafetyHookContext& regs)
        {
            static bool resumed = false;
            if (std::exchange(resumed, true))
                return;
            if (ResumeThiscall)
                ResumeThiscall(regs.esi, nullptr);
            else
                ResumeStdcall(regs.esi);
            regs.eip = ResumeMessageBoxEnd;
        });
    }
}
