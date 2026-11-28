module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RainDroplets.hpp"

export module Misc;

import ComVars;
import Window;
import Hud;
import Intro;
import OptionsMenu;
import Camera;
import States;
import Gamepad;

namespace
{
    using StreamStop = void(__cdecl*)();

    struct AudioStream
    {
        uint8_t** owner;
        size_t offset;
        StreamStop stop;
    };

    std::array<AudioStream, 2> AudioStreams{};

    HANDLE DuplicateAudioWorker(const AudioStream& audio)
    {
        const auto owner = *audio.owner;
        if (!owner)
            return nullptr;

        const auto stream = owner + audio.offset;
        const auto lock = reinterpret_cast<CRITICAL_SECTION*>(stream + 0x184);
        HANDLE copy = nullptr;
        EnterCriticalSection(lock);
        const auto worker = *reinterpret_cast<HANDLE*>(stream + 0x178);
        if (worker)
        {
            const auto process = GetCurrentProcess();
            DuplicateHandle(process, worker, process, &copy, SYNCHRONIZE, FALSE, 0);
        }
        LeaveCriticalSection(lock);
        return copy;
    }

    void __cdecl StopAudioStreams()
    {
        const std::array workers{ DuplicateAudioWorker(AudioStreams[0]), DuplicateAudioWorker(AudioStreams[1]) };
        for (const auto& stream : AudioStreams)
            stream.stop();

        // The native stop requests are asynchronous. The sound device must outlive both workers; a worker that never
        // ends must not keep the game from closing.
        for (const auto worker : workers)
        {
            if (worker)
            {
                WaitForSingleObject(worker, 3000);
                CloseHandle(worker);
            }
        }
    }

    void InitAudioShutdown()
    {
        auto calls = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? E8 ? ? ? ?");
        const auto firstStop = calls.get_first<uint8_t>(15);
        const auto secondStop = firstStop + 5;
        const auto voice = injector::GetBranchDestination(firstStop).as_int();
        const auto music = injector::GetBranchDestination(secondStop).as_int();
        AudioStreams = {{
            { *reinterpret_cast<uint8_t***>(voice + 27), 0x120, reinterpret_cast<StreamStop>(voice) },
            { *reinterpret_cast<uint8_t***>(music + 27), 0x88, reinterpret_cast<StreamStop>(music) }
        }};
        injector::MakeCALL(firstStop, StopAudioStreams, true);
        injector::MakeNOP(secondStop, 5, true);
    }
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            hook::pattern pattern;
            auto& settings = FixSettings();

            Screen.Width = settings.Int("MAIN.ResX");
            Screen.Height = settings.Int("MAIN.ResY");

            if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
                std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
            if (Screen.Width <= 0 || Screen.Height <= 0)
                std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

            SRSWindow::Init(Screen.Width, Screen.Height);
            InitAudioShutdown();
            WFP::onGameProcessEvent() += []()
            {
                const auto deepest = SRSStates::Deepest();
                SRSHud::Driving = deepest && std::string_view(SRSStates::Name(deepest)) == "MP2Game";
                // Off the road the frontend, the pause menu or a video is on the screen: no drops over it
                RainDroplets::MenuOpen(!SRSHud::Driving);
            };
            SRSWindow::FrameCallback = []() { SRSPad::UpdateDevice(); WFP::onGameProcessEvent().executeAll(); };
            SRSWindow::InputMessage = SRSPad::InputMessage;

            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

            pattern = hook::pattern("C7 47 14 ? ? ? ? C7 47 18"); //0x4C81A3 0x4C81AA
            injector::WriteMemory<float>(pattern.get_first<float>(3), Screen.fWidth, true);
            injector::WriteMemory<float>(pattern.get_first<float>(10), Screen.fHeight, true);

            pattern = hook::pattern("89 4F 14 D8 0D ? ? ? ? D9 5F 18"); //0x4C8235
            struct SetResHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    float temp = 0.0f;
                    _asm {fstp    dword ptr[temp]}

                    *(float*)(regs.edi + 0x14) = Screen.fWidth;
                    *(float*)(regs.edi + 0x18) = Screen.fHeight;
                }
            }; injector::MakeInline<SetResHook>(pattern.get_first<uint32_t>(3), pattern.get_first<uint32_t>(12));

            pattern = hook::pattern("84 DB 74 09 DB 87 ? ? ? ? D9 5F 14"); //0x4C8241
            injector::MakeNOP(pattern.get_first<uint32_t>(0), 13, true);

            //3D
            static float f3DScale = 1.0f / (480.0f * Screen.fAspectRatio);
            pattern = hook::pattern("D8 0D ? ? ? ? D9 5C 24 04 D9 46"); //0x4BC66B
            injector::WriteMemory<float*>(pattern.get_first<float*>(2), &f3DScale, true);

            pattern = hook::pattern("C7 05 ? ? ? ? 00 00 20 44"); //0x553DFC //mov
            if (pattern.empty())
            {
                (void)hook::pattern(pattern).get_first();
                std::abort();
            }
            for (size_t i = 0; i < pattern.size(); ++i)
            {
                injector::WriteMemory<float>(pattern.get(i).get<float>(6), 480.0f * Screen.fAspectRatio, true);
            }

            pattern = hook::pattern("68 00 00 F0 43 68 00 00 20 44"); //0x4CA7E6 //push
            if (pattern.empty())
            {
                (void)hook::pattern(pattern).get_first();
                std::abort();
            }
            for (size_t i = 0; i < pattern.size(); ++i)
            {
                injector::WriteMemory<float>(pattern.get(i).get<float>(6), 480.0f * Screen.fAspectRatio, true);
            }

            // Before the 2D hooks: the menu looks up the text function the HUD hooks
            SRSMenu::Init();
            SRSHud::Init(Screen.Width, Screen.Height);
            SRSIntro::Init();

            pattern = hook::pattern("DB 44 24 08 8D 86 A8 00 00 00 8D"); //0x5288D2
            struct RearviewMirrorHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    int32_t fMirrorOffset = *(int32_t*)(regs.esp + 0x8) + (int32_t)(((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f);
                    _asm {fild    dword ptr[fMirrorOffset]}
                    regs.eax = regs.esi + 0xA8;
                }
            }; injector::MakeInline<RearviewMirrorHook>(pattern.get_first<uint32_t>(0), pattern.get_first<uint32_t>(10));

            SRSCamera::Init(Screen.Width, Screen.Height);
            SRSPad::Init();
            asi_settings::LiveLink::Start();
        };
    }
} Misc;
