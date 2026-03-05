module;

#include <stdafx.h>

export module BlacklistControls;

import ComVars;

export void OverrideForBlacklistControls(const char* path)
{
    static const std::string blacklistINI = "blacklist.ini";
    static const std::string LocalizationMenus = "Localization\\Menus";

    auto path_view = std::string_view(path);
    auto dest = const_cast<char*>(path);
    if (path_view == "ActionScheme.ini")
    {
        if (GetOverloadedFilePathA(blacklistINI.c_str(), nullptr, 0))
            strcpy_s(dest, blacklistINI.length() + 1, blacklistINI.c_str());
    }
    else if (path_view.contains(LocalizationMenus))
    {
        auto replace_view = [](std::string_view str, std::string_view pattern, std::string_view replacement) -> auto
        {
            return str | std::views::split(pattern) | std::views::join_with(replacement);
        };

        auto new_path = replace_view(path_view, LocalizationMenus, "Blacklist\\Menus") | std::ranges::to<std::string>();
        if (GetOverloadedFilePathA(new_path.c_str(), nullptr, 0))
            strcpy_s(dest, new_path.length() + 1, new_path.c_str());
    }
}

int WhichPrecisionModeNeeded = 0;
int CrouchSprint = 0;
int CrouchSprintOff = 0;
int Sprint = 0;
int SprintOff = 0;
bool bSprintNeeded = false;
bool bCrouchSprintNeeded = false;
int TogglePrecisionMode = 0;
int TogglePrecisionModeOn = 0;
int TogglePrecisionModeOff = 0;
static injector::hook_back<void(__fastcall*)(int*, void*, const char*, int)> hb458CD3;
void __fastcall fnTogglePrecisionMode(int* out, void* edx, const char* name, int a3)
{
    hb458CD3.fun(&Sprint, edx, "Sprint", a3);
    hb458CD3.fun(&SprintOff, edx, "SprintOff", a3);
    hb458CD3.fun(&CrouchSprint, edx, "CrouchSprint", a3);
    hb458CD3.fun(&CrouchSprintOff, edx, "CrouchSprintOff", a3);
    hb458CD3.fun(&TogglePrecisionModeOn, edx, "TogglePrecisionModeOn", a3);
    hb458CD3.fun(&TogglePrecisionModeOff, edx, "TogglePrecisionModeOff", a3);
    hb458CD3.fun(out, edx, name, a3);
    TogglePrecisionMode = *out;
}

int DetectB2W = 0;
void __fastcall fnDetectB2W(int* out, void* edx, const char* name, int a3)
{
    hb458CD3.fun(out, edx, name, a3);
    DetectB2W = *out;
}

int StopDetectB2W = 0;
void __fastcall fnStopDetectB2W(int* out, void* edx, const char* name, int a3)
{
    hb458CD3.fun(out, edx, name, a3);
    StopDetectB2W = *out;
}

BOOL __fastcall sub_56C0AD(uint32_t* _this, void* edx)
{
    auto aimcheck = (*(uint8_t*)((uint8_t*)_this + 0xA42));

    // run speed, disable run when player wants to aim
    if (WhichPrecisionModeNeeded == TogglePrecisionModeOn)
    {
        *(float*)&_this[753] = 200.0f;
    }
    else if (WhichPrecisionModeNeeded == TogglePrecisionModeOff)
    {
        *(float*)&_this[753] = 1000000.0f;
    }

    enum AimType
    {
        eNormal = 4,
        eFromCover = 5,
        eFirstPerson = 12,
    };

    if ((WhichPrecisionModeNeeded == TogglePrecisionModeOn) && (aimcheck == eNormal || aimcheck == eFromCover || aimcheck == eFirstPerson))
        return true;
    if ((WhichPrecisionModeNeeded == TogglePrecisionModeOff) && (aimcheck != eNormal && aimcheck != eFromCover && aimcheck != eFirstPerson))
        return true;

    return ((_this[655] & 0x100000) != 0) || ((_this[656] & 0x200000) != 0);
}

