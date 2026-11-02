module;

#include <stdafx.h>

export module Frontend;

import ComVars;

// Cinematic bars only in their own state
SafetyHookInline shsub_A2A350 = {};
void __fastcall sub_A2A350(void* _this, void* edx, int a2, int a3, void* a4)
{
    if (pBarsOwner && (*reinterpret_cast<uint32_t*>(pBarsOwner + 0xCC) == 3))
        return shsub_A2A350.unsafe_fastcall(_this, edx, a2, a3, a4);
}

// The settings menu's aspect ratio option shows what it does with the fix
struct TextHookGamersEdition
{
    void operator()(injector::reg_pack& regs)
    {
        char* label = szAspectRatioLabel.get_ptr();
        if (strncmp(label, "16/9", 4) == 0)
        {
            std::memcpy(label, "Bars", 4);
        }
        else
        {
            if (strncmp(label, "4/3", 3) == 0)
            {
                std::memcpy(label, "Std", 4);
            }
        }

        regs.ecx = *(uint16_t*)(regs.edi + 6);
        regs.edx = *(uint16_t*)(regs.edi + 4);
    }
};

struct TextHookSignatureEdition
{
    void operator()(injector::reg_pack& regs)
    {
        char* label = szAspectRatioLabel.get_ptr();
        if (strncmp(label, "16/9", 4) == 0)
        {
            std::memcpy(label, "Auto", 4);
        }
        else if (strncmp(label, "4/3", 4) == 0)
        {
            std::memcpy(label, "Std", 4);
        }
        else if (strncmp(label, "4/3 ", 4) == 0)
        {
            std::memcpy(label, "Black Bands", sizeof("Black Bands"));
        }

        regs.ecx = *(uint16_t*)(regs.edi + 6);
        regs.edx = *(uint16_t*)(regs.edi + 4);
    }
};

class Frontend
{
public:
    Frontend()
    {
        WFP::onInitEvent() += []()
        {
            auto text_pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 50 55 56 E8 ? ? ? ? 0F B7 4F 06 0F B7 57 04"); //0x992C4C
            szAspectRatioLabel.SetAddress(*text_pattern.count(1).get(0).get<char*>(1));

            auto pattern = find_pattern("A1 ? ? ? ? 8B 88 00 08 00 00 81 C1 B4 68 06 00", "A1 ? ? ? ? 89 45 FC 8B 0D ? ? ? ? 89 0D");
            pBarsOwner.SetAddress(*pattern.get_first<uintptr_t*>(1)); //0xCBFBE0

            if (!Screen.bSignatureEdition)
            {
                pattern = hook::pattern("8B 41 10 6A 00");
                shsub_A2A350 = safetyhook::create_inline(pattern.get_first(), sub_A2A350);
                injector::MakeInline<TextHookGamersEdition>(text_pattern.get_first(18), text_pattern.get_first(18 + 8));
            }
            else
            {
                injector::MakeInline<TextHookSignatureEdition>(text_pattern.get_first(18), text_pattern.get_first(18 + 8));
            }
        };
    }
} Frontend;
