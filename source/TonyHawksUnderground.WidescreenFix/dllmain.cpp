#include "stdafx.h"
#include <random>

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    int32_t Width43;
    float fAspectRatio;
    float fAspectRatioDiff;
    float fFieldOfView;
    float fHUDScaleX;
    float fHudOffset;
    float fHudOffsetReal;
} Screen;

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
    bool bRandomSongOrderFix = iniReader.ReadInteger("MAIN", "RandomSongOrderFix", 1) != 0;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fAspectRatioDiff = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

    //addresses is not based on drm free exe
    //Resolution
    auto pattern = hook::pattern("6A 01 6A 01 ? 8B 15");
    static int32_t* dword_6D3608 = *pattern.get_first<int32_t*>(7);
    static int32_t* dword_6D360C = *pattern.get_first<int32_t*>(-4);
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_6D3608 = Screen.Width;
            *dword_6D360C = Screen.Height;
        }
    };
    pattern = hook::pattern(pattern_str(0xA3, to_bytes(dword_6D3608)));
    for (size_t i = 0; i < pattern.size(); ++i)
        injector::MakeInline<SetResHook>(pattern.get(i).get<uint32_t>(0));

    pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_6D360C)));
    for (size_t i = 0; i < pattern.size(); ++i)
        injector::MakeInline<SetResHook>(pattern.get(i).get<uint32_t>(0), pattern.get(i).get<uint32_t>(6));

    pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_6D3608)));
    if (!pattern.count_hint(1).empty())
        injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern(pattern_str(0xA3, to_bytes(dword_6D360C)));
    if (!pattern.count_hint(1).empty())
        injector::MakeInline<SetResHook>(pattern.get_first(0));

    //Aspect Ratio
    pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 6A 00 68 ? ? ? ? E8 ? ? ? ? D9 5C 24 08");
    if (!pattern.count_hint(1).empty())
    {
        injector::WriteMemory<float>(pattern.get_first(1), Screen.fAspectRatio, true);

        pattern = hook::pattern("E8 ? ? ? ? D9 04 24 D8 74 24 04 DE C9");
        injector::WriteMemory(injector::GetBranchDestination(pattern.get_first()).as_int() + 2, &Screen.fAspectRatio, true);
    }
    else
    {
        pattern = hook::pattern("D9 05 ? ? ? ? 51 D9 1C 24 E8 ? ? ? ? 6A 00");
        injector::WriteMemory<float>(*pattern.get_first<float*>(2), Screen.fAspectRatio, true);
    }

    //FOV
    pattern = hook::pattern("0D ? ? ? ? D9 9E ? ? ? ? 5E 59 C3");
    struct FovHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float fov = 0.0f;
            _asm {fstp dword ptr[fov]}
            *(float*)(regs.esi + 0xA4) = AdjustFOV(fov, Screen.fAspectRatio);
        }
    }; injector::MakeInline<FovHook>(pattern.get_first(5), pattern.get_first(5 + 6));

    //HUD
    if (bFixHUD)
    {
        static int32_t nHudOffset = static_cast<int32_t>(Screen.fHudOffsetReal);
        static int32_t* dword_72DFC4;
        struct SetOffsetHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *dword_72DFC4 = nHudOffset;
            }
        };

        pattern = hook::pattern("D8 0D ? ? ? ? 03 C2 C1 F8 02");
        if (!pattern.count_hint(1).empty())
        {
            injector::WriteMemory<float>(*pattern.get_first<float*>(2), Screen.fHUDScaleX, true);
            dword_72DFC4 = *hook::get_pattern<int32_t*>("DA 05 ? ? ? ? 0F BF 88", 2);
        }
        else
        {
            Screen.fHUDScaleX = 1.0f / Screen.fHUDScaleX;
            pattern = hook::pattern("DC 0D ? ? ? ? DE FB D9");
            injector::WriteMemory<double>(*pattern.get_first<double*>(2), Screen.fHUDScaleX, true);

            dword_72DFC4 = *hook::get_pattern<int32_t*>("DA 05 ? ? ? ? 89 4C 24 20 89 54 24 18", 2);
        }

        pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_72DFC4)));
        for (size_t i = 0; i < pattern.size(); ++i)
            injector::MakeInline<SetOffsetHook>(pattern.get(i).get<uint32_t>(0), pattern.get(i).get<uint32_t>(6));
    }

    if (bRandomSongOrderFix)
    {
        pattern = hook::pattern("83 C4 08 ? D0 07 00 00"); //0x424BDD or 423CDB
        auto rpattern = hook::range_pattern((uintptr_t)pattern.get_first(3), (uintptr_t)pattern.get_first(110), "75 ? A1");
        static auto dword_69B718 = *rpattern.get_first<int32_t*>(3);
        static auto dword_681D14 = *hook::range_pattern((uintptr_t)pattern.get_first(3), (uintptr_t)pattern.get_first(60), "A1 ? ? ? ?").get_first<int32_t*>(1);
        struct RandomHook
        {
            void operator()(injector::reg_pack& regs)
            {
                auto num_songs = *dword_681D14;
                int32_t* sp_xbox_randomized_songs = dword_69B718;

                std::vector<int32_t> songs;
                songs.assign(std::addressof(sp_xbox_randomized_songs[0]), std::addressof(sp_xbox_randomized_songs[num_songs]));
                std::mt19937 r{ std::random_device{}() };
                std::shuffle(std::begin(songs), std::end(songs), r);

                std::copy(songs.begin(), songs.end(), sp_xbox_randomized_songs);
            }
        }; injector::MakeInline<RandomHook>(pattern.get_first(3), rpattern.get_first(2));
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("6A 01 6A 01 ? 8B 15"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {

    }
    return TRUE;
}