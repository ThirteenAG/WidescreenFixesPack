module;

#include <stdafx.h>

export module Frontend;

import ComVars;
import MenuLayout;

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
        if (x == 0 && y == 0 && w == Screen.Width && h == Screen.Height)
        {
            x = static_cast<int32_t>(Screen.fHudOffsetReal);
            //y = top;
            w = static_cast<int32_t>(Screen.fWidth - Screen.fHudOffsetReal - Screen.fHudOffsetReal);
            //h = Screen.PresetHeight;

            auto og = GetTexColor();
            SetTexColor(0xFF000000);
            hb_519CA0.fun(texID, 0, 0, (int)Screen.fHudOffsetReal, Screen.Height, a6, a7, a8, a9, a10, a11, a12, a13, a14);
            hb_519CA0.fun(texID, Screen.Width - (int32_t)Screen.fHudOffsetReal, 0, Screen.Width, Screen.Height, a6, a7, a8, a9, a10, a11, a12, a13, a14);
            SetTexColor(og);
        }
        else
        {
            if ((x == 0 && y == 0 && w == 640 && h == 360) || (x == 0 && y == 0 && w == 960 && h == 540) || (x == 0 && y == 540 && w == 960 && h == 540))
            {
                x += static_cast<int32_t>(Screen.fHudOffsetReal);
                w -= static_cast<int32_t>(Screen.fHudOffsetReal);

                auto og = GetTexColor();
                SetTexColor(0xFF000000);
                hb_519CA0.fun(texID, 0, 0, (int)Screen.fHudOffsetReal, Screen.Height, a6, a7, a8, a9, a10, a11, a12, a13, a14);
                hb_519CA0.fun(texID, Screen.Width - (int32_t)Screen.fHudOffsetReal, 0, Screen.Width, Screen.Height, a6, a7, a8, a9, a10, a11, a12, a13, a14);
                SetTexColor(og);
            }
            else
            {
                if ((x == 960 && y == 0 && w == 960 && h == 540) || (x == 960 && y == 540 && w == 960 && h == 540))
                {
                    w -= static_cast<int32_t>(Screen.fHudOffsetReal);

                    auto og = GetTexColor();
                    SetTexColor(0xFF000000);
                    hb_519CA0.fun(texID, 0, 0, (int)Screen.fHudOffsetReal, Screen.Height, a6, a7, 1, 1, a10, a11, a12, a13, a14);
                    hb_519CA0.fun(texID, Screen.Width - (int32_t)Screen.fHudOffsetReal, 0, Screen.Width, Screen.Height, a6, a7, 1, 1, a10, a11, a12, a13, a14);
                    SetTexColor(og);
                }
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

        // Both modes show the whole movie at its own aspect
        float scaleX = Screen.fWidth / static_cast<float>(video[0]);
        float scaleY = Screen.fHeight / static_cast<float>(video[1]);
        scaleX = scaleY = std::min(scaleX, scaleY);
        width = std::max(1, static_cast<int>(std::lround(video[0] * scaleX)));
        height = std::max(1, static_cast<int>(std::lround(video[1] * scaleY)));
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
