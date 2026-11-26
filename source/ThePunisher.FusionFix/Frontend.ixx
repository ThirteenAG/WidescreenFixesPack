module;

#include <stdafx.h>
#include <bcrypt.h>
#include <fstream>
#include "FixSettings.hpp"

#pragma comment(lib, "bcrypt.lib")

export module Frontend;

import ComVars;
import MenuLayout;

namespace
{
    std::atomic_bool FillMovies{ true };
    const uint32_t* TheatricalVideo{};
    const uint32_t* (WINAPI* OpenBink)(const char*, unsigned){};

    bool IsVerifiedTheatricalFile(const char* filename)
    {
        const std::string_view path(filename);
        const auto basename = path.substr(path.find_last_of("\\/") + 1);
        if (basename.size() != sizeof("punmovie.bik") - 1 || _stricmp(basename.data(), "punmovie.bik") != 0)
            return false;

        constexpr int64_t fileSize = 18392720;
        std::ifstream stream(filename, std::ios::binary | std::ios::ate);
        if (!stream || stream.tellg() != fileSize)
            return false;
        stream.seekg(0);

        // Every frame of this asset has sixteen completely black lines at each edge.
        constexpr std::array<UCHAR, 32> expected{
            0x48, 0xE3, 0x4D, 0x63, 0x6F, 0xEB, 0x8A, 0x12, 0xC9, 0xB4, 0x9B, 0xB8, 0x9C, 0xD8, 0x90, 0x7D,
            0xF6, 0xE2, 0x6F, 0x60, 0x9E, 0x56, 0x50, 0x42, 0xE0, 0xB7, 0xFF, 0x77, 0x15, 0xDF, 0x5E, 0xF8
        };
        BCRYPT_ALG_HANDLE algorithm{};
        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
            return false;
        BCRYPT_HASH_HANDLE hash{};
        bool valid = BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) >= 0;
        std::array<UCHAR, 65536> bytes;
        while (valid && stream)
        {
            stream.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
            if (const auto count = stream.gcount())
                valid = BCryptHashData(hash, bytes.data(), static_cast<ULONG>(count), 0) >= 0;
        }
        std::array<UCHAR, 32> actual{};
        valid = valid && stream.eof() && BCryptFinishHash(hash, actual.data(), static_cast<ULONG>(actual.size()), 0) >= 0;
        if (hash)
            BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return valid && actual == expected;
    }

    const uint32_t* WINAPI OpenMovie(const char* filename, unsigned flags)
    {
        const auto* video = OpenBink(filename, flags);
        TheatricalVideo = video && filename && video[0] == 640 && video[1] == 480 && video[2] == 2754 && IsVerifiedTheatricalFile(filename)
            ? video : nullptr;
        return video;
    }
}

// Loading screens are drawn from a timer thread while the game thread can draw too. Each 2D draw function locks
// the one shared vertex buffer, fills it and draws it, so two threads at once crash on its emptied pointer:
// these functions (and the buffer's re-creation) run one at a time.
namespace DrawSerialization
{
    std::recursive_mutex mutex;
    std::array<SafetyHookInline, 7> hooks;

