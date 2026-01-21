#include "stdafx.h"

import ComVars;
import Speedhack;
import Framelimit;
import RawInput;
import WidescreenHUD;

int32_t nLanguage;
int32_t __cdecl SetLanguage(LPCSTR lpValueName)
{
    return nLanguage;
}

SafetyHookInline shsub_654780 = {};
void __fastcall sub_654780(void* _this, void* edx, int a2)
{
    a2 = Screen.Width43;
    shsub_654780.unsafe_fastcall(_this, edx, a2);
}

void __cdecl sub_654D60(float* input1, float* input2, int alignmentMode, float* output)
{
    if (!input1 || !input2 || !output) return;

    // Scale factors
    output[5] = input1[5] * input2[5];  // Width scale
    output[6] = input1[6] * input2[6];  // Height scale
    output[3] = input2[3] * output[5];  // Scaled width component
    output[4] = input2[4] * output[6];  // Scaled height component
    output[0] = input1[0] * input2[0];  // Base scale

    float xPos;
    switch (alignmentMode)
    {
        case 0: case 3: case 6:  // Left/top alignment
            xPos = (input2[1] * output[5]) + input1[1];
            break;
        case 1: case 4: case 7:  // Center alignment
            xPos = ((input1[3] - output[3]) * 0.5f) + (input2[1] * output[5]) + input1[1];
            break;
        case 2: case 5: case 8:  // Right/bottom alignment
            xPos = (input1[3] + input1[1]) - output[3] + (input2[1] * output[5]);
            break;
        default:
            return;
    }

    output[1] = xPos + Screen.fHudOffset;  // Apply HUD offset

    switch (alignmentMode)
    {
        case 0: case 1: case 2:  // Top alignment
            output[2] = (input2[2] * output[6]) + input1[2];
            break;
        case 3: case 4: case 5:  // Middle alignment
            output[2] = ((input1[4] - output[4]) * 0.5f) + (input2[2] * output[6]) + input1[2];
            break;
        case 6: case 7: case 8:  // Bottom alignment
            output[2] = (input1[4] + input1[2]) - output[4] + (input2[2] * output[6]);
            break;
    }
}

BOOL WINAPI SetWindowPosHook(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
    return TRUE;
}

BOOL WINAPI AdjustWindowRectExHook(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle)
{
    return TRUE;
}

