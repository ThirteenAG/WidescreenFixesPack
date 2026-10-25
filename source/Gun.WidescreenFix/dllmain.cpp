#include "stdafx.h"
#include "FixSettings.hpp"
#include "Display.hpp"
#include "NativeMenu.hpp"

ASI_SETTINGS_EXPORT(FixSettings())

struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    float fHudScaleX;
    float fHudOffset;
    int Width43;
    float* DefaultAR;
    uint8_t** CutsceneOwner;
} Screen;

bool IsGameplayFOV(const uint8_t* camera)
{
    if (!camera || !Screen.CutsceneOwner || !*Screen.CutsceneOwner)
        return false;
    auto manager = *reinterpret_cast<uint8_t**>(*Screen.CutsceneOwner + 0x1F0);
    if (!manager)
        return false;
    auto node = *reinterpret_cast<uint8_t**>(manager + 0x6C);
    // The native empty list has a sentinel id of -1. Unavailable state keeps the native framing.
    return node && *reinterpret_cast<int32_t*>(node + 4) == -1 && *reinterpret_cast<const float*>(camera + 0xD4) == 1.0f;
}

float GameplayFOV(float fov, float factor, const uint8_t* camera)
{
    if (!std::isfinite(fov) || fov <= 0.0f || !std::isfinite(factor) || factor <= 0.0f || factor == 1.0f || !IsGameplayFOV(camera))
        return fov;
    return std::min(fov * std::clamp(factor, 0.5f, 2.0f), 170.0f);
}

float SetAspect()
{
    return Screen.fAspectRatio * ((*Screen.DefaultAR) / (4.0f / 3.0f));
}

SafetyHookInline shPlayMovie;
int __cdecl PlayMovie(const char* name)
{
    // Publisher and studio logos; the story intro and attract videos still play.
    std::string_view movie{ name ? name : "" };
    for (auto logo : { "atvi", "beenox", "nsintro" })
    {
        auto it = std::search(movie.begin(), movie.end(), logo, logo + std::strlen(logo), [](char a, char b) { return std::tolower((unsigned char)a) == b; });
        if (it != movie.end())
            return 0;
    }
    return shPlayMovie.ccall<int>(name);
}

