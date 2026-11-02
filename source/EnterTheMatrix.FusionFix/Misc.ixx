module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Misc;

import ComVars;
import Draw;

namespace Intro
{
    using AdvanceFn = void (__thiscall*)(uint8_t*);

    SafetyHookInline UpdateHook, RenderHook;
    AdvanceFn Advance = nullptr;
    bool Skip = false;
    std::atomic<bool> Done{ false };

    int32_t& Step(uint8_t* frontend) { return *reinterpret_cast<int32_t*>(frontend + 0x24); }
    int32_t& Fade(uint8_t* frontend) { return *reinterpret_cast<int32_t*>(frontend + 0x9A0); }

    void Run(uint8_t* self)
    {
        if (Skip)
        {
            if (Step(self) == 0)
            {
                // The game sets up its first logo; each advance closes one logo movie and opens the next step
                UpdateHook.thiscall(self);
                while (Step(self) >= 1 && Step(self) <= 4)
                    Advance(self);
                // The notice step: the title movies load on the next update
                if (Step(self) == 5)
                    Fade(self) = 99;
                else
                    Skip = false;
                return;
            }
            if (Step(self) == 5 && Fade(self) < 0)
            {
                Skip = false;
                Advance(self);
                return;
            }
        }
        UpdateHook.thiscall(self);
    }

    void __fastcall Update(uint8_t* self)
    {
        const int32_t step = Step(self);
        Run(self);
        // The copyright notice, the last step of the start-up sequence, has ended: the title menu follows
        if (step == 5 && Step(self) != 5)
            Done = true;
    }

    void __fastcall Render(uint8_t* self)
    {
        if (Skip && Step(self) == 5)
            return;
        RenderHook.thiscall(self);
    }
}

namespace UpdateNotice
{
    void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    void Init()
    {
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "EnterTheMatrix.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []() { return Intro::Done.load(); };
        config.show = [](const std::wstring& text)
        {
            Notice::Text = text;
            Notice::Until = GetTickCount64() + 12000;
        };
        RWGTA::UpdateCheck::Init(std::move(config));
    }
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            // Skip intro: the logo movies and the copyright notice
            auto pattern = hook::pattern("55 8B EC 83 EC 08 89 4D FC 8B 45 FC 8B 48 24 89 4D F8 8B 55 F8 83 EA 01 89 55 F8 83 7D F8 04");
            Intro::Advance = reinterpret_cast<Intro::AdvanceFn>(pattern.get_first());
            pattern = hook::pattern("55 8B EC 83 EC 0C 89 4D F8 C7 45 FC 02 00 00 00 33 C0 A0");
            Intro::UpdateHook = safetyhook::create_inline(pattern.get_first(), Intro::Update);
            pattern = hook::pattern("55 8B EC 81 EC 88 00 00 00 89 8D 78 FF FF FF 8B 85 78 FF FF FF 83 78 24 05");
            nFrontendFont.SetAddress(*pattern.get_first<int32_t*>(0x95));
            Intro::RenderHook = safetyhook::create_inline(pattern.get_first(), Intro::Render);
            Intro::Skip = FixSettings().Bool("MAIN.SkipIntro");
            UpdateNotice::Init();
        };

        WFP::onGameProcessEvent() += []()
        {
            RWGTA::UpdateCheck::Tick();
        };
    }
} Misc;
