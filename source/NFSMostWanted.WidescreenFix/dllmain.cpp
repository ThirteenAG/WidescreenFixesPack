#include "stdafx.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    int32_t Width43;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHudScaleX;
    float fHudPosX;
} Screen;

struct bVector3
{
    float x;
    float y;
    float z;
    float pad;
};

struct bVector4
{
    float x;
    float y;
    float z;
    float w;
};

struct bMatrix4
{
    bVector4 v0;
    bVector4 v1;
    bVector4 v2;
    bVector4 v3;
};

bool bIsResizing = false;
bool bFixHUD = true;
bool bFixFOV = true;
int nScaling = 1;

static float hor3DScale = 4.0f / 3.0f;
static float fRainScaleX = ((0.75f) * (4.0f / 3.0f));
float* HudScaleX_8AF9A4 = (float*)0x8AF9A4;
float* FE_Xpos_894B40 = (float*)0x894B40;
float* AutosculptScale_8AE8F8 = (float*)0x8AE8F8;
float* ArrestBlurScale_8AFA08 = (float*)0x8AFA08;

bool* DrawHUD_57CAA8 = (bool*)0x57CAA8;

namespace ShadowRes
{
    constexpr uint32_t ShadowDepthCheckRes = 3072;

    // 0 = D24S8 (Shadow Map)
    // 1 = INTZ (Depth Buffer as Texture)
    // 2 = DF16 (Depth Buffer as Texture, Radeon Only)
    // 3 = DF24 (Depth Buffer as Texture, Radeon Only)
    uint32_t ShadowMapTexFormats[] = {75, 0x5A544E49, 0x36314644, 0x34324644};
    int CurrentTexFormat = 0;
    bool bForceSharpShadows = false;

    uint32_t Resolution = 2048;
    bool bAutoScaleShadowsRes = true;

    // X resolution ptrs
    uintptr_t dword_6C86B1;
    uint32_t dword_6C878B;
    uint32_t dword_6C87BD;
    uintptr_t dword_6C87F4;

    // Y resolution ptrs
    uintptr_t dword_6C86C1;
    uint32_t* dword_6C8786;
    uint32_t* dword_6C87B8;
    uintptr_t dword_6C87EF;


    uintptr_t DepthBiasAddr_901AC0 = 0x901AC0;
    uintptr_t DepthBiasSlopeAddr_901ABC = 0x901ABC;
    float DepthBias = 4.0f;
    float DepthBiasSlope = 4.0f;

    void update(uint32_t res)
    {
        uint32_t resval = res;
        if (resval > 16384)
            resval = 16384;

        uint32_t resX = resval;
        if (bAutoScaleShadowsRes)
        {
            float newShadowX = resval * (Screen.fAspectRatio / (4.0f / 3.0f));
            resX = static_cast<uint32_t>(newShadowX);
        }
        uint32_t resY = resval;
        
        if (resX > 16384)
            resX = 16384;

        *(uint32_t*)dword_6C86B1 = resX;
        *(uint32_t*)dword_6C878B = resX;
        *(uint32_t*)dword_6C87BD = resX;
        *(uint32_t*)dword_6C87F4 = resX;

        *(uint32_t*)dword_6C86C1 = resY;
        *(uint32_t*)dword_6C8786 = resY;
        *(uint32_t*)dword_6C87B8 = resY;
        *(uint32_t*)dword_6C87EF = resY;

        if (resX > resval)
            resval = resX;

        if ((CurrentTexFormat > 0) && !bForceSharpShadows)
        {
            DepthBias = *(int32_t*)DepthBiasAddr_901AC0 * (static_cast<float>(resval) / 1024.0f);
            DepthBiasSlope = *(float*)DepthBiasSlopeAddr_901ABC * (static_cast<float>(resval) / 1024.0f);
        }
        else if (resval > ShadowDepthCheckRes)
        {
            DepthBias = *(int32_t*)DepthBiasAddr_901AC0 * (static_cast<float>(resval) / static_cast<float>(ShadowDepthCheckRes));
            DepthBiasSlope = *(float*)DepthBiasSlopeAddr_901ABC * (static_cast<float>(resval) / static_cast<float>(ShadowDepthCheckRes));
        }
    }
}

void updateValues(const float& newWidth, const float& newHeight)
{
    //Screen resolution
    Screen.Width = newWidth;
    Screen.Height = newHeight;

    // clamp the size because below 32x32 the game crashes!
    if (Screen.Width < 32)
        Screen.Width = 32;

    if (Screen.Height < 32)
        Screen.Height = 32;

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fHudScaleX = (1.0f / Screen.fWidth * (Screen.fHeight / 480.0f)) * 2.0f;
    Screen.fHudPosX = 640.0f / (640.0f * Screen.fHudScaleX);

    //Autosculpt scaling
    *AutosculptScale_8AE8F8 = 480.0f * Screen.fAspectRatio;

    //Arrest blur
    *ArrestBlurScale_8AFA08 = (1.0f / 640.0f) * ((4.0f / 3.0f) / Screen.fAspectRatio);

    //Rain droplets
    fRainScaleX = ((0.75f / Screen.fAspectRatio) * (4.0f / 3.0f));

    if (bFixFOV)
    {
        hor3DScale = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
        if (nScaling)
            hor3DScale /= 1.047485948f;
    }

    if (bFixHUD)
    {
        *HudScaleX_8AF9A4 = Screen.fHudScaleX;
        *FE_Xpos_894B40 = Screen.fHudPosX;
    }

    if (ShadowRes::Resolution)
        ShadowRes::update(ShadowRes::Resolution);
}

void __stdcall RacingResolution_Hook(int *width, int *height)
{
    *width = Screen.Width;
    *height = Screen.Height;
}

// cave at 0x6E726B - in eDisplayFrame
// for skipping shader recompilation
uint32_t FastWndReset_Exit_True = 0x6E728D;
uint32_t FastWndReset_Exit_False = 0x6E7272;
uint32_t* ResetWnd_982C39 = (uint32_t*)0x00982C39;
void __declspec(naked) FastWndReset_Cave()
{
    if (bIsResizing)
        _asm jmp FastWndReset_Exit_True
    _asm
    {
        mov eax, ResetWnd_982C39
        mov al, byte ptr [eax]
        test al, al
        jmp FastWndReset_Exit_False
    }
}

// cave at 0x6E72C6 - in eDisplayFrame
// at the end of the reset procedure
uint32_t FastWndReset_Finish_Exit = 0x006E72CD;
void __declspec(naked) FastWndReset_Finish_Cave()
{
    bIsResizing = false;
    *DrawHUD_57CAA8 = true;
    _asm
    {
        mov eax, ResetWnd_982C39
        mov [eax], 0
        jmp FastWndReset_Finish_Exit
    }
}

unsigned int GameWndProcAddr = 0;
LRESULT(WINAPI* GameWndProc)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WSFixWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_SIZE:
        {
            bIsResizing = true;
            *DrawHUD_57CAA8 = false;
            updateValues((float)LOWORD(lParam), (float)HIWORD(lParam));
        }
        return TRUE;
    }

    return GameWndProc(hWnd, msg, wParam, lParam);
}

#pragma runtime_checks( "", off )
float NOSTrailScalar = 2.0f;
float NOSTrailPositionScalar = 0.3f;
bMatrix4 carbody_nos;

void(__thiscall* CarRenderInfo_RenderFlaresOnCar)(void* CarRenderInfo, void* eView, bVector3* vec, bMatrix4* matrix, int unk1, int unk2, int unk3) = (void(__thiscall*)(void*, void*, bVector3*, bMatrix4*, int, int, int))0x00742950;
void __stdcall CarRenderInfo_RenderFlaresOnCar_Hook(void* eView, bVector3 *position, bMatrix4 *body_matrix, int unk1, int unk2, int unk3)
{
    uint32_t thethis = 0;
    _asm mov thethis, ecx
    memcpy(&carbody_nos, body_matrix, sizeof(bMatrix4));

    float pos_scale = (NOSTrailScalar * NOSTrailPositionScalar);
    if (pos_scale < 1.0f)
        pos_scale = 1.0f;

    carbody_nos.v0.x *= pos_scale;
    carbody_nos.v0.y *= pos_scale;
    carbody_nos.v0.z *= pos_scale;

    carbody_nos.v2.x *= pos_scale;
    carbody_nos.v2.y *= pos_scale;

    return CarRenderInfo_RenderFlaresOnCar((void*)thethis, eView, position, &carbody_nos, unk1, unk2, unk3);
}

