module;

#include <stdafx.h>

export module BlackMagazineFix;

import ComVars;

// Magazine covers (the magazine view of the front end) had a black background. While the 3D scene is drawn, its
// viewport is cleared whenever the world is rendered (dword_73645C == 1), which wipes the background drawn before
// it. In the magazine view the 3D pass doesn't clear; the 2D background viewport clears instead. Outside the
// magazine view nothing changes.

uint32_t* dword_73645C = nullptr;
uint8_t* pIsMagazineMode = nullptr;
void* off_71AA4C = nullptr;

// Replaces "cmp dword_73645C, 1" before SetViewport(clear): ZF set = clear the 3D viewport
void __declspec(naked) CmpShouldClearSurfaceDuring3DRender()
{
    _asm
    {
        mov eax, dword_73645C
        cmp dword ptr[eax], 1
        jne resume

        mov eax, pIsMagazineMode
        cmp byte ptr[eax], 0

        resume:
        ret
    }
}

// Replaces "mov esi, offset dword_71AA4C" after "push 0" (the clear flag) for the background's SetViewport
void __declspec(naked) MaybeClearSurfaceDuringBackgroundRender()
{
    _asm
    {
        mov esi, pIsMagazineMode
        cmp byte ptr[esi], 0
        jz resume

        mov dword ptr[esp + 4], 1

        resume:
        mov esi, off_71AA4C
        ret
    }
}

class BlackMagazineFix
{
public:
    BlackMagazineFix()
    {
        WFP::onInitEvent() += []()
        {
            auto magazine = hook::pattern("8B F8 A0 ? ? ? ? 84 C0 0F 85");
            auto clear3D = hook::pattern("83 3D ? ? ? ? 01 8B 77 58");
            auto background = hook::pattern("BE ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 83 C4 ? 85 C0");
            if (magazine.empty() || clear3D.empty() || background.empty())
                return;

            pIsMagazineMode = *magazine.get_first<uint8_t*>(3);

            dword_73645C = *clear3D.get_first<uint32_t*>(2);
            injector::MakeRangedNOP(clear3D.get_first(0), clear3D.get_first(7));
            injector::MakeCALL(clear3D.get_first(0), CmpShouldClearSurfaceDuring3DRender);

            off_71AA4C = *background.get_first<void*>(1);
            injector::MakeCALL(background.get_first(0), MaybeClearSurfaceDuringBackgroundRender);
        };
    }
} BlackMagazineFix;
