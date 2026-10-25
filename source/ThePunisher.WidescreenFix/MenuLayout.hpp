#pragma once

namespace PunMenuLayout
{
    inline SafetyHookInline WidthHook, MainDrawHook;
    inline thread_local bool Drawing = false;
    inline int Width{}, Height{}, CanvasWidth{}, Left{};
    inline void(__cdecl* Viewport)(int, int, int, int){};
    inline void(__cdecl* GetViewport)(int*, int*, int*, int*){};
    inline void(__cdecl* Colour)(int, int, int, int){};
    inline int(__cdecl* Fill)(int, int, int, int, int, int){};

    inline int __cdecl GetWidth()
    {
        return Drawing ? CanvasWidth : WidthHook.ccall<int>();
    }

    struct Scope
    {
        int x{}, y{}, width{}, height{};
        bool active{}, clipped{};
        explicit Scope(uintptr_t root)
        {
            if (!WidthHook || Drawing) return;
            Drawing = active = true;
            if (CanvasWidth >= Width) return;
            // Keep artwork, native clipping and mouse hit tests on the same canvas.
            auto* widget = reinterpret_cast<int*>(root);
            widget[11] = Left;
            widget[13] = CanvasWidth;
            GetViewport(&x, &y, &width, &height);
            Viewport(0, 0, Width, Height);
            Colour(0, 0, 0, 255);
            if (Left > 0)
            {
                Fill(0, 0, Left, Height, 96, 0);
            }
            if (Left + CanvasWidth < Width)
            {
                Fill(Left + CanvasWidth, 0, Width - Left - CanvasWidth, Height, 96, 0);
            }
            Viewport(Left, 0, CanvasWidth, Height);
            clipped = true;
        }
        ~Scope()
        {
            if (!active) return;
            Drawing = false;
            if (clipped)
            {
                Viewport(x, y, width, height);
            }
        }
    };

    inline uintptr_t __fastcall DrawMain(void* root, void*)
    {
        Scope layout(reinterpret_cast<uintptr_t>(root));
        return MainDrawHook.fastcall<uintptr_t>(root, nullptr);
    }

    inline void Init(int width, int height)
    {
        if (!PunMenu::SupportedImage()) return;
        auto main = hook::pattern("83 EC 0C E8 ? ? ? ? B8 ? ? ? ? 8D 48 01 8A 10 40 84 D2 75 F9 53 56 57");
        auto screen = hook::pattern("A1 50 32 91 00 C3 CC CC CC CC");
        auto viewport = hook::pattern("8B 4C 24 0C 8B 54 24 04 8B 44 24 10 83 EC 18 53 8B 5C 24 24 55 56");
        auto getViewport = hook::pattern("8B 0D ? ? ? ? 8B 44 24 04 8B 54 24 08 89 08 A1 ? ? ? ? 8B 4C 24 0C 89 02");
        auto colour = hook::pattern("8A 44 24 04 8A 4C 24 08 8A 54 24 0C A2 ? ? ? ? 8A 44 24");
        auto fill = hook::pattern("8B 4C 24 0C 8B 44 24 04 8B 54 24 08 81 EC 84 00 00 00 85 C9 53 55 56 57");
        if (main.size() != 1 || screen.size() != 1 || viewport.size() != 1 || getViewport.size() != 1 || colour.size() != 1 || fill.size() != 1)
        {
            OutputDebugStringA("The Punisher Widescreen Fix: menu canvas signatures unavailable.\n");
            return;
        }
        Width = width;
        Height = height;
        CanvasWidth = std::min(width, static_cast<int>(std::lround(height * (4.0 / 3.0))));
        Left = (width - CanvasWidth) / 2;
        Viewport = reinterpret_cast<decltype(Viewport)>(viewport.get_first());
        GetViewport = reinterpret_cast<decltype(GetViewport)>(getViewport.get_first());
        Colour = reinterpret_cast<decltype(Colour)>(colour.get_first());
        Fill = reinterpret_cast<decltype(Fill)>(fill.get_first());
        WidthHook = safetyhook::create_inline(screen.get_first(), GetWidth);
        if (WidthHook)
        {
            MainDrawHook = safetyhook::create_inline(main.get_first(), DrawMain);
            if (!MainDrawHook) WidthHook.reset();
        }
    }
}