bVector3* WorldPos1;
bVector3* WorldPos2;
bVector3* NOSFlarePos;

uint32_t* NOSTrailCave2Exit = (uint32_t*)0x745040;
void __declspec(naked) NOSTrailCave2()
{
    _asm
    {
        mov WorldPos1, edx
        mov WorldPos2, esi
        lea edx, [esp+0x30]
        mov NOSFlarePos, edx
    }

    (*NOSFlarePos).x = ((*WorldPos1).x - (*WorldPos2).x) * NOSTrailScalar;
    (*NOSFlarePos).y = ((*WorldPos1).y - (*WorldPos2).y) * NOSTrailScalar;
    (*NOSFlarePos).z = ((*WorldPos1).z - (*WorldPos2).z) * NOSTrailScalar;

    _asm
    {
        xor eax, eax
        jmp NOSTrailCave2Exit
    }
}
#pragma runtime_checks( "", restore )

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
    bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) != 0;
    bool bHUDWidescreenMode = iniReader.ReadInteger("MAIN", "HUDWidescreenMode", 1) == 1;
    int nFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1);
    nScaling = iniReader.ReadInteger("MAIN", "Scaling", 1);
    bool bSkipIntro = iniReader.ReadInteger("MISC", "SkipIntro", 0) != 0;
    bool bFixResolutionText = iniReader.ReadInteger("MISC", "FixResolutionText", 1) != 0;

    static auto szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MISC", "CustomUserFilesDirectoryInGameDir", "");
    static std::filesystem::path CustomUserDir;
    if (szCustomUserFilesDirectoryInGameDir.empty() || szCustomUserFilesDirectoryInGameDir == "0")
        szCustomUserFilesDirectoryInGameDir.clear();

    bool bWriteSettingsToFile = iniReader.ReadInteger("MISC", "WriteSettingsToFile", 1) != 0;
    static int nImproveGamepadSupport = iniReader.ReadInteger("MISC", "ImproveGamepadSupport", 0);
    bool bForceHighSpecAudio = iniReader.ReadInteger("MISC", "ForceHighSpecAudio", 1) != 0;
    static float fLeftStickDeadzone = iniReader.ReadFloat("MISC", "LeftStickDeadzone", 10.0f);
    static int SimRate = iniReader.ReadInteger("MISC", "SimRate", -1);
    int nWindowedMode = iniReader.ReadInteger("MISC", "WindowedMode", 0);

    ShadowRes::Resolution = iniReader.ReadInteger("GRAPHICS", "ShadowsRes", 2048);
    ShadowRes::bAutoScaleShadowsRes = iniReader.ReadInteger("GRAPHICS", "AutoScaleShadowsRes", 1) != 0;
    ShadowRes::CurrentTexFormat = iniReader.ReadInteger("GRAPHICS", "ShadowMapTextureFormat", 0);
    ShadowRes::bForceSharpShadows = iniReader.ReadInteger("GRAPHICS", "ForceSharpShadows", 0) != 0;
    static float fRainDropletsScale = iniReader.ReadFloat("GRAPHICS", "RainDropletsScale", 0.5f);
    bool bShadowsFix = iniReader.ReadInteger("GRAPHICS", "ShadowsFix", 1) != 0;
    bool bImproveShadowLOD = iniReader.ReadInteger("GRAPHICS", "ImproveShadowLOD", 1) != 0;
    bool bDisableMotionBlur = iniReader.ReadInteger("GRAPHICS", "DisableMotionBlur", 0) != 0;
    bool bLightStreaksEnable = iniReader.ReadInteger("GRAPHICS", "LightStreaksEnable", 0) != 0;
    bool bBleachByPassEnable = iniReader.ReadInteger("GRAPHICS", "BleachByPassEnable", 0) != 0;
    static uint32_t ForcedGPUVendor = static_cast<uint32_t>(iniReader.ReadInteger("GRAPHICS", "ForcedGPUVendor", 0x10DE));

    bool bFixNOSTrailLength = iniReader.ReadInteger("NOSTrail", "FixNOSTrailLength", 1) == 1;
    bool bFixNOSTrailPosition = iniReader.ReadInteger("NOSTrail", "FixNOSTrailPosition", 0) != 0;
    static float fCustomNOSTrailLength = iniReader.ReadFloat("NOSTrail", "CustomNOSTrailLength", 1.0f);
    NOSTrailPositionScalar = iniReader.ReadFloat("NOSTrail", "NOSTrailPositionScalar", 0.3f);

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    // clamp the size because below 32x32 the game crashes!
    if (Screen.Width < 32)
        Screen.Width = 32;

    if (Screen.Height < 32)
        Screen.Height = 32;

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fHudScaleX = (1.0f / Screen.fWidth * (Screen.fHeight / 480.0f)) * 2.0f;
    Screen.fHudPosX = 640.0f / (640.0f * Screen.fHudScaleX);

    // 08/2022. - keep memory areas unprotected to allow updating of values without constantly calling VirtualProtect ~ Xan
    DWORD oldprotect = 0;

    //Screen resolution
    for (size_t i = 0; i < 2; i++)
    {
        uint32_t* sub_6C27D0 = hook::pattern("A1 ? ? ? ? 83 F8 05 0F ? ? 00 00 00 FF 24 85 ? ? ? ? 8B 44 24 04").count(1).get(0).get<uint32_t>(0);
        injector::MakeJMP(sub_6C27D0, RacingResolution_Hook, true);
    }

    //Autosculpt scaling
    AutosculptScale_8AE8F8 = *hook::pattern("D8 0D ? ? ? ? DA 74 24 18 E8 ? ? ? ? 89 46 04 EB 03").count(1).get(0).get<float*>(2);
    injector::UnprotectMemory(AutosculptScale_8AE8F8, sizeof(float), oldprotect);
    *AutosculptScale_8AE8F8 = 480.0f * Screen.fAspectRatio;

    //Arrest blur
    ArrestBlurScale_8AFA08 = *hook::pattern("D8 0D ? ? ? ? 8B 4C 24 18 8B 54 24 1C").count(1).get(0).get<float*>(2);
    injector::UnprotectMemory(ArrestBlurScale_8AFA08, sizeof(float), oldprotect);
    *ArrestBlurScale_8AFA08 = (1.0f / 640.0f) * ((4.0f / 3.0f) / Screen.fAspectRatio);

    //Rain droplets
    fRainScaleX = ((0.75f / Screen.fAspectRatio) * (4.0f / 3.0f));
    auto pattern = hook::pattern("D9 44 24 0C D8 44 24 10 8B 4C 24 08 8B 44 24 10 8B D1");
    struct RainDropletsHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float esp0C = *(float*)(regs.esp + 0x0C);
            float esp10 = *(float*)(regs.esp + 0x10);
            _asm fld dword ptr[esp0C]
            _asm fmul dword ptr[fRainScaleX]
            _asm fmul dword ptr[fRainDropletsScale]
            _asm fadd dword ptr[esp10]
        }
    }; injector::MakeInline<RainDropletsHook>(pattern.get_first(0), pattern.get_first(8)); //6D480D

    struct RainDropletsYScaleHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float esp08 = *(float*)(regs.esp + 0x08);
            _asm fmul dword ptr[fRainDropletsScale]
                _asm fadd dword ptr[esp08]
                * (uintptr_t*)(regs.esp + 0x38) = regs.ecx;
        }
    }; injector::MakeInline<RainDropletsYScaleHook>(pattern.get_first(30), pattern.get_first(30 + 8)); //6D482B

    if (ShadowRes::Resolution)
    {
        uintptr_t loc_6E5507 = reinterpret_cast<uintptr_t>(hook::pattern("52 68 C3 00 00 00 50 FF 91 E4 00 00 00").get_first(0)) + 0xD;
        uintptr_t loc_6E54E1 = loc_6E5507 - 0x26;

        ShadowRes::DepthBiasAddr_901AC0 = *(uintptr_t*)(loc_6E54E1 + 2);
        ShadowRes::DepthBiasSlopeAddr_901ABC = *(uintptr_t*)(loc_6E5507 + 2);

        struct ShadowDepthBiasHook
        {
            void operator()(injector::reg_pack& regs)
            {
                _asm fld ShadowRes::DepthBias
            }
        }; injector::MakeInline<ShadowDepthBiasHook>(loc_6E54E1, loc_6E54E1 + 6);

        struct ShadowDepthBiasSlopeHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.edx = *(uint32_t*)(&ShadowRes::DepthBiasSlope);
            }
        }; injector::MakeInline<ShadowDepthBiasSlopeHook>(loc_6E5507, loc_6E5507 + 6);

        uintptr_t loc_6BD32B = reinterpret_cast<uintptr_t>(hook::pattern("89 56 08 89 46 0C C7 46 04 0F 00 00 00 89 7E 10 E8").get_first(0)) + 0x10;
        uintptr_t loc_6BD333 = loc_6BD32B + 8;

        ShadowRes::dword_6C86B1 = static_cast<uintptr_t>(injector::GetBranchDestination(loc_6BD32B)) + 1;
        ShadowRes::dword_6C86C1 = static_cast<uintptr_t>(injector::GetBranchDestination(loc_6BD333)) + 1;

        ShadowRes::dword_6C8786 = hook::pattern("68 00 04 00 00 68 00 04 00 00 50 FF 51 5C 85 C0 7C 32").count(1).get(0).get<uint32_t>(1);
        ShadowRes::dword_6C878B = (uint32_t)ShadowRes::dword_6C8786 + 5;
        ShadowRes::dword_6C87B8 = hook::pattern("68 00 04 00 00 68 00 04 00 00 50 FF 52 5C 85 C0 7D 36").count(1).get(0).get<uint32_t>(1);
        ShadowRes::dword_6C87BD = (uint32_t)ShadowRes::dword_6C87B8 + 5;

        uintptr_t loc_6C87E5 = reinterpret_cast<uintptr_t>(hook::pattern("68 44 46 31 36 6A 02 6A 01 68").get_first(0));
        ShadowRes::dword_6C87EF = loc_6C87E5 + 0xA;
        ShadowRes::dword_6C87F4 = ShadowRes::dword_6C87EF + 5;

        injector::UnprotectMemory(ShadowRes::dword_6C86B1, sizeof(uint32_t), oldprotect);
        injector::UnprotectMemory(ShadowRes::dword_6C86C1, sizeof(uint32_t), oldprotect);
        injector::UnprotectMemory(ShadowRes::dword_6C8786, sizeof(uint32_t), oldprotect);
        injector::UnprotectMemory(ShadowRes::dword_6C878B, sizeof(uint32_t), oldprotect);
        injector::UnprotectMemory(ShadowRes::dword_6C87B8, sizeof(uint32_t), oldprotect);
        injector::UnprotectMemory(ShadowRes::dword_6C87BD, sizeof(uint32_t), oldprotect);
        injector::UnprotectMemory(ShadowRes::dword_6C87F4, sizeof(uint32_t), oldprotect);
        injector::UnprotectMemory(ShadowRes::dword_6C87EF, sizeof(uint32_t), oldprotect);

        ShadowRes::update(ShadowRes::Resolution);
    }

    if (ShadowRes::CurrentTexFormat >= 0)
    {
        uintptr_t loc_6C87E5 = reinterpret_cast<uintptr_t>(hook::pattern("68 44 46 31 36 6A 02 6A 01 68").get_first(0));
        uintptr_t loc_6C8798 = loc_6C87E5 - 0x4D;
        uintptr_t loc_6C87D5 = loc_6C87E5 - 0x10;
        uintptr_t loc_6C87A2 = loc_6C87E5 - 0x43;
        uintptr_t loc_6C87B2 = loc_6C87E5 - 0x33;

        uintptr_t loc_6C174E = reinterpret_cast<uintptr_t>(hook::pattern("68 44 46 31 36 6A 03 6A 02 6A 16 6A 01 52 50 FF").get_first(0));
        uintptr_t loc_6C1703 = loc_6C174E - 0x4B;
        uintptr_t loc_6C170A = loc_6C174E - 0x44;
        uintptr_t loc_6C1719 = loc_6C174E - 0x35;
        uintptr_t loc_6C172C = loc_6C174E - 0x22;
        uintptr_t loc_6C1741 = loc_6C174E - 0xD;
        uintptr_t loc_6C1764 = loc_6C174E + 0x16;

        uintptr_t dword_982C08 = *reinterpret_cast<uintptr_t*>(loc_6C172C + 2);

        if (ShadowRes::CurrentTexFormat > (_countof(ShadowRes::ShadowMapTexFormats) - 1))
            ShadowRes::CurrentTexFormat = (_countof(ShadowRes::ShadowMapTexFormats) - 1);

        // disable writes to the shadow map texture type variable
        injector::MakeNOP(loc_6C1703, 6);
        injector::MakeNOP(loc_6C172C, 10);
        injector::MakeNOP(loc_6C1764, 10);

        if (ShadowRes::ShadowMapTexFormats[ShadowRes::CurrentTexFormat] < 0x7F)
        {
            injector::MakeJMP(loc_6C8798, loc_6C87A2);
            injector::WriteMemory<uint8_t>(loc_6C87B2 + 1, ShadowRes::ShadowMapTexFormats[ShadowRes::CurrentTexFormat] & 0xFF, true);

            injector::MakeNOP(loc_6C170A, 2);
            injector::WriteMemory<uint8_t>(loc_6C1719 + 1, ShadowRes::ShadowMapTexFormats[ShadowRes::CurrentTexFormat] & 0xFF, true);
        }
        else
        {
            injector::MakeJMP(loc_6C8798, loc_6C87D5);
            injector::WriteMemory<uint32_t>(loc_6C87E5 + 1, ShadowRes::ShadowMapTexFormats[ShadowRes::CurrentTexFormat], true);

            injector::MakeJMP(loc_6C170A, loc_6C1741);
            injector::WriteMemory<uint32_t>(loc_6C174E + 1, ShadowRes::ShadowMapTexFormats[ShadowRes::CurrentTexFormat], true);
        }

        // Var at 0x00982C08
        // 1 = Sample Shadow Map Directly, 2 = Depth Buffer as Texture
        if (ShadowRes::CurrentTexFormat == 0)
            *(uint32_t*)dword_982C08 = 1;
        else
            *(uint32_t*)dword_982C08 = 2;
    }

    if (ForcedGPUVendor)
    {
        uint32_t* dword_93D898 = *hook::pattern("A1 ? ? ? ? 49 3D 02 10 00 00 89 0D").count(1).get(0).get<uint32_t*>(1);

        for (size_t i = 0; i < 20; i++)
        {
            uint32_t* dword__93D898 = hook::pattern(pattern_str(to_bytes(dword_93D898))).count(1).get(0).get<uint32_t>(0);
            injector::WriteMemory(dword__93D898, &ForcedGPUVendor, true);
        }
    }

    if (ShadowRes::bForceSharpShadows)
    {
        uintptr_t loc_6D5F3A = reinterpret_cast<uintptr_t>(hook::pattern("83 E8 03 89 44 24 14 EB 04 3B C3 7F 44").get_first(0));
        injector::WriteMemory<uint8_t>(loc_6D5F3A + 2, 2, true);
    }

    if (bImproveShadowLOD)
    {
        uint32_t* dword_6E5174 = hook::pattern("68 ? ? ? ? EB ? A1 ? ? ? ? 0D").count(1).get(0).get<uint32_t>(1);
        injector::WriteMemory(dword_6E5174, 0x00000000, true);
        uint32_t* dword_6BFFA2 = hook::pattern("68 ? ? ? ? 50 41 68").count(2).get(1).get<uint32_t>(1);
        injector::WriteMemory(dword_6BFFA2, 0x00006102, true);
    }

    if (bFixHUD)
    {
        HudScaleX_8AF9A4 = *hook::pattern("D8 0D ? ? ? ? D8 25 ? ? ? ? D9 5C 24 20 D9 46 04").count(1).get(0).get<float*>(2);
        injector::UnprotectMemory(HudScaleX_8AF9A4, sizeof(float), oldprotect);
        *HudScaleX_8AF9A4 = Screen.fHudScaleX;

        //fHudScaleY = *(float*)0x8AF9A0;
        //injector::WriteMemory<float>(0x8AF9A0, fHudScaleY, true);

        FE_Xpos_894B40 = *hook::pattern("D8 25 ? ? ? ? D9 5C 24 14 DB 05 ? ? ? ? D8 25 ? ? ? ? D9 5C 24 1C 74 20").count(1).get(0).get<float*>(2);
        injector::UnprotectMemory(FE_Xpos_894B40, sizeof(float), oldprotect);
        *FE_Xpos_894B40 = Screen.fHudPosX;

        // make code read the FE X position from the variable
        pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"); // 0x56FED4
        struct HudPosXHook1
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x19C) = *(float*)(FE_Xpos_894B40);
            }
        }; injector::MakeInline<HudPosXHook1>(pattern.get_first(0), pattern.get_first(11));
       
        pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"); // 0x584EEF
        struct HudPosXHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x90) = *(float*)(FE_Xpos_894B40);
            }
        }; injector::MakeInline<HudPosXHook2>(pattern.get_first(0), pattern.get_first(11));
       
        pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"); // 0x599E79
        struct HudPosXHook3
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x84) = *(float*)(FE_Xpos_894B40);
            }
        }; injector::MakeInline<HudPosXHook3>(pattern.get_first(0), pattern.get_first(11));

        pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"); // 0x0059A120
        struct HudPosXHook4
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0xC4) = *(float*)(FE_Xpos_894B40);
            }
        }; injector::MakeInline<HudPosXHook4>(pattern.get_first(0), pattern.get_first(11));

        pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"); // 0x0059A5AB
        struct HudPosXHook5
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x94) = *(float*)(FE_Xpos_894B40);
            }
        }; injector::MakeInline<HudPosXHook5>(pattern.get_first(0), pattern.get_first(11));

        pattern = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43"); // 0x0059A83E
        injector::MakeInline<HudPosXHook5>(pattern.get_first(0), pattern.get_first(11));

        pattern = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43"); // 0x005A44C8
        struct HudPosXHook6
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x64) = *(float*)(FE_Xpos_894B40);
            }
        }; injector::MakeInline<HudPosXHook6>(pattern.get_first(0), pattern.get_first(8));


        //mirror position fix
        pattern = hook::pattern("C7 44 24 70 00 00 E1 43 C7 44 24 74 00 00 98 41 C7 84 24 80 00 00 00 00 00 3E 43"); // 0x6E70C0
        struct MirrorPosXHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x70) = (*(float*)(FE_Xpos_894B40) - 320.0f) + 450.0f;
                *(float*)(regs.esp + 0xA0) = (*(float*)(FE_Xpos_894B40) - 320.0f) + 450.0f;
                *(float*)(regs.esp + 0x80) = (*(float*)(FE_Xpos_894B40) - 320.0f) + 190.0f;
                *(float*)(regs.esp + 0x90) = (*(float*)(FE_Xpos_894B40) - 320.0f) + 190.0f;
                // others
                *(float*)(regs.esp + 0x74) = 19.0;
                *(float*)(regs.esp + 0x84) = 19.0;
                *(float*)(regs.esp + 0x94) = 89.0;
                *(float*)(regs.esp + 0xA4) = 89.0;
            }
        }; injector::MakeInline<MirrorPosXHook>(pattern.get_first(0), pattern.get_first(82));
    }

    if (bFixFOV)
    {
        hor3DScale = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
        static float ver3DScale = 1.0f; // don't touch this
        static float mirrorScale = 0.4f;
        static float f1215 = 1.215f;
        static float f043434 = 0.43434f;
        static float f1 = 1.0f; // horizontal for vehicle reflection
        static float flt1 = 0.0f;
        static float flt2 = 0.0f;
        static float flt3 = 0.0f;

        if (nScaling)
        {
            hor3DScale /= 1.047485948f;
            f1215 = 1.21f;

            if (nScaling == 2)
            {
                f1215 = 1.27f;
            }
        }

        // Xan's note: please, write direct addresses in names whereever possible. Obfuscating it behind math or pattern detector is stupid and just makes maintainence harder.
        
        uint32_t* dword_6CF4F0 = hook::pattern("DB 40 18 DA 70 14").count(1).get(0).get<uint32_t>(0);
        uintptr_t loc_6CF4EA = (uintptr_t)dword_6CF4F0 - 6;
        uintptr_t loc_6CF4F6 = (uintptr_t)dword_6CF4F0 + 6;
        
        struct FOVHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uint32_t*)(regs.edi + 0x60);

                if (regs.ecx == 1 || regs.ecx == 4) //Headlights stretching, reflections etc 
                {
                    flt1 = hor3DScale;
                    flt2 = f043434;
                    flt3 = f1215;
                }
                else
                {
                    if (regs.ecx > 10)
                    {
                        flt1 = f1;
                        flt2 = 0.5f;
                        flt3 = 1.0f;
                        _asm fld ds : f1
                        return;
                    }
                    else
                    {
                        flt1 = 1.0f;
                        flt2 = 0.5f;
                        flt3 = 1.0f;
                    }
                }

                if (regs.ecx == 3) //if rearview mirror
                    _asm fld ds : mirrorScale
                else
                    _asm fld ds : ver3DScale
            }
        }; injector::MakeInline<FOVHook>(loc_6CF4EA, loc_6CF4F6);
        injector::WriteMemory(dword_6CF4F0, 0x9001F983, true); //cmp     ecx, 1

        uint32_t* dword_6CF566 = hook::pattern("D8 3D ? ? ? ? D9 5C 24 20 DB 44 24 30 D8 4C 24 2C").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_6CF566, &flt1, true);

        // FOV being different in menus and in-game fix
        uint32_t* dword_6CF578 = hook::pattern("D8 0D ? ? ? ? E8 ? ? ? ? 8B F8 57 E8").count(2).get(1).get<uint32_t>(2);
        injector::WriteMemory(dword_6CF578, &flt2, true);

        uint32_t* dword_6CF5DC = hook::pattern("D8 3D ? ? ? ? D9 44 24 20 D8 64 24 24").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_6CF5DC, &flt3, true);

        //Shadow pop-in fix
        uint32_t* dword_6C9653 = hook::pattern("D8 0D ? ? ? ? D9 5C 24 ? E8 ? ? ? ? 8A").count(1).get(0).get<uint32_t>(2);
        static float fShadowDistanceMultiplier = 10.0f;
        injector::WriteMemory((uint32_t)dword_6C9653, &fShadowDistanceMultiplier, true);

        // Shadow camera FOV fix
        uintptr_t loc_6E4652 = reinterpret_cast<uintptr_t>(hook::pattern("8B 46 60 8B 48 18 8B 50 14 89 4C 24 14 DB 44 24 14").get_first(0)) + 9;
        uintptr_t loc_6E4668 = loc_6E4652 + 0x16;

        uintptr_t loc_6E47E4 = reinterpret_cast<uintptr_t>(hook::pattern("8B 40 60 8B 50 18 8B 40 14 89 54 24 14 89 44 24 18 0F B7 8B C4 00 00 00").get_first(0)) + 0x18;
        uintptr_t loc_6E47EC = loc_6E47E4 + 8;

        uintptr_t loc_6E46B0 = reinterpret_cast<uintptr_t>(hook::pattern("D8 7C 24 1C 0F B7 8B C4 00 00 00 89 4C 24 1C").get_first(0)) + 0xF;
        uintptr_t loc_6E4830 = reinterpret_cast<uintptr_t>(hook::pattern("D8 7C 24 1C 0F B7 93 C4 00 00 00 89 54 24 1C").get_first(0)) + 0xF;

        
        struct ShadowCameraFix1
        {
            void operator()(injector::reg_pack& regs)
            {
                _asm fld ver3DScale
            }
        }; injector::MakeInline<ShadowCameraFix1>(loc_6E4652, loc_6E4652 + 8);
        injector::MakeNOP(loc_6E4668, 4);

        struct ShadowCameraFix2
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uint32_t*)(regs.esp + 0x14) = regs.ecx;
                _asm fld ver3DScale
            }
        }; injector::MakeInline<ShadowCameraFix2>(loc_6E47E4, loc_6E47E4 + 8);
        injector::MakeNOP(loc_6E47EC, 4);

        injector::WriteMemory<uintptr_t>(loc_6E46B0 + 2, (uintptr_t)&hor3DScale, true);
        injector::WriteMemory<uintptr_t>(loc_6E4830 + 2, (uintptr_t)&hor3DScale, true);
    }

    uint32_t* dword_57CB82 = hook::pattern("3A 55 34 0F 85 0B 02 00 00 A1").count(1).get(0).get<uint32_t>(0); // HUD
    uint32_t* dword_5696CB = hook::pattern("8A 41 34 38 86 30 03 00 00 74 52 84 C0").count(1).get(0).get<uint32_t>(0); // HUD
    uint32_t* dword_58D883 = hook::pattern("8A 40 34 5F 5E 5D 3B CB 5B 75 12").count(1).get(0).get<uint32_t>(0); // EA Trax
    uint32_t* dword_56885A = hook::pattern("38 48 34 74 31 8B 4E 38 68 7E 78 8E 90").count(1).get(0).get<uint32_t>(0); // Wrong Way Sign
    if (bHUDWidescreenMode)
    {
        injector::WriteMemory<uint32_t>(dword_57CB82, 0x0F01FA80, true);
        injector::WriteMemory<uint32_t>(dword_5696CB, 0x389001B0, true);
        injector::WriteMemory<uint32_t>(dword_58D883, 0x5F9001B0, true);
        injector::WriteMemory<uint32_t>(dword_56885A, 0x7401F980, true);

        //Widescreen Splash
        pattern = hook::pattern("8B 46 10 8B 3D ? ? ? ? 53 50");
        injector::MakeNOP(pattern.get_first(-2), 2, true);
        pattern = hook::pattern("E8 ? ? ? ? 84 C0 B8 ? ? ? ? 75 ? B8 ? ? ? ? C3");
        auto aWs_mw_ls_splas = *pattern.count(2).get(1).get<char*>(8);
        auto aMw_ls_splash_0 = *pattern.count(2).get(1).get<char*>(15);
        injector::WriteMemoryRaw(aMw_ls_splash_0, aWs_mw_ls_splas, strlen(aWs_mw_ls_splas), true);
        uint32_t* dword_5A3142 = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 51 E8 ? ? ? ? 83 C4 ? 3B FB").count(1).get(0).get<uint32_t>(6);
        injector::WriteMemory(dword_5A3142, 0xC4DF3FF2, true); //"CLICK to continue" (PC)

        //issue #343 HD Compatible For Optimal Gaming Logo
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 0C E8 ? ? ? ? 85 C0 75 ? 8B 46");
        injector::MakeCALL(pattern.get_first(0), injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 8B 46 10 68 6D 44 CF 13"), true), true);
    }
    else
    {
        injector::WriteMemory<uint32_t>(dword_57CB82, 0x0F00FA80, true);
        injector::WriteMemory<uint32_t>(dword_5696CB, 0x389000B0, true);
        injector::WriteMemory<uint32_t>(dword_58D883, 0x5F9000B0, true);
        injector::WriteMemory<uint32_t>(dword_56885A, 0x7400F980, true);
    }

    if (nFMVWidescreenMode)
    {
        uint32_t* dword_59A02A = hook::pattern("68 00 00 80 3F 68 00 00 00 3F 68 00 00 00 3F 68 00 00 00 BF 68 00 00 00 BF 8B CB E8 ? ? ? ? 8B 4C 24 18").count(1).get(0).get<uint32_t>(6);
        injector::WriteMemory<float>(dword_59A02A + 0, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Height (Bottom)
        injector::WriteMemory<float>((uint32_t)dword_59A02A + 5, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Right)
        injector::WriteMemory<float>((uint32_t)dword_59A02A + 10, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Height (Top)
        injector::WriteMemory<float>((uint32_t)dword_59A02A + 15, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Left)
        
        if (nFMVWidescreenMode > 1)
        {
            injector::WriteMemory<float>(dword_59A02A + 0, (0.5f / ((4.0f / 3.0f) / (4.0f / 3.0f))), true); // Height (Bottom)
            injector::WriteMemory<float>((uint32_t)dword_59A02A + 5, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Right)
            injector::WriteMemory<float>((uint32_t)dword_59A02A + 10, -(0.5f / ((4.0f / 3.0f) / (4.0f / 3.0f))), true); // Height (Top)
            injector::WriteMemory<float>((uint32_t)dword_59A02A + 15, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true); // Width (Left)
        }
    }

    if (bShadowsFix)
    {
        //dynamic shadow fix that stops them from disappearing when going into tunnels, under bridges, etc.
        uint32_t* dword_6DE377 = hook::pattern("75 3B C7 05 ? ? ? ? 00 00 80 3F").count(1).get(0).get<uint32_t>(0);
        injector::MakeNOP(dword_6DE377, 2, true);
        injector::WriteMemory((uint32_t)dword_6DE377 + 8, 0, true);
    
        //car shadow opacity
        uint32_t* dword_8A0E50 = *hook::pattern("D9 05 ? ? ? ? 8B 54 24 70 D9 1A E9 D1").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory(dword_8A0E50, 60.0f, true);
    }

    if (bForceHighSpecAudio)
    {
        uint32_t* dword_4C41F5 = hook::pattern("C7 05 ? ? ? ? ? ? 00 00 A1 ? ? ? ? 85 C0 ? ? A3 ? ? ? ? 8D 44 24 08").count(1).get(0).get<uint32_t>(6);
        injector::WriteMemory<int>(dword_4C41F5, 44100, true);
    }

    if (!szCustomUserFilesDirectoryInGameDir.empty())
    {
        CustomUserDir = GetExeModulePath<std::filesystem::path>();
        CustomUserDir.append(szCustomUserFilesDirectoryInGameDir);

        auto SHGetFolderPathAHook = [](HWND /*hwnd*/, int /*csidl*/, HANDLE /*hToken*/, DWORD /*dwFlags*/, LPSTR pszPath) -> HRESULT
        {
            CreateDirectoryW((LPCWSTR)(CustomUserDir.u16string().c_str()), NULL);
            memcpy(pszPath, CustomUserDir.u8string().data(), CustomUserDir.u8string().size() + 1);

            return S_OK;
        };

        auto pattern = hook::pattern("50 6A 00 6A 00 68 ? 80 00 00 6A 00");
        for (size_t i = 0; i < pattern.size(); i++)
        {
            uint32_t* dword_6CBF17 = pattern.get(i).get<uint32_t>(12);
            if (*(BYTE*)dword_6CBF17 != 0xFF)
                dword_6CBF17 = pattern.get(i).get<uint32_t>(14);

            injector::MakeCALL((uint32_t)dword_6CBF17, static_cast<HRESULT(WINAPI*)(HWND, int, HANDLE, DWORD, LPSTR)>(SHGetFolderPathAHook), true);
            injector::MakeNOP((uint32_t)dword_6CBF17 + 5, 1, true);
        }
    }

    if (bSkipIntro)
    {
        static auto counter = 0;
        static auto og_value = 0;
        pattern = hook::pattern("A1 ? ? ? ? 85 C0 74 1C 8B 45 04");
        static uint32_t* dword_926144 = *pattern.get_first<uint32_t*>(1);
        struct SkipIntroHook
        {
            void operator()(injector::reg_pack& regs)
            {
                if (counter < 3)
                {
                    if (counter == 0)
                        og_value = *dword_926144;
                    *dword_926144 = 1;
                    counter++;
                }
                else
                {
                    *dword_926144 = og_value;
                }

                regs.eax = *(uint32_t*)dword_926144;
            }
        }; injector::MakeInline<SkipIntroHook>(pattern.get_first(0));
    }

    if (nImproveGamepadSupport)
    {
        pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 51 A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 04 89 04 24 85 C0 C7 44 24 ? ? ? ? ? 74 22 8B 4C 24 24 8B 54 24 20 51");
        static auto CreateResourceFile = (void*(*)(const char* ResourceFileName, int32_t ResourceFileType, int, int, int)) pattern.get_first(0); //0x0065FD30
        pattern = hook::pattern("8B 44 24 04 56 8B F1 8B 4C 24 0C 89 46 34 89 4E 38 FF 05 ? ? ? ? 8B 46 3C 85 C0 75 14 8B 56 10 C1 EA 03 81 E2 ? ? ? ? 52 8B CE");
        static auto ResourceFileBeginLoading = (void(__thiscall *)(void* rsc, int a1, int a2)) pattern.get_first(0); //0x006616F0;
        static auto LoadResourceFile = [](const char* ResourceFileName, int32_t ResourceFileType, int32_t nUnk1 = 0, int32_t nUnk2 = 0, int32_t nUnk3 = 0, int32_t nUnk4 = 0, int32_t nUnk5 = 0)
        {
            auto r = CreateResourceFile(ResourceFileName, ResourceFileType, nUnk1, nUnk2, nUnk3);
            ResourceFileBeginLoading(r, nUnk4, nUnk5);
        };

        if (nImproveGamepadSupport < 3)
        {
            static auto TPKPath = GetThisModulePath<std::string>().substr(GetExeModulePath<std::string>().length());

            if (nImproveGamepadSupport == 1)
                TPKPath += "buttons-xbox.tpk";
            else if (nImproveGamepadSupport == 2)
                TPKPath += "buttons-playstation.tpk";

            static injector::hook_back<void(__cdecl*)()> hb_662B30;
            auto LoadTPK = []()
            {
                if (std::filesystem::exists(TPKPath))
                {
                    LoadResourceFile(TPKPath.c_str(), 1);
                }
                return hb_662B30.fun();
            };

            pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 56 57 B9 ? ? ? ? E8"); //0x6660B6
            hb_662B30.fun = injector::MakeCALL(pattern.get_first(0), static_cast<void(__cdecl*)()>(LoadTPK), true).get();

            /*
            cursor
            constexpr float cursorScale = 1.0f * (128.0f / 16.0f);
            pattern = hook::pattern("C7 84 24 34 02 00 00 00 00 80 3F D9"); //5704F8
            injector::WriteMemory<float>(pattern.get_first(7), cursorScale, true);
            injector::WriteMemory<float>(pattern.get_first(36), cursorScale, true);
            injector::WriteMemory<float>(pattern.get_first(47), cursorScale, true);
            injector::WriteMemory<float>(pattern.get_first(69), cursorScale, true);
            */
        }

        struct PadState
        {
            int32_t LSAxis1;
            int32_t LSAxis2;
            int32_t LTRT;
            int32_t A;
            int32_t B;
            int32_t X;
            int32_t Y;
            int32_t LB;
            int32_t RB;
            int32_t Select;
            int32_t Start;
            int32_t LSClick;
            int32_t RSClick;
        };

        pattern = hook::pattern("C7 45 E0 01 00 00 00 89 5D E4"); //0x7F2AE2
        static uintptr_t ButtonsState = (uintptr_t)*hook::pattern("0F B6 54 24 04 33 C0 B9").count(1).get(0).get<uint32_t*>(8); //0x514B90
        static int32_t* nGameState = (int32_t*)*hook::pattern("83 3D ? ? ? ? 06 ? ? A1").count(1).get(0).get<uint32_t*>(2); //0x925E90
        struct CatchPad
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uintptr_t*)(regs.ebp - 0x20) = 1; //mov     dword ptr [ebp-20h], 1

                PadState* PadKeyPresses = (PadState*)(regs.esi + 0x234); //dpad is PadKeyPresses+0x220

                //Keyboard 
                //008F3584 2
                //008F3514 3 
                //008F34F8 4 
                if (PadKeyPresses != nullptr && PadKeyPresses != (PadState*)0x1)
                {
                    if (PadKeyPresses->Select) //3
                    {
                        *(int32_t*)(ButtonsState + 0xCC) = 1;
                    }
                    else
                    {
                        *(int32_t*)(ButtonsState + 0xCC) = 0;
                    }

                    if (PadKeyPresses->X) //2
                    {
                        *(int32_t*)(ButtonsState + 0x13C) = 1;
                    }
                    else
                    {
                        *(int32_t*)(ButtonsState + 0x13C) = 0;
                    }
                    if (PadKeyPresses->Start) //4
                    {
                        *(int32_t*)(ButtonsState + 0xB0) = 1;
                    }
                    else
                    {
                        *(int32_t*)(ButtonsState + 0xB0) = 0;
                    }
                    if (PadKeyPresses->LSClick && PadKeyPresses->RSClick)
                    {
                        if (*nGameState == 3)
                        {
                            keybd_event(BYTE(VkKeyScan('Q')), 0, 0, 0);
                            keybd_event(BYTE(VkKeyScan('Q')), 0, KEYEVENTF_KEYUP, 0);
                        }
                    }
                }
            }
        }; injector::MakeInline<CatchPad>(pattern.get_first(0), pattern.get_first(7));

        const char* ControlsTexts[] = { " 2", " 3", " 4", " 5", " 6", " 7", " 8", " 9", " 10", " 1", " Up", " Down", " Left", " Right", "X Rotation", "Y Rotation", "X Axis", "Y Axis", "Z Axis", "Hat Switch" };
        const char* ControlsTextsXBOX[] = { "B", "X", "Y", "LB", "RB", "View (Select)", "Menu (Start)", "Left stick", "Right stick", "A", "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right", "Right stick Left/Right", "Right stick Up/Down", "Left stick Left/Right", "Left stick Up/Down", "LT / RT", "D-pad" };
        const char* ControlsTextsPS[] = { "Circle", "Square", "Triangle", "L1", "R1", "Select", "Start", "L3", "R3", "Cross", "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right", "Right stick Left/Right", "Right stick Up/Down", "Left stick Left/Right", "Left stick Up/Down", "L2 / R2", "D-pad" };

        static std::vector<std::string> Texts(ControlsTexts, std::end(ControlsTexts));
        static std::vector<std::string> TextsXBOX(ControlsTextsXBOX, std::end(ControlsTextsXBOX));
        static std::vector<std::string> TextsPS(ControlsTextsPS, std::end(ControlsTextsPS));

        pattern = hook::pattern("8B 44 24 1C 50 E8 ? ? ? ? 83 C4 10 5F 5E 5B C3"); //0x6282D3
        injector::WriteMemory<uint8_t>(pattern.get_first(16 + 5), 0xC3, true);
        struct Buttons
        {
            void operator()(injector::reg_pack& regs)
            {
                auto pszStr = *(char**)(regs.esp + 4);
                auto it = std::find_if(Texts.begin(), Texts.end(), [&](const std::string& str) { std::string s(pszStr); return s.find(str) != std::string::npos; });
                auto i = std::distance(Texts.begin(), it);

                if (it != Texts.end())
                {
                    if (nImproveGamepadSupport != 2)
                        strcpy(pszStr, TextsXBOX[i].c_str());
                    else
                        strcpy(pszStr, TextsPS[i].c_str());
                }
            }
        }; injector::MakeInline<Buttons>(pattern.get_first(16));

        pattern = hook::pattern("5E D8 0D ? ? ? ? 59 C2 08 00"); //0x628A51
        static auto unk_91F7F4 = *hook::get_pattern<void**>("3D ? ? ? ? 89 7C 24 14 0F 8C ? ? ? ? 5F", 1);
        struct MenuRemap
        {
            void operator()(injector::reg_pack& regs)
            {
                static const auto f0078125 = 0.0078125f;
                _asm fmul dword ptr[f0078125]

                auto dword_91FABC = &unk_91F7F4[178];
                auto dword_91FAF0 = &unk_91F7F4[191];
                auto dword_91FC90 = &unk_91F7F4[295];

                *(uint32_t*)(dword_91FABC) = 0; // "Enter"; changed B to A
                *(uint32_t*)(dword_91FAF0) = 1; // "ESC"; changed X to B
                *(uint32_t*)(dword_91FC90) = 3; // "1"; changed A to Y
            }
        }; injector::MakeInline<MenuRemap>(pattern.get_first(1), pattern.get_first(7));

        // Start menu text
        uint32_t* dword_5A313D = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 51 E8 ? ? ? ? 83 C4 ? 3B FB").count(1).get(0).get<uint32_t>(1);
        if (nImproveGamepadSupport != 2)
            injector::WriteMemory(dword_5A313D, 0xD18D4C4C, true); //"Press START to begin" (Xbox)
        else
            injector::WriteMemory(dword_5A313D, 0xDC64C04C, true); //"Press START button" (PlayStation)
    }

    if (bDisableMotionBlur)
    {
        uint32_t* dword_6DF1D2 = hook::pattern("FF 91 ? ? ? ? 89 1D ? ? ? ? 39 1D ? ? ? ? ? ? 39 1D").count(1).get(0).get<uint32_t>(18);
        injector::WriteMemory<uint8_t>(dword_6DF1D2, 0xEB, true);
    }

    if (nWindowedMode)
    {
        uint32_t* dword_6E6D77 = hook::pattern("A1 ? ? ? ? 68 00 00 00 10 6A F0 50").count(1).get(0).get<uint32_t>(0);
        uint32_t* dword_6E6D8A = (uint32_t*)((uint32_t)dword_6E6D77 + 0x13);
        uint32_t* dword_6E6C94 = hook::pattern("FF 15 ? ? ? ? 50 A3 ? ? ? ? FF 15 ? ? ? ? 39 1D ? ? ? ? 75 0E").count(1).get(0).get<uint32_t>(0);
        uint32_t* dword_6E6C3A = hook::pattern("FF 15 ? ? ? ? 6A 0D 6A 0D E8 ? ? ? ? 50 68 04 67 DD 08").count(1).get(0).get<uint32_t>(0);
        uint32_t* dword_982BF0 = *(uint32_t**)((uint32_t)dword_6E6C94 + 0x14);

        // skip SetWindowLong because it messes things up
        injector::MakeJMP(dword_6E6D77, dword_6E6D8A, true);
        // hook the offending functions
        injector::MakeNOP(dword_6E6C94, 6, true);
        injector::MakeCALL(dword_6E6C94, WindowedModeWrapper::CreateWindowExA_Hook, true);
        injector::MakeNOP(dword_6E6C3A, 6, true);
        injector::MakeCALL(dword_6E6C3A, WindowedModeWrapper::AdjustWindowRect_Hook, true);

        *(int*)dword_982BF0 = 1;

        switch (nWindowedMode)
        {
        case 5:
            WindowedModeWrapper::bStretchWindow = true;
            break;
        case 4:
            WindowedModeWrapper::bScaleWindow = true;
            break;
        case 3:
        case 2:
            WindowedModeWrapper::bBorderlessWindowed = false;
            break;
        default:
            break;
        }

        if (nWindowedMode == 3)
        {
            WindowedModeWrapper::bEnableWindowResize = true;

            // dereference the current WndProc from the game executable and write to the function pointer (to maximize compatibility)
            uint32_t* wndproc_addr = hook::pattern("C7 44 24 44 ? ? ? ? 89 5C 24 48 89 5C 24 4C").count(1).get(0).get<uint32_t>(4);
            GameWndProcAddr = *(unsigned int*)wndproc_addr;
            GameWndProc = (LRESULT(WINAPI*)(HWND, UINT, WPARAM, LPARAM))GameWndProcAddr;
            injector::WriteMemory<unsigned int>(wndproc_addr, (unsigned int)&WSFixWndProc, true);


            DrawHUD_57CAA8 = hook::pattern("8B 41 0C BD 01 00 00 00 3B C5").count(1).get(0).get<bool>(4);
            ResetWnd_982C39 = *hook::pattern("A0 ? ? ? ? 84 C0 74 59 E8 ? ? ? ?").count(1).get(0).get<uint32_t*>(1);
            injector::UnprotectMemory(DrawHUD_57CAA8, 1, oldprotect);

            // cave entrypoints
            uint32_t* dword_6E726B = hook::pattern("A0 ? ? ? ? 84 C0 74 59 E8 ? ? ? ?").count(1).get(0).get<uint32_t>(0);
            uint32_t* dword_6E72C6 = hook::pattern("C6 05 ? ? ? ? 00 39 2D ? ? ? ? 89 2D ? ? ? ?").count(1).get(0).get<uint32_t>(0);;
            FastWndReset_Exit_True = ((uint32_t)dword_6E726B) + 0x22;
            FastWndReset_Exit_False = ((uint32_t)dword_6E726B) + 0x7;
            FastWndReset_Finish_Exit = ((uint32_t)dword_6E72C6) + 0x7;

            injector::MakeJMP(dword_6E726B, FastWndReset_Cave, true);
            injector::MakeJMP(dword_6E72C6, FastWndReset_Finish_Cave, true);
        }
    }

    if (fLeftStickDeadzone)
    {
        // DInput [ 0 32767 | 32768 65535 ] 
        fLeftStickDeadzone /= 200.0f;

        pattern = hook::pattern("89 86 34 02 00 00 8D 45 D4"); //0x7F29B2
        struct DeadzoneHookX
        {
            void operator()(injector::reg_pack& regs)
            {
                double dStickState = (double)regs.eax / 65535.0;
                dStickState -= 0.5;
                if (std::abs(dStickState) <= fLeftStickDeadzone)
                    dStickState = 0.0;
                dStickState += 0.5;
                *(uint32_t*)(regs.esi + 0x234) = (uint32_t)(dStickState * 65535.0);
            }
        }; injector::MakeInline<DeadzoneHookX>(pattern.get_first(0), pattern.get_first(6));

        struct DeadzoneHookY
        {
            void operator()(injector::reg_pack& regs)
            {
                double dStickState = (double)regs.edx / 65535.0;
                dStickState -= 0.5;
                if (std::abs(dStickState) <= fLeftStickDeadzone)
                    dStickState = 0.0;
                dStickState += 0.5;
                *(uint32_t*)(regs.esi + 0x238) = (uint32_t)(dStickState * 65535.0);
            }
        }; injector::MakeInline<DeadzoneHookY>(pattern.get_first(18 + 0), pattern.get_first(18 + 6));
    }

    if (SimRate)
    {
        if (SimRate < 0)
        {
            if (SimRate == -1)
                SimRate = GetDesktopRefreshRate();
            else if (SimRate == -2)
                SimRate = GetDesktopRefreshRate() * 2;
            else
                SimRate = 60;
        }

        // this shouldn't be necessary - if the game frametime/rate is matched with the sim frametime/rate, then everything is fine.
        // limit rate to avoid issues...
        //if (SimRate > 360)
        //    SimRate = 360;
        //if (SimRate < 60)
        //    SimRate = 60;

        static float FrameTime = 1.0f / SimRate;
        //static float fnFPSLimit = (float)SimRate;

        // Frame times
        // PrepareRealTimestep() NTSC video mode frametime .rdata
        uint32_t* dword_6612EC = hook::pattern("D9 05 ? ? ? ? B9 ? ? ? ? D8 44 24 14 D9 5C 24 14").count(1).get(0).get<uint32_t>(53); //0x006612B7 anchor
        // MainLoop frametime .text
        float* flt_666100 = hook::pattern("E8 ? ? ? ? 68 89 88 88 3C").count(1).get(0).get<float>(6);
        // World_Service UglyTimestepHack initial state .data
        float* flt_903290 = *hook::pattern("8B 0D ? ? ? ? 85 C9 C7 05 ? ? ? ? 00 00 00 00 74 05").count(1).get(0).get<float*>(10); //0x0075AAD8 dereference
        // GetDebugRealTime() NTSC frametime 1 .text
        uint32_t* dword_65C78F = hook::pattern("83 EC 08 A1 ? ? ? ? 89 44 24 04 A1 ? ? ? ? 85 C0 75 08").count(1).get(0).get<uint32_t>(0x1F); //0x0065C770 anchor
        // GetDebugRealTime() NTSC frametime 2 .text
        uint32_t* dword_65C7D9 = hook::pattern("83 EC 08 A1 ? ? ? ? 89 44 24 04 A1 ? ? ? ? 85 C0 75 08").count(1).get(0).get<uint32_t>(0x69); //0x0065C770 anchor

        injector::WriteMemory(dword_6612EC, &FrameTime, true);
        //injector::WriteMemory(flt_8970F0, FrameTime, true);
        injector::WriteMemory(flt_666100, FrameTime, true);
        *flt_903290 = FrameTime;
        injector::WriteMemory(dword_65C78F, &FrameTime, true);
        injector::WriteMemory(dword_65C7D9, &FrameTime, true);

        // Frame rates
        // TODO: if any issues arise, figure out where 60.0 values are used and update the constants...
    }

    if (bFixNOSTrailLength)
    {
        static int TargetRate = 60;

        if (SimRate)
            TargetRate = SimRate;

        constexpr float NOSTargetFPS = 60.0f; // original FPS we're targeting from. Consoles target 60 but run at 30, hence have longer trails than PC. Targeting 60 is smarter due to less issues with shorter trails. Use SimRate -2 to get the same effect as console versions.
        NOSTrailScalar = (TargetRate / NOSTargetFPS) * fCustomNOSTrailLength;

        pattern = hook::pattern("EB 06 8D 9B 00 00 00 00 40 89 44 24 18"); // 0x00745038
        injector::MakeJMP(pattern.get_first(0), NOSTrailCave2, true);
        NOSTrailCave2Exit = (uint32_t*)pattern.get_first(8);

        if (bFixNOSTrailPosition)
        {
            pattern = hook::pattern("D9 44 24 30 6A 02 D8 4C 24 10 6A 00 8B CB C1 E1 06"); // 0x00745088
            injector::MakeCALL(pattern.get_first(0x60), CarRenderInfo_RenderFlaresOnCar_Hook, true);
            pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 74 53 56 57 8B F9 89 7C 24 3C"); // 0x00742950
            CarRenderInfo_RenderFlaresOnCar = (void(__thiscall*)(void*, void*, bVector3*, bMatrix4*, int, int, int))pattern.get_first(0);
        }
    }

    if (bLightStreaksEnable)
    {
        uintptr_t loc_6C1841 = reinterpret_cast<uintptr_t>(hook::pattern("A1 ? ? ? ? 8B 0C 85 ? ? ? ? 85 C9 75 20 85 C0").get_first(0)) - 0x113;
        uintptr_t loc_6C19EC = reinterpret_cast<uintptr_t>(hook::pattern("C7 44 24 10 40 00 00 00 FF 15 ? ? ? ? 39 7C 24 18").get_first(0)) - 0x134;
        uintptr_t loc_6C1AD8 = loc_6C19EC + 0xEC;
        uintptr_t loc_6C310B = reinterpret_cast<uintptr_t>(hook::pattern("99 83 E2 03 03 C2 8B C8 8B C7 99 83 E2 03 03 C2 C1 F8 02 A3").get_first(0)) - 0x20;

        uintptr_t g_LightStreaksEnable = *reinterpret_cast<uintptr_t*>(loc_6C1841 + 1);

        // disable control of the variable
        injector::MakeNOP(loc_6C1841, 5);
        injector::MakeNOP(loc_6C19EC, 6);
        injector::MakeNOP(loc_6C1AD8, 6);
        injector::MakeNOP(loc_6C310B, 6);

        *(uint32_t*)g_LightStreaksEnable = 1;
    }

    if (bBleachByPassEnable)
    {
        uintptr_t loc_6C18D1 = reinterpret_cast<uintptr_t>(hook::pattern("A1 ? ? ? ? 8B 0C 85 ? ? ? ? 85 C9 75 20 85 C0").get_first(0)) - 0x83;
        uintptr_t loc_6C1A9F = reinterpret_cast<uintptr_t>(hook::pattern("C7 44 24 10 40 00 00 00 FF 15 ? ? ? ? 39 7C 24 18").get_first(0)) - 0x81;
        uintptr_t loc_6C1B0E = loc_6C1A9F + 0x6F;
        uintptr_t loc_6C2FC0 = reinterpret_cast<uintptr_t>(hook::pattern("99 83 E2 03 03 C2 8B C8 8B C7 99 83 E2 03 03 C2 C1 F8 02 A3").get_first(0)) - 0x16B;

        uintptr_t g_BleachByPassEnable = *reinterpret_cast<uintptr_t*>(loc_6C18D1 + 1);

        // disable control of the variable
        injector::MakeNOP(loc_6C18D1, 5);
        injector::MakeNOP(loc_6C1A9F, 6);
        injector::MakeNOP(loc_6C1B0E, 6);
        injector::MakeNOP(loc_6C2FC0, 6);

        *(uint32_t*)g_BleachByPassEnable = 1;
    }

    if (bFixResolutionText)
    {
        uintptr_t loc_51B81F = reinterpret_cast<uintptr_t>(hook::pattern("B8 12 7B 66 76 8B 77 30 85").get_first(0)) + 0xC;

        // Video Options Resolution text
        static wchar_t CurrResString[16];
        struct ResolutionTextFix
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uint32_t*)(regs.esi + 0x1C) |= 0x400000;
                *(uint32_t*)(regs.esi + 0x60) = 0;

                swprintf(CurrResString, L"%ux%u", Screen.Width, Screen.Height);
                *(wchar_t**)(regs.esi + 0x64) = CurrResString;
            }
        }; injector::MakeInline<ResolutionTextFix>(loc_51B81F, loc_51B81F + 0xA);

        uintptr_t loc_5297B0 = reinterpret_cast<uintptr_t>(hook::pattern("8B F0 83 C4 04 89 74 24 10 85 F6 C7 44 24 20 01 00 00 00 74 11 6A 01").get_first(0)) + 0x1E;
        uintptr_t VOptionsVTable = *reinterpret_cast<uintptr_t*>(loc_5297B0 + 2);
        uintptr_t loc_528430 = *reinterpret_cast<uintptr_t*>(VOptionsVTable);
        uintptr_t loc_528431 = loc_528430 + 1;
        uintptr_t loc_528433 = loc_528430 + 3;

        static uintptr_t VOResFreeAddr = static_cast<uintptr_t>(injector::GetBranchDestination(loc_528433));
        
        struct ResolutionTextFreeHook
        {
            void operator()(injector::reg_pack& regs)
            {
                // [obj + 0x30] = FEString
                uintptr_t FEString = *(uintptr_t*)(regs.ecx + 0x30);
                *(wchar_t**)(FEString + 0x64) = nullptr; // null the string so it doesn't get freed by the game!

                regs.esi = regs.ecx;

                reinterpret_cast<void(__thiscall*)(uintptr_t)>(VOResFreeAddr)(regs.ecx);
            }
        }; injector::MakeInline<ResolutionTextFreeHook>(loc_528431, loc_528431 + 7);
    }

    if (bWriteSettingsToFile)
    {
        std::filesystem::path SettingsSavePath;
        if (!szCustomUserFilesDirectoryInGameDir.empty())
            SettingsSavePath = CustomUserDir;

        auto GetFolderPathpattern = hook::pattern("50 6A 00 6A 00 68 ? 80 00 00 6A 00");
        uintptr_t GetFolderPathCallDest = injector::GetBranchDestination(GetFolderPathpattern.count(1).get(0).get<uintptr_t>(14), true).as_int();

        if (GetFolderPathCallDest && szCustomUserFilesDirectoryInGameDir.empty())
        {
            char szSettingsSavePath[MAX_PATH];
            injector::stdcall<HRESULT(HWND, int, HANDLE, DWORD, LPSTR)>::call(GetFolderPathCallDest, NULL, 0x8005, NULL, NULL, szSettingsSavePath);
            SettingsSavePath = szSettingsSavePath;
        }

        SettingsSavePath.append("NFS Most Wanted");
        SettingsSavePath.append("Settings.ini");

        RegistryWrapper("Need for Speed", SettingsSavePath);
        auto RegIAT = *hook::pattern("FF 15 ? ? ? ? 8D 54 24 04 52").get(0).get<uintptr_t*>(2);
        injector::WriteMemory(&RegIAT[0], RegistryWrapper::RegCreateKeyA, true);
        injector::WriteMemory(&RegIAT[1], RegistryWrapper::RegOpenKeyExA, true);
        injector::WriteMemory(&RegIAT[2], RegistryWrapper::RegCloseKey, true);
        injector::WriteMemory(&RegIAT[3], RegistryWrapper::RegSetValueExA, true);
        injector::WriteMemory(&RegIAT[4], RegistryWrapper::RegQueryValueExA, true);
        RegistryWrapper::AddPathWriter("Install Dir", "InstallDir", "Path");
        RegistryWrapper::AddDefault("@", "INSERTYOURCDKEYHERE");
        RegistryWrapper::AddDefault("CD Drive", "D:\\");
        RegistryWrapper::AddDefault("CacheSize", "2936691712");
        RegistryWrapper::AddDefault("SwapSize", "73400320");
        RegistryWrapper::AddDefault("Language", "English");
        RegistryWrapper::AddDefault("StreamingInstall", "0");
        RegistryWrapper::AddDefault("VTMode", "0");
        RegistryWrapper::AddDefault("VERSION", "0");
        RegistryWrapper::AddDefault("SIZE", "0");
        RegistryWrapper::AddDefault("g_CarEnvironmentMapEnable", "3");
        RegistryWrapper::AddDefault("g_CarEnvironmentMapUpdateData", "1");
        RegistryWrapper::AddDefault("g_RoadReflectionEnable", "3");
        RegistryWrapper::AddDefault("g_MotionBlurEnable", "1");
        RegistryWrapper::AddDefault("g_ParticleSystemEnable", "1");
        RegistryWrapper::AddDefault("g_WorldLodLevel", "3");
        RegistryWrapper::AddDefault("g_CarLodLevel", "1");
        RegistryWrapper::AddDefault("g_OverBrightEnable", "1");
        RegistryWrapper::AddDefault("g_FSAALevel", "7");
        RegistryWrapper::AddDefault("g_RainEnable", "1");
        RegistryWrapper::AddDefault("g_TextureFiltering", "2");
        RegistryWrapper::AddDefault("g_RacingResolution", "1");
        RegistryWrapper::AddDefault("g_PerformanceLevel", "5");
        RegistryWrapper::AddDefault("g_VSyncOn", "0");
        RegistryWrapper::AddDefault("g_ShadowDetail", "2");
        RegistryWrapper::AddDefault("g_VisualTreatment", "1");
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("C7 00 80 02 00 00 C7 01 E0 01 00 00 C2 08 00"));
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
