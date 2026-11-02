module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Movies;

import ComVars;

// Startup logo videos: opening them fails, which the game treats as a finished video.
SafetyHookInline shBinkOpen;
void* __stdcall BinkOpenHook(const char* name, uint32_t flags)
{
    constexpr uint32_t BINKFILEHANDLE = 0x00800000;
    if (name && !(flags & BINKFILEHANDLE))
    {
        std::string_view file{ name };
        if (auto slash = file.find_last_of("\\/"); slash != file.npos)
            file.remove_prefix(slash + 1);
        file = file.substr(0, file.find('.'));
        for (auto logo : { "mov1", "mov2", "mov3", "mov4" })
        {
            if (file.size() == std::strlen(logo) && std::equal(file.begin(), file.end(), logo, [](char a, char b) { return std::tolower((unsigned char)a) == std::tolower((unsigned char)b); }))
                return nullptr;
        }
    }
    return shBinkOpen.stdcall<void*>(name, flags);
}

void InstallLogoSkip()
{
    if (shBinkOpen || !FixSettings().Bool("MAIN.SkipIntro"))
        return;
    if (auto bink = GetModuleHandleW(L"binkw32.dll"))
    {
        if (auto open = GetProcAddress(bink, "_BinkOpen@8"))
            shBinkOpen = safetyhook::create_inline(open, BinkOpenHook);
    }
}

class Movies
{
public:
    Movies()
    {
        WFP::onInitEvent() += []()
        {
            InstallLogoSkip();

            // FMVs
            if (!Screen.bSignatureEdition)
            {
                auto pattern = hook::pattern("89 50 18 8B 06 8B CE FF 50 14 8B 4F 08");
                struct FMVHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        float scaleX = 1.0f, scaleY = 1.0f;
                        const float aspect = Screen.fFMVScale;
                        if (std::isfinite(aspect) && aspect > 0.0f)
                        {
                            // The native video plane uses the game's 4:3 design space.
                            const float fit = std::min(1.0f, aspect);
                            scaleX = fit / aspect;
                            scaleY = fit;
                        }
                        for (int offset : { -0x54, -0x38, -0x1C, 0 })
                        {
                            *(float*)(regs.eax + offset) *= scaleX;
                            *(float*)(regs.eax + offset + 4) *= scaleY;
                        }

                        *(uintptr_t*)(regs.eax + 0x18) = regs.edx;
                        regs.eax = *(uintptr_t*)(regs.esi);
                    }
                }; injector::MakeInline<FMVHook>(pattern.get_first(0));
            }
        };
    }
} Movies;