void Init()
{
    Screen.Width = FixSettings().Int("MAIN.ResX");
    Screen.Height = FixSettings().Int("MAIN.ResY");
    bool bFixHUD = FixSettings().Bool("MAIN.FixHUD");
    bool bFixFOV = FixSettings().Bool("MAIN.FixFOV");
    bool bSkipIntro = FixSettings().Bool("MAIN.SkipIntro");
    uint32_t nGameSpeed = static_cast<uint32_t>(std::clamp<int32_t>(FixSettings().Int("MISC.GameSpeed"), 0, 1000));

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 32767 || Screen.Height > 32767)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fHudOffset = (0.5f / ((4.0f / 3.0f) / (Screen.fAspectRatio)));
    Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));

    auto pattern = hook::pattern("A3 ? ? ? ? E8 ? ? ? ? 84 C0 74 38 8B 0D");
    auto resolution2 = hook::pattern("A3 ? ? ? ? E8 ? ? ? ? 6A 00 68 ? ? ? ? 8D 4C 24 14 A3");
    auto aspect = hook::pattern("E8 ? ? ? ? D8 1D ? ? ? ? D9 44 24");
    if (pattern.size() != 1 || resolution2.size() != 1 || aspect.size() != 1)
    {
        OutputDebugStringA("Gun Widescreen Fix: unsupported resolution signatures.\n");
        return;
    }
    auto pSetAR = injector::GetBranchDestination(aspect.get_first(), true).as_int();
    if (*reinterpret_cast<uint16_t*>(pSetAR) != 0x05D9)
    {
        OutputDebugStringA("Gun Widescreen Fix: aspect function already replaced.\n");
        return;
    }
    static auto dword_6B75A0_Y = *pattern.count(1).get(0).get<uint32_t*>(1);
    static auto dword_6B759C_X = dword_6B75A0_Y - 1;
    struct ResHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_6B759C_X = Screen.Width;
            *dword_6B75A0_Y = Screen.Height;
        }
    }; injector::MakeInline<ResHook1>(pattern.get_first());

    pattern = resolution2;
    struct ResHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_6B759C_X = Screen.Width;
            *dword_6B75A0_Y = Screen.Height;
        }
    }; injector::MakeInline<ResHook2>(pattern.get_first());

    Screen.DefaultAR = *(float**)(pSetAR + 2);
    injector::MakeJMP(pSetAR, SetAspect, true);

    if (bFixHUD)
    {
        const char* signatures[] =
        {
            "D8 0D ? ? ? ? 8B 35 ? ? ? ? 85 F6 D9 1D",
            "D8 C1 D9 5C 24 28 DB 05 ? ? ? ? D9 47 2C",
            "D8 C1 D9 5C 24 28 D9 05 ? ? ? ? D8",
            "D8 C1 D9 5C 24 18 DD D8 DD D8 DD D8",
            "D8 44 24 20 D9 5C 24 60 0F 84 7A 03 00 00",
            "D8 C2 8B F0 D8 05 ? ? ? ? D8 0D ? ? ? ? D8",
        };
        bFixHUD = std::all_of(std::begin(signatures), std::end(signatures), [](const char* signature)
        {
            return hook::pattern(signature).size() == 1;
        }) && hook::pattern("D8 C1 D9 5C 24 14 DD D8").size() == 2;
        if (!bFixHUD)
            OutputDebugStringA("Gun Widescreen Fix: HUD signatures unavailable or ambiguous.\n");
    }
    if (bFixHUD)
    {
        Screen.fHudScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
        Screen.fHudOffset = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

        pattern = hook::pattern("D8 0D ? ? ? ? 8B 35 ? ? ? ? 85 F6 D9 1D");
        static auto hudScale = *pattern.count(1).get(0).get<float*>(2);
        ProtectedGameRef<float>{hudScale} = Screen.fHudScaleX; //0x6814CC
        GunDisplay::resolution = [](int, int) { ProtectedGameRef<float>{hudScale} = Screen.fHudScaleX; };

        struct TextHudHook
        {
            void operator()(injector::reg_pack& regs)
            {
                float fCurOffset = 0.0f;
                _asm
                {
                    fadd    st, st(1)
                    fstp    dword ptr[fCurOffset]
                }
                *reinterpret_cast<float*>(regs.esp + 0x28) = fCurOffset + Screen.fHudOffset;

            }
        };
        pattern = hook::pattern("D8 C1 D9 5C 24 28 DB 05 ? ? ? ? D9 47 2C");  //0x4DC226
        injector::MakeInline<TextHudHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6));

        pattern = hook::pattern("D8 C1 D9 5C 24 28 D9 05 ? ? ? ? D8");  //0x4F12DB
        injector::MakeInline<TextHudHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6));

        pattern = hook::pattern("D8 C1 D9 5C 24 18 DD D8 DD D8 DD D8");  //0x4F12E9
        struct HudHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                float fCurOffset = 0.0f;
                _asm
                {
                    fadd    st, st(1)
                    fstp    dword ptr[fCurOffset]
                }
                *reinterpret_cast<float*>(regs.esp + 0x18) = fCurOffset + Screen.fHudOffset;
            }
        }; injector::MakeInline<HudHook2>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));

        pattern = hook::pattern("D8 44 24 20 D9 5C 24 60 0F 84 7A 03 00 00");  //0x4F1BD7
        struct HudHook3
        {
            void operator()(injector::reg_pack& regs)
            {
                float temp1 = *(float*)(regs.esp + 0x20);
                float temp2 = 0.0f;
                _asm
                {
                    fadd    dword ptr[temp1]
                    fstp    dword ptr[temp2]
                }
                *reinterpret_cast<float*>(regs.esp + 0x60) = temp2;
                *reinterpret_cast<float*>(regs.esp + 0x5C) += Screen.fHudOffset;
                *reinterpret_cast<float*>(regs.esp + 0x6C) += Screen.fHudOffset;
                *reinterpret_cast<float*>(regs.esp + 0x7C) += Screen.fHudOffset;
                *reinterpret_cast<float*>(regs.esp + 0x8C) += Screen.fHudOffset;
            }
        }; injector::MakeInline<HudHook3>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(8));
        ProtectedGameRef<uint16_t>{pattern.count(1).get(0).get<uint16_t>(6)} = 0xC084; //test    al, al

        pattern = hook::pattern("D8 C1 D9 5C 24 14 DD D8");  //0x4F0D06
        struct HudHook4
        {
            void operator()(injector::reg_pack& regs)
            {
                float temp1 = 0.0f;
                _asm
                {
                    fadd    st, st(1)
                    fstp    dword ptr[temp1]
                }
                *reinterpret_cast<float*>(regs.esp + 0x14) = temp1 + Screen.fHudOffset;
                *reinterpret_cast<float*>(regs.esp + 0x18) += Screen.fHudOffset;
            }
        }; injector::MakeInline<HudHook4>(pattern.count(2).get(1).get<uint32_t>(0), pattern.count(2).get(1).get<uint32_t>(6));

        pattern = hook::pattern("D8 C2 8B F0 D8 05 ? ? ? ? D8 0D ? ? ? ? D8"); //main menu cursor fix
        ProtectedGameRef<uint8_t>{pattern.count(1).get(0).get<uint8_t>(2)} = 0xC0; //0x4FC837
        ProtectedGameRef<uint8_t>{pattern.count(1).get(0).get<uint8_t>(17)} = 0xC0; //0x4FC847
    }

    // The cutscene manager's list of playing cutscenes; gameplay FOV only while it is empty.
    pattern = hook::pattern("8B 3D ? ? ? ? 0F 94 C1 88 4C 24 0C 68 74 52 15 DA");
    if (pattern.size() == 1)
        Screen.CutsceneOwner = *pattern.get_first<uint8_t**>(2);
    else
        OutputDebugStringA("Gun Widescreen Fix: gameplay camera state unavailable.\n");
    pattern = hook::pattern("D9 9E C4 00 00 00 E8 ? ? ? ? D9 86 C4 00 00 00 5E 59 C3");
    if (pattern.size() == 1)
    {
        static bool bAdjustFOV = bFixFOV;
        // The native fstp has stored the angle and balanced the x87 stack. The following call rebuilds derived camera values.
        static auto FovHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
        {
            auto camera = reinterpret_cast<uint8_t*>(regs.esi);
            float fov = *reinterpret_cast<float*>(camera + 0xC4);
            static const auto& factor = FixSettings().Float("MAIN.FOVFactor");
            fov = GameplayFOV(fov, factor, camera);
            *reinterpret_cast<float*>(camera + 0xC4) = bAdjustFOV ? AdjustFOV(fov, Screen.fAspectRatio) : fov;
        });
        if (!FovHook)
            OutputDebugStringA("Gun Widescreen Fix: camera hook unavailable.\n");
    }
    else
    {
        OutputDebugStringA("Gun Widescreen Fix: camera signature unavailable or ambiguous.\n");
    }

    if (bSkipIntro)
    {
        pattern = hook::pattern("81 EC 14 01 00 00 A1 ? ? ? ? 53 56 57 33 DB 53 89 84 24 20 01 00 00 E8 ? ? ? ? 83 C4 04 8B F0 E8");
        if (pattern.size() == 1)
            shPlayMovie = safetyhook::create_inline(pattern.get_first(), PlayMovie);
        else
            OutputDebugStringA("Gun Widescreen Fix: movie signature unavailable or ambiguous.\n");
    }

    if (nGameSpeed)
    {
        pattern = hook::pattern("C7 05 ? ? ? ? 1E 00 00 00 A2 ? ? ? ? C3"); //0x52E7CA
        if (pattern.size() == 1)
            ProtectedGameRef<uint32_t>{pattern.get_first<uint32_t>(6)} = nGameSpeed;
        else
            OutputDebugStringA("Gun Widescreen Fix: timing signature unavailable or ambiguous.\n");
    }
    //43F8B8 - fps

    GunMenu::Init();

    auto updateHudScale = std::move(GunDisplay::resolution);
    GunDisplay::Init(Screen.Width, Screen.Height, [updateHudScale](int width, int height)
    {
        if (width <= 0 || height <= 0) return;
        Screen.Width = width;
        Screen.Height = height;
        Screen.fWidth = static_cast<float>(width);
        Screen.fHeight = static_cast<float>(height);
        Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
        Screen.Width43 = static_cast<int>(Screen.fHeight * (4.0f / 3.0f));
        Screen.fHudScaleX = Screen.fHeight / (480.0f * Screen.fWidth);
        Screen.fHudOffset = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) * 0.5f;
        *dword_6B759C_X = width;
        *dword_6B75A0_Y = height;
        if (updateHudScale) updateHudScale(width, height);
    });
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            WindowedModeWrapper::EnableDpiAwareness();
            FixSettings();
            asi_settings::LiveLink::Start();
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("6A 60 68 ? ? ? ? E8 ? ? ? ? BF 94 00 00 00"));
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