void Init()
{
    CIniReader iniReader("");
    bool bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    bool bDoNotUseRegistryPath = iniReader.ReadInteger("MAIN", "DoNotUseRegistryPath", 1) != 0;
    bool bBorderlessWindowed = iniReader.ReadInteger("MAIN", "BorderlessWindowed", 1) != 0;
    nLanguage = iniReader.ReadInteger("MAIN", "Language", -1);
    static bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
    Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(iniReader.ReadString("MAIN", "HudAspectRatioConstraint", ""));
    static bool bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) != 0;

    nFrameLimitType = iniReader.ReadInteger("FRAMELIMIT", "FrameLimitType", 1);
    fFpsLimit = std::clamp(static_cast<float>(iniReader.ReadInteger("FRAMELIMIT", "FpsLimit", 30)), 30.0f, FLT_MAX);
    fGameSpeedFactor = std::min(1.0f, 60.0f / fFpsLimit);

    static float fSensitivityFactor = std::abs(iniReader.ReadFloat("MOUSE", "SensitivityFactor", 0.0f));
    fRawInputMouse = std::abs(iniReader.ReadFloat("MOUSE", "RawInputMouse", 1.0f));

    if (bSkipIntro)
    {
        auto pattern = hook::pattern("6A 01 6A 01 68 ? ? ? ? E8 ? ? ? ? 6A 01 6A 01");
        injector::MakeJMP(pattern.get_first(0), hook::get_pattern("68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 08 E8 ? ? ? ? A1 ? ? ? ? 3B C3 75 23 6A 10"));
    }

    if (bDoNotUseRegistryPath)
    {
        auto pattern = hook::pattern("B9 20 00 00 00 8D 7C 24 18 F3 AB 8D 44 24 0C"); //0x496F27
        struct RegHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ecx = 0x20;
                regs.edi = (regs.esp + 0x18);

                GetModuleFileNameA(NULL, (char*)regs.edi, MAX_PATH);
                *strrchr((char*)regs.edi, '\\') = '\0';
                strcat_s((char*)regs.edi, MAX_PATH, "\\data");
            }
        }; injector::MakeInline<RegHook>(pattern.get_first(0), pattern.get_first(11));
        injector::MakeJMP(pattern.get_first(11), hook::pattern("8D 44 24 18 8D 50 01").count(2).get(1).get<uintptr_t>(0), true); //0x496FD8
    }

    if (bBorderlessWindowed)
    {
        auto pattern = hook::pattern("FF 15 ? ? ? ? 85 C0 A3 ? ? ? ? 75 ? FF 15");
        injector::MakeNOP(pattern.get_first(), 6, true);
        injector::MakeCALL(pattern.get_first(), WindowedModeWrapper::CreateWindowExA_Hook, true);

        pattern = hook::pattern("FF 15 ? ? ? ? 8B 0D ? ? ? ? 6A 05");
        injector::MakeNOP(pattern.get_first(), 6, true);
        injector::MakeCALL(pattern.get_first(), SetWindowPosHook, true);

        pattern = hook::pattern("FF 15 ? ? ? ? A1 ? ? ? ? 8B 0D");
        injector::MakeNOP(pattern.get_first(), 6, true);
        injector::MakeCALL(pattern.get_first(), AdjustWindowRectExHook, true);
    }

    if (nLanguage >= 0)
    {
        auto pattern = hook::pattern("E8 ? ? ? ? 8B 04 85 ? ? ? ? 83 C4 04 C3"); //0x495E95
        injector::MakeCALL(pattern.count(1).get(0).get<uintptr_t>(0), SetLanguage, true);
        pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 83 C4 04 C3"); //0x495EB5
        injector::MakeCALL(pattern.count(2).get(1).get<uintptr_t>(5), SetLanguage, true);
    }

    auto pattern = hook::pattern("89 55 00 89 5D 04 C7 45 08 15 00 00 00 89 7D 0C"); //0x649478
    struct ResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            Screen.Width = regs.edx;
            Screen.Height = regs.ebx;
            *(uint32_t*)(regs.ebp + 0x00) = Screen.Width;
            *(uint32_t*)(regs.ebp + 0x04) = Screen.Height;

            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
            Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
            Screen.fWidth43 = static_cast<float>(Screen.Width43);
            Screen.fHudOffset = (1.0f / (Screen.fHeight * (4.0f / 3.0f))) * ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);

            Screen.fWidescreenHudOffset = std::abs(CalculateWidescreenOffset(Screen.fWidth, Screen.fHeight, 640.0f, 480.0f, 0.0f, true));
            Screen.fWidescreenHudOffsetPhone = Screen.fWidescreenHudOffset / (Screen.fHeight * (4.0f / 3.0f));
            if (Screen.fHudAspectRatioConstraint.has_value())
            {
                float value = Screen.fHudAspectRatioConstraint.value();
                if (value < 0.0f || value > (32.0f / 9.0f))
                    Screen.fWidescreenHudOffset = value;
                else
                {
                    value = ClampHudAspectRatio(value, Screen.fAspectRatio);
                    Screen.fWidescreenHudOffset = std::abs(CalculateWidescreenOffset(Screen.fHeight * value, Screen.fHeight, 640.0f, 480.0f, 0.0f, true));
                }
            }
            Screen.fWidescreenHudOffsetRadar = Screen.fWidescreenHudOffset;
            Screen.fWidescreenHudOffset = Screen.fWidescreenHudOffset / (Screen.fHeight * (4.0f / 3.0f));
        }
    }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(6));

    if (bFixHUD)
    {
        auto pattern = hook::pattern("8B 44 24 ? F3 0F 2A C0 0F 28 C8 F3 0F 59 0D ? ? ? ? F3 0F 11 89");
        shsub_654780 = safetyhook::create_inline(pattern.get_first(), sub_654780);

        pattern = hook::pattern("8B 4C 24 ? 85 C9 56");
        //static auto shsub_654D60 = safetyhook::create_inline(pattern.get_first(), sub_654D60);
        static auto shsub_654D60 = safetyhook::create_inline(pattern.get_first(), WidescreenHUD);

        //subtitles
        pattern = hook::pattern("F3 0F 59 05 ? ? ? ? F3 0F 11 4C 24");
        static auto SubtitlesPosHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            regs.xmm1.f32[0] *= Screen.fAspectRatio / (4.0f / 3.0f);
        });

        // 3D models (armoury etc)
        pattern = hook::pattern("F3 0F 10 80 ? ? ? ? 68 15 26 DB FB 8D 4C 24 ? F3 0F 11 44 24 ? E8 ? ? ? ? 8B 4E ? F3 0F 2A C0 F3 0F 59 44 24");
        static auto Move3DPos1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(float*)(regs.esi + 0x54) += (Screen.fWidth - (Screen.fHeight * (4.0f / 3.0f))) / 2.0f;
        });

        pattern = hook::pattern("E8 ? ? ? ? 8B 56 ? F3 0F 2A C0 F3 0F 59 44 24 ? F3 0F 11 46 ? F3 0F 10 82 ? ? ? ? 68 0F E5 4D F5 8D 4C 24 ? F3 0F 11 44 24 ? E8 ? ? ? ? F3 0F 2A C0 F3 0F 59 44 24 ? 68 00 00 80 3F");
        static auto Move3DPos2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(float*)(regs.esp + 0x5C) += (Screen.fWidth - (Screen.fHeight * (4.0f / 3.0f))) / 2.0f;
        });

        // Menu map
        pattern = hook::pattern("F3 0F 59 0D ? ? ? ? F3 0F 10 05 ? ? ? ? F3 0F 58 C8");
        injector::MakeNOP(pattern.get_first(), 8, true);
        static auto MapHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            regs.xmm1.f32[0] *= (Screen.fWidth - (Screen.fHeight * (4.0f / 3.0f))) / 2.0f;
        });

        static float xx = 1.0f;
        pattern = hook::pattern("0F B7 F6 F3 0F 2A DE");
        static auto BlipsHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            regs.xmm2.f32[0] += (Screen.fWidth / 2.0f) - (2.0f * Screen.fHeight / 3.0f) - 64.0f;
        });

        pattern = hook::pattern("89 56 ? F3 0F 59 05");
        static auto PlayerBlipHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            regs.xmm0.f32[0] += (Screen.fWidth / 2.0f) - (2.0f * Screen.fHeight / 3.0f) - 64.0f;
        });

        //radar
        pattern = hook::pattern("F3 0F 58 C8 66 89 44 24 ? F3 0F 2C C1 F3 0F 2A C9 F3 0F 5C C8");
        injector::MakeNOP(pattern.get_first(), 4, true);
        static auto RadarPosHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            regs.xmm1.f32[0] += -15.0f + ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f) - Screen.fWidescreenHudOffsetRadar;
        });

        pattern = hook::pattern("F3 0F 10 0D ? ? ? ? F3 0F 2A C0 F3 0F 5E C8 F3 0F 11 4F");
        injector::MakeNOP(pattern.get_first(), 8, true);
        static auto RadarDiscPosHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            regs.xmm1.f32[0] = -15.0f + ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f) - Screen.fWidescreenHudOffsetRadar;
        });

        pattern = hook::pattern("89 64 24 ? 83 EC 08");
        static auto RadarClipArea = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            regs.eax = Screen.Width;
        });
    }

    if (bFixFOV)
    {
        pattern = hook::pattern("F3 0F 10 45 08 56 8B F1 8B 46 1C"); //0x64BDF9 
        struct FOVHook
        {
            void operator()(injector::reg_pack& regs)
            {
                Screen.fFieldOfView = *(float*)(regs.ebp + 0x8) * (((4.0f / 3.0f)) / (Screen.fAspectRatio));
                regs.xmm0.f32[0] = Screen.fFieldOfView;
            }
        }; injector::MakeInline<FOVHook>(pattern.get_first(0));
    }

    if (fFpsLimit)
    {
        InitFrameLimiter();
    }

    if (fSensitivityFactor)
    {
        fSensitivityFactor *= 0.0099999998f;

        pattern = hook::pattern("D8 0D ? ? ? ? DD 1C 24");
        injector::WriteMemory(pattern.get_first(2), &fSensitivityFactor, true);

        pattern = hook::pattern("F3 0F 59 05 ? ? ? ? 83 C2 FF");
        injector::WriteMemory(pattern.get_first(4), &fSensitivityFactor, true);
    }

    if (fRawInputMouse)
        InitRawInput();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("BF 94 00 00 00 8B C7"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    if (reason == DLL_PROCESS_DETACH)
    {

    }
    return TRUE;
}