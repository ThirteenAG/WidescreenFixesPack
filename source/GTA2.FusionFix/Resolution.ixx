module;

#include <stdafx.h>

export module Resolution;

import ComVars;
import Window;

// The configured resolution in the video mode setup
export namespace GTA2Resolution
{
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            const auto [width, height] = GTA2Window::OutputResolution();
            regs.eax = height;
            dword_673578 = width;
            dword_6732E8 = height;
            dword_6732E4 = width;
            dword_6732E0 = height;
        }
    };

    void Install()
    {
        window_width.SetAddress(*hook::pattern("8B 3D ? ? ? ? 8B 4C 24 1C").get_first<uint32_t*>(2));
        window_height.SetAddress(*hook::pattern("8B 0D ? ? ? ? 2B F8").get_first<uint32_t*>(2));

        // Res change
        if (nResX && nResY)
        {
            auto pattern = hook::pattern("8B 2D ? ? ? ? 56 8B 35 ? ? ? ? 57 8B 3D"); //0x4CB29F
            dword_6732E0.SetAddress(*pattern.get_first<uint32_t*>(2));
            pattern = hook::pattern("89 0D ? ? ? ? A3 ? ? ? ? 89 0D ? ? ? ? EB 5F"); //0x4CB2D5
            dword_673578.SetAddress(*pattern.get_first<uint32_t*>(2));
            dword_6732E8.SetAddress(*pattern.get_first<uint32_t*>(7));
            dword_6732E4.SetAddress(*pattern.get_first<uint32_t*>(13));
            injector::MakeInline<SetResHook>(pattern.get_first(-12), pattern.get_first(17));

            pattern = hook::pattern("74 49 A3 ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? 84 C0");
            injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(0), 0xEB, true); //0x4CC61E
            injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(24), 0xEB, true); //0x4CC636
            pattern = hook::pattern("74 2B A1 ? ? ? ? B9 ? ? ? ? 50 68");
            injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(0), 0xEB, true); //0x4CB692
            pattern = hook::pattern("B8 ? ? ? ? 3B C8 74 2B 6A 10 68");
            fullscreenWidth.SetAddress(pattern.get_first<int>(1)); fullscreenHeight.SetAddress(pattern.get_first<int>(12));
            fullscreenWidth = nResX; //0x4CB59C + 1
            injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(10), 32, true);
            fullscreenHeight = nResY; //0x4CB5A7 + 1
            pattern = hook::pattern("6A ? 50 51 52 32 DB"); //0x4CB583
            //injector::WriteMemory<uint8_t>(pattern.get_first(1), 32, true); // causes green menu
            injector::MakeNOP(pattern.get_first(32), 2, true); //0x4CB5A3
        }
    }
}
