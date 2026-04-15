module;

#include <stdafx.h>

export module BlackMagazineFix;

import ComVars;

uint32_t* dword_73645C;
uint8_t* pIsMagazineMode;
void* off_71AA4C;
void __declspec(naked) CmpShouldClearSurfaceDuring3DRender()
{
    _asm
    {
        mov eax, dword_73645C
        cmp dword ptr[eax], 1
        jne resume

        // pIsMagazineMode is set if the magazine view screen is open.
        mov eax, pIsMagazineMode
        cmp byte ptr[eax], 0

        resume:
        ret
    }
}

void __declspec(naked) MaybeClearSurfaceDuringBackgroundRender()
{
    _asm
    {
        call CmpShouldClearSurfaceDuring3DRender
        jz resume

        // In case 3D rendering won't clear the render surface, it must
        // be done during the 2D background rendering.
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
            CIniReader iniReader("");
            bool bBlackMagazineFix = iniReader.ReadInteger("MISC", "BlackMagazineFix", 0) != 0;

            if (bBlackMagazineFix)
            {
                auto pattern = hook::pattern("8B F8 A0 ? ? ? ? 84 C0 0F 85");
                pIsMagazineMode = *pattern.get_first<uint8_t*>(3);

                pattern = hook::pattern("83 3D ? ? ? ? 01 8B 77 58");
                dword_73645C = *pattern.get_first<uint32_t*>(2);
                injector::MakeRangedNOP(pattern.get_first(0), pattern.get_first(7));
                injector::MakeCALL(pattern.get_first(0), CmpShouldClearSurfaceDuring3DRender); // 4098A2

                pattern = hook::pattern("6A 00 BE ? ? ? ? 74 ? BE");
                off_71AA4C = *pattern.get_first<void*>(10);
                injector::MakeCALL(pattern.get_first(9), MaybeClearSurfaceDuringBackgroundRender); // 409CF2
            }
        };
    }
} BlackMagazineFix;