export void InitBlacklistControls()
{
    if (bBlacklistControlScheme)
    {
        // aiming check
        auto pattern = hook::pattern("E8 ? ? ? ? 8B 4D F8 48");
        injector::MakeCALL(pattern.get_first(), sub_56C0AD, true);

        // add modes for aiming
        pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 10 8B 00 3B 05 ? ? ? ? 75 04");
        hb458CD3.fun = injector::MakeCALL(pattern.get_first(), fnTogglePrecisionMode, true).get();

        // check aiming mode
        pattern = hook::pattern("3B 05 ? ? ? ? 75 07 8B CE E8 ? ? ? ? 5F 5E 5B");
        static auto loc_56DC64 = (uintptr_t)pattern.get_first(15);
        struct GetPrecisionMode
        {
            void operator()(injector::reg_pack& regs)
            {
                if (regs.eax == CrouchSprint)
                    bCrouchSprintNeeded = true;
                else if (regs.eax == CrouchSprintOff)
                    bCrouchSprintNeeded = false;

                if (regs.eax == Sprint)
                    bSprintNeeded = true;
                else if (regs.eax == SprintOff)
                    bSprintNeeded = false;

                if (regs.eax == TogglePrecisionMode || regs.eax == TogglePrecisionModeOn || regs.eax == TogglePrecisionModeOff)
                    WhichPrecisionModeNeeded = regs.eax;
                else
                    *(uintptr_t*)(regs.esp - 4) = loc_56DC64;
            }
        }; injector::MakeInline<GetPrecisionMode>(pattern.get_first(0), pattern.get_first(8));

        // add crouch sprint
        pattern = hook::pattern("F3 0F 10 45 ? F3 0F 11 45 ? F3 0F 10 86");
        static auto GetSpeedReferenceAdjustment = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (bCrouchSprintNeeded)
                *(float*)(regs.ebp + 0x0C) *= 1.5f;
            else if (bSprintNeeded)
                *(float*)(regs.ebp + 0x0C) *= 1.25f;
        });

        // cover modes
        pattern = hook::pattern("E8 ? ? ? ? F6 05 ? ? ? ? ? 75 17 83 0D ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? F6 05 ? ? ? ? ? 75 17 83 0D ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? F6 05 ? ? ? ? ? 75 17 83 0D ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? F6 05 ? ? ? ? ? 75 1A 81 0D ? ? ? ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? BF");
        hb458CD3.fun = injector::MakeCALL(pattern.get_first(0), fnDetectB2W, true).get();
        pattern = hook::pattern("E8 ? ? ? ? F6 05 ? ? ? ? ? 75 17 83 0D ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? F6 05 ? ? ? ? ? 75 17 83 0D ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? F6 05 ? ? ? ? ? 75 1A 81 0D ? ? ? ? ? ? ? ? 53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? BF");
        hb458CD3.fun = injector::MakeCALL(pattern.get_first(0), fnStopDetectB2W, true).get();

        // cover
        static bool bCoverStateStarted = false;
        static bool bReleased = false;
        static auto loc_56DBB3 = (uintptr_t)hook::get_pattern("3B 05 ? ? ? ? 75 13 8B 8E ? ? ? ? 8B 01");
        static auto lastDetectTime = std::chrono::steady_clock::now();

        pattern = hook::pattern("3B 05 ? ? ? ? 75 0B 09 BE");
        struct CoverHook
        {
            void operator()(injector::reg_pack& regs)
            {
                if (regs.eax == DetectB2W || regs.eax == StopDetectB2W)
                {
                    if (regs.eax == DetectB2W)
                    {
                        if (bCoverStateStarted)
                        {
                            *(uint32_t*)(regs.esi + 0xA44) &= ~0x100;
                        }
                        else
                        {
                            lastDetectTime = std::chrono::steady_clock::now();
                            *(uint32_t*)(regs.esi + 0xA44) |= 0x100;
                        }
                    }
                }
                else
                    *(uintptr_t*)(regs.esp - 4) = loc_56DBB3;
            }
        }; injector::MakeInline<CoverHook>(pattern.get_first(0), pattern.get_first(14));

        pattern = hook::pattern("81 88 ? ? ? ? ? ? ? ? 8B 4B 78");
        static auto UCoverNavStartState = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            bCoverStateStarted = true;
        });

        pattern = hook::pattern("0F 57 C0 53 55 56 57");
        static auto UCoverNavEndState = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            bCoverStateStarted = false;
        });

        pattern = hook::pattern("66 F7 86 ? ? ? ? ? ? 0F 84 ? ? ? ? 0F 57 C0");
        static auto UCoverNavStartStateCheck = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - lastDetectTime).count();

            // Wait for 1 second before applying StopDetectB2W code
            if (elapsed >= 1 && elapsed <= 2)
            {
                if (!bCoverStateStarted)
                    *(uint32_t*)(regs.esi + 0xA44) &= ~0x100;
            }
        });
    }
}