    template<size_t I>
    int __cdecl Draw(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13, int a14)
    {
        const std::lock_guard lock(mutex);
        return hooks[I].ccall<int>(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
    }

    template<size_t... I>
    void Install(const std::array<const char*, sizeof...(I)>& patterns, std::index_sequence<I...>)
    {
        ((hooks[I] = safetyhook::create_inline(hook::pattern(patterns[I]).get_first(), Draw<I>)), ...);
    }

    void Install()
    {
        // Both threads also call the Direct3D device: let the runtime serialize those calls
        auto create = hook::pattern("68 ? ? ? ? 8D 74 24 30 56 6A 40 51 8B 4C 24 24 6A 01 51 50 FF 52 3C");
        injector::WriteMemory<uint8_t>(create.get_first(11), 0x40 | 0x04, true); // hardware vertex processing | multithreaded

        // The callers clean the stack, so the extra arguments passed on are never read
        Install({
            "53 8B 5C 24 1C F6 C3 08 55 75 1D 8B 44 24 0C 8B 4C 24 1C 8B 54 24 18 6A",
            "8B 4C 24 10 B8 01 00 00 00 83 EC 18 3B C8 0F 8C B8 02 00 00 39 44 24 2C",
            "83 EC 08 53 55 56 8B 74 24 18 57 8D 44 24 10 50 8D 4C 24 18 51 56 E8",
            "51 8B 44 24 08 8B 4C 24 18 8B 54 24 14 53 56 6A FF 6A FF 6A FF 50 51 52",
            "56 8B 74 24 10 85 F6 75 08 89 35 ? ? ? ? 5E C3 F6 44 24 1C 08 75 1D",
            "51 A1 ? ? ? ? 55 8B 6C 24 0C 56 03 C5 33 F6 3D 28 23 00 00 7C 31 E8",
            "A1 ? ? ? ? 8B 08 53 33 DB 68 ? ? ? ? 53 53 68 08 02 00 00 68 00",
        }, std::make_index_sequence<7>{});
    }
}

char* memstr(char* block, const char* pattern, size_t bsize)
{
    const size_t length = strlen(pattern);
    if (length > bsize)
        return nullptr;
    auto result = std::search(block, block + bsize, pattern, pattern + length);
    return result == block + bsize ? nullptr : result;
}



export void InitFrontend()
{
    DrawSerialization::Install();
    ObservePunisherSetting("MAIN.FMVWidescreenMode", []()
    {
        FillMovies.store(PunisherNumericSetting("MAIN.FMVWidescreenMode", 1) != 0, std::memory_order_relaxed);
    });
    auto imports = IATHook::Replace(GetModuleHandleW(nullptr), "binkw32.dll", std::forward_as_tuple("_BinkOpen@8", OpenMovie));
    OpenBink = reinterpret_cast<decltype(OpenBink)>(imports.at("_BinkOpen@8").get());

    static injector::hook_back<void(__cdecl*)(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int8_t, int8_t, int32_t, int8_t, int8_t)> hb_519CA0;
    hook::pattern pattern;
    //Menu
    pattern = hook::pattern("89 0D ? ? ? ? 68 ? ? ? ? 8D"); //0x477E26
    dword_73DF80.SetAddress(*pattern.get_first<uint32_t*>(2));
    static auto dword_73DFA4 = *hook::get_pattern<uint32_t>("89 81 ? ? ? ? 8D 0C 24 E8 ? ? ? ? 8B 15", 2);
    static auto dword_73DFA8 = *hook::get_pattern<uint32_t>("89 82 ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 6B C9 2C", 2);
    static auto dword_73DFAC = *hook::get_pattern<uint32_t>("89 81 ? ? ? ? 8D 0C 24 E8 ? ? ? ? 8B 0D", 2);
    static auto dword_73DFB0 = *hook::get_pattern<uint32_t>("8D 4C 24 04 89 82 ? ? ? ? E8 ? ? ? ? 84 C0", 6);
    struct MenuHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto i = dword_73DF80.get() * 44;
            auto x = *(int32_t*)(dword_73DFA4 + i);
            auto y = *(int32_t*)(dword_73DFA8 + i);
            auto w = *(int32_t*)(dword_73DFAC + i);
            auto h = *(int32_t*)&regs.eax;
            if (x == 0 && w == Screen.PresetWidth && h == (Screen.PresetHeight - y))
            {
                //*(int32_t*)(dword_73DFA4 + i) = x;
                //*(int32_t*)(dword_73DFA8 + i) = y;
                *reinterpret_cast<int32_t*>(dword_73DFAC + i) = Screen.Width; //makes weapons overlay not being cut off by preset
                *(int32_t*)&regs.eax = Screen.Height;
            }
            dword_73DF80 = regs.ecx;
        }
    }; injector::MakeInline<MenuHook>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern("A2 ? ? ? ? 8A 44 24 10 88 0D"); //0x510B6C
    gTexColor.SetAddress(*pattern.get_first<uint32_t*>(1));
    static auto GetTexColor = []() -> uint32_t
    {
        return gTexColor.get();
    };

    static auto SetTexColor = [](uint32_t argb)
    {
        gTexColor = argb;
    };

    pattern = hook::pattern("57 8B 7C 24 08 57 E8 ? ? ? ? 83 C4 04 85 C0 7D 7E A1 ? ? ? ? 55 56 8B F0 6B F6 2C"); //50C750
    static auto GetTexID = (int32_t(__cdecl*)(const char* FileName)) pattern.get_first(0);


    auto DrawHook = [](int32_t texID, int32_t x, int32_t y, int32_t w, int32_t h, int32_t a6, int32_t a7, int32_t a8, int32_t a9, int8_t a10, int8_t a11, int32_t a12, int8_t a13, int8_t a14)
    {
        const int left = std::max(0, static_cast<int>(Screen.fHudOffsetReal));
        const auto drawBorder = [&](int bx, int by, int bw, int bh, int sourceWidth, int sourceHeight)
        {
            if (bw > 0 && bh > 0)
                hb_519CA0.fun(texID, bx, by, bw, bh, a6, a7, sourceWidth, sourceHeight, a10, a11, a12, a13, a14);
        };
        if (x == 0 && y == 0 && w == Screen.Width && h == Screen.Height)
        {
            w = std::min(Screen.Width, Screen.Width43);
            h = std::min(Screen.Height, static_cast<int>(std::lround(Screen.Width * 0.75f)));
            x = (Screen.Width - w) / 2;
            y = (Screen.Height - h) / 2;
            const auto originalColour = GetTexColor();
            SetTexColor(0xFF000000);
            drawBorder(0, 0, x, Screen.Height, a8, a9);
            drawBorder(x + w, 0, Screen.Width - x - w, Screen.Height, a8, a9);
            drawBorder(0, 0, Screen.Width, y, a8, a9);
            drawBorder(0, y + h, Screen.Width, Screen.Height - y - h, a8, a9);
            SetTexColor(originalColour);
        }
        else if (left > 0)
        {
            const bool leftTile = x == 0 && ((y == 0 && w == 640 && h == 360) || ((y == 0 || y == 540) && w == 960 && h == 540));
            const bool rightTile = x == 960 && (y == 0 || y == 540) && w == 960 && h == 540;
            if (leftTile || rightTile)
            {
                if (leftTile)
                    x += left;
                w = std::max(0, w - left);
                const auto originalColour = GetTexColor();
                SetTexColor(0xFF000000);
                drawBorder(0, 0, left, Screen.Height, rightTile ? 1 : a8, rightTile ? 1 : a9);
                drawBorder(Screen.Width - left, 0, left, Screen.Height, rightTile ? 1 : a8, rightTile ? 1 : a9);
                SetTexColor(originalColour);
            }
        }
        return hb_519CA0.fun(texID, x, y, w, h, a6, a7, a8, a9, a10, a11, a12, a13, a14);
    };
    pattern = hook::pattern("8B 54 24 34 51 52 50 E8 ? ? ? ? 83 C4 38 C3"); //0x510C68
    hb_519CA0.fun = injector::MakeCALL(pattern.get_first(7), static_cast<void(__cdecl*)(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int8_t, int8_t, int32_t, int8_t, int8_t)>(DrawHook), true).get();

    // This call belongs only to the movie player; menu textures keep their framing.
    auto movieDraw = hook::pattern("8B 15 ? ? ? ? 50 6A 00 6A 00 52 E8 ? ? ? ? 6A 00 E8 ? ? ? ? 83 C4 3C");
    auto moviePlayer = hook::pattern("A1 ? ? ? ? 83 EC 08 85 C0 0F 84 ? ? ? ? 8D 44 24 0C 50 E8");
    MovieDecoder.SetAddress(*moviePlayer.get_first<uint32_t**>(1));
    auto drawMovie = [](int texture, int x, int y, int width, int height,
        int sourceX, int sourceY, int sourceWidth, int sourceHeight,
        char flipX, char flipY, int stage, int flags)
    {
        using Draw = int(__cdecl*)(int, int, int, int, int, int, int,
            int, int, char, char, int, int, char);
        const auto draw = reinterpret_cast<Draw>(hb_519CA0.fun);
        if (PunMenuLayout::Drawing)
        {
            return draw(texture, x, y, width, height, sourceX, sourceY,
                sourceWidth, sourceHeight, flipX, flipY, stage, flags, 0);
        }
        const auto video = MovieDecoder.get();
        if (!video || !video[0] || !video[1] || Screen.Width <= 0 || Screen.Height <= 0)
            return draw(texture, x, y, width, height, sourceX, sourceY,
                sourceWidth, sourceHeight, flipX, flipY, stage, flags, 0);

        const bool fill = FillMovies.load(std::memory_order_relaxed);
        const bool crop = fill && video == TheatricalVideo && sourceY == 0 && sourceHeight == static_cast<int>(video[1]);
        if (crop)
        {
            sourceY += 16;
            sourceHeight -= 32;
        }
        const float areaWidth = fill ? Screen.fWidth : std::min(Screen.fWidth, Screen.fHeight * (4.0f / 3.0f));
        const float movieHeight = static_cast<float>(video[1]) - (crop ? 32.0f : 0.0f);
        const float scale = std::min(areaWidth / static_cast<float>(video[0]), Screen.fHeight / movieHeight);
        width = std::max(1, static_cast<int>(std::lround(video[0] * scale)));
        height = std::max(1, static_cast<int>(std::lround(movieHeight * scale)));
        x = (Screen.Width - width) / 2;
        y = (Screen.Height - height) / 2;

        const auto colour = GetTexColor();
        SetTexColor(0xFF000000);
        if (x > 0)
        {
            draw(texture, 0, 0, x, Screen.Height, sourceX, sourceY,
                sourceWidth, sourceHeight, flipX, flipY, stage, flags, 0);
            draw(texture, x + width, 0, Screen.Width - x - width, Screen.Height,
                sourceX, sourceY, sourceWidth, sourceHeight, flipX, flipY, stage, flags, 0);
        }
        if (y > 0)
        {
            draw(texture, 0, 0, Screen.Width, y, sourceX, sourceY,
                sourceWidth, sourceHeight, flipX, flipY, stage, flags, 0);
            draw(texture, 0, y + height, Screen.Width, Screen.Height - y - height,
                sourceX, sourceY, sourceWidth, sourceHeight, flipX, flipY, stage, flags, 0);
        }
        SetTexColor(colour);
        return draw(texture, x, y, width, height, sourceX, sourceY,
            sourceWidth, sourceHeight, flipX, flipY, stage, flags, 0);
    };
    injector::MakeCALL(movieDraw.get_first(12), +drawMovie, true);

    //struct WeaponsOverlayHook
    //{
    //    void operator()(injector::reg_pack& regs)
    //    {
    //        regs.eax = *(uint32_t*)(regs.esi + regs.edi + 0x230);
    //        bWeapOverlay = true;
    //    }
    //}; injector::MakeInline<WeaponsOverlayHook>(0x47D2DA, 0x47D2DA+7);

    auto weaponFirst = hook::pattern("6A 00 6A 00 B9 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 6A FF 68 ? ? ? ? E8 ? ? ? ? 83 C4 08 A3 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C3 CC CC CC CC CC CC CC CC 6A 01").get_first<uint8_t>();
    auto weaponSecond = hook::pattern("6A 00 6A 00 B9 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 6A FF 68 ? ? ? ? E8 ? ? ? ? 83 C4 08 A3 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C3 CC CC CC CC CC CC CC CC 83 EC 08").get_first<uint8_t>();
    injector::WriteMemory<uint8_t>(weaponFirst + 1, 1, true);
    injector::WriteMemory<uint8_t>(weaponSecond + 1, 1, true);

    pattern = hook::pattern("75 ? E8 ? ? ? ? 8B 4C 24 28 89 01 8B CB E8 ? ? ? ? 89 45 00"); //0x5266C4
    struct WeaponsOverlayHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            **(int32_t**)(regs.esp + 0x28) = static_cast<int32_t>(static_cast<float>(**(int32_t**)(regs.esp + 0x28)) * (Screen.fWidth / (480.0f * Screen.fAspectRatio)));
            **(int32_t**)(regs.esp + 0x2C) = static_cast<int32_t>(static_cast<float>(**(int32_t**)(regs.esp + 0x2C)) * Screen.fHeight / 448.0f);

            **(int32_t**)(regs.esp + 0x28) = **(int32_t**)(regs.esp + 0x28) + static_cast<int32_t>(Screen.fHudOffsetReal);
        }
    };
    injector::MakeNOP(pattern.get_first(0), 2, true); //weapons overlay preset
    injector::MakeInline<WeaponsOverlayHook2>(pattern.get_first(23), pattern.get_first(23 + 14)); //0x5266DB, 0x5266E9

    pattern = hook::pattern("D8 0D ? ? ? ? D9 5C 24 18 E8 ? ? ? ? 89"); //0x47D10E
    injector::WriteMemory<float*>(pattern.get_first<float*>(2), &Screen.fHUDScaleX, true); //weapons overlay scale
    //pattern = hook::pattern("D8 0D ? ? ? ? E8 ? ? ? ? 8B 15 ? ? ? ? 50 6A"); //0x557FC8
    //ProtectedGameRef<float*>{pattern.get_first<float*>(2)} = &Screen.fHUDScaleX; //videos scale

    pattern = hook::pattern("8B 8F 8C 00 00 00 50 53 53 51"); //0x48ADB2
    struct BackgroundHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = *(uint32_t*)(regs.edi + 0x8C);
            auto str = (char*)((*(uintptr_t*)(regs.edi)) + 0x30);
            static const char* menuBackgrounds[] = { "options_bg", "bf_detail_bg", "pc_controls_menu_bg", "gs_menu_bg", "av_menu_bg" };

            for (size_t i = 0; i < std::size(menuBackgrounds); ++i)
            {
                if (memstr(str, menuBackgrounds[i], 0x60) != NULL)
                {
                    regs.eax += 1;
                    break;
                }
            }
        }
    }; injector::MakeInline<BackgroundHook>(pattern.get_first(0), pattern.get_first(6));

}
