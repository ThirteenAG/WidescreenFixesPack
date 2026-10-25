module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

export module UI;

import ComVars;

bool bSignInFlowComplete = false;
bool bTriedToSignIn = false;

namespace Blaze
{
    namespace ProtoFire
    {
        SafetyHookInline shConnect = {};
        unsigned int __fastcall connect(void* _this, void* edx, const char* a2, int16_t a3, char a4, void* a5, int* a6)
        {
            if (!bTriedToSignIn)
            {
                bTriedToSignIn = true;
                return 0x800D0000;
            }
            return shConnect.fastcall<unsigned int>(_this, edx, a2, a3, a4, a5, a6);
        }
    }
}

namespace Ebisu
{
    // EbisuStartup(flags, port, info, handle): the Origin client listens on this computer at the port
    int (__cdecl* EbisuStartup)(int, int, void*, void*) = nullptr;

    bool IsOriginClientRunning(int port)
    {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
            return true;

        bool connected = true;
        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s != INVALID_SOCKET)
        {
            u_long nonBlocking = 1;
            ioctlsocket(s, FIONBIO, &nonBlocking);
            sockaddr_in addr = {};
            addr.sin_family = AF_INET;
            addr.sin_port = htons((u_short)port);
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            ::connect(s, (sockaddr*)&addr, sizeof(addr));

            fd_set writable, failed;
            FD_ZERO(&writable);
            FD_SET(s, &writable);
            FD_ZERO(&failed);
            FD_SET(s, &failed);
            timeval timeout = { 0, 250000 };
            connected = select(0, nullptr, &writable, &failed, &timeout) > 0 && FD_ISSET(s, &writable);
            closesocket(s);
        }

        WSACleanup();
        return connected;
    }

    int __cdecl EbisuStartupHook(int flags, int port, void* info, void* handle)
    {
        if (IsOriginClientRunning(port))
            return EbisuStartup(flags, port, info, handle);
        return 0xA0000001; // an error: the game goes on without Origin
    }
}

namespace fb
{
    namespace UIInputManager
    {
        SafetyHookInline shProcessConceptInput = {};
        void __fastcall processConceptInput(void** inputManager, void* edx, float a2, float a3)
        {
            shProcessConceptInput.fastcall(inputManager, edx, a2, a3);

            if (!bSignInFlowComplete)
            {
                if (*inputManager)
                {
                    constexpr int UIInputAction_OK = 10;
                    (**(void (__thiscall***)(void*, int, int)) * inputManager)(*inputManager, 1, UIInputAction_OK);
                }
            }
        }
    }
}

namespace fb
{
    namespace UIOnlineSignInFlowComplete
    {
        SafetyHookInline shexecute = {};
        void __stdcall execute(void* params, void* asset, void* dataSource)
        {
            bSignInFlowComplete = true;
            return shexecute.unsafe_stdcall(params, asset, dataSource);
        }
    }
}

// Escape is bound to the menus' Back and also to Activate (the pad's Start), and Activate is handled first. Offline,
// Activate on the Challenge Series list starts signing in to the online service, so Escape never got back out of it
// (and on the main menu it started signing in too). Outside races an Activate that comes with Back is dropped: on a pad Start and B
// are different buttons, and F1 or Start still activate.
namespace EscapeKey
{
    constexpr int UIInputAction_Activate = 6;
    constexpr int UIInputAction_Back = 11;

    int (__thiscall* GetConcept)(void* concepts, int action, int a3, int a4) = nullptr;

    bool IsBackPressed(uintptr_t inputManager, void* reader)
    {
        auto backInput = GetConcept((void*)(inputManager + 0x0C), UIInputAction_Back, 1, 0);
        auto getValue = (*(float (__thiscall***)(void*, int))reader)[5];
        return std::fabs(getValue(reader, backInput)) >= 0.05f;
    }
}

class UI
{
public:
    UI()
    {
        WFP::onInitEvent() += []()
        {
            {
                // UIInputManager::processConceptInput: for each action, the "pressed" call (edi = action, ebx = input reader)
                auto lookup = hook::pattern("6A 00 6A 01 57 8D 4D 0C 83 C6 14 E8");
                auto pressed = hook::pattern("8B 4D 00 85 C9 0F 84 ? ? ? ? 8B 01 8B 10 57 6A 01 FF D2");
                if (!lookup.empty() && !pressed.empty())
                {
                    EscapeKey::GetConcept = (decltype(EscapeKey::GetConcept))injector::GetBranchDestination(lookup.get_first(11)).as_int();
                    static auto nextAction = (uintptr_t)pressed.get_first(11) + *pressed.get_first<int32_t>(7);
                    static auto ActivatePressedHook = safetyhook::create_mid(pressed.get_first(), [](SafetyHookContext& regs)
                    {
                        // In a race Activate is the pause menu, which Escape opens
                        bool inRace = GetTickCount64() - fb::Vehicle::LastUpdate < 500;
                        if (regs.edi == EscapeKey::UIInputAction_Activate && !inRace && EscapeKey::IsBackPressed(regs.ebp, (void*)regs.ebx))
                            regs.eip = nextAction;
                    });
                }
            }

            auto bSkipIntro = FixSettings().Int("MAIN.SkipIntro") != 0;

            if (bSkipIntro)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? 85 C0 74 ? 8B CE E8 ? ? ? ? 8B 4E ? 8D 46");
                Blaze::ProtoFire::shConnect = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), Blaze::ProtoFire::connect);

                pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? 57 51 8B CE ? ? ? E8");
                fb::UIInputManager::shProcessConceptInput = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), fb::UIInputManager::processConceptInput);

                pattern = hook::pattern("E8 ? ? ? ? ? ? 6A ? 6A ? 8B C8 8B 42 ? 68 ? ? ? ? FF D0 6A ? E8");
                fb::UIOnlineSignInFlowComplete::shexecute = safetyhook::create_inline(pattern.get_first(), fb::UIOnlineSignInFlowComplete::execute);

                // The Origin SDK looks for the Origin client on this computer first and waits a long time for it
                // when it isn't running (about 40 seconds in 1.0). Without the client it now fails right away,
                // which the game handles the same way as after that wait.
                pattern = hook::pattern("68 90 0C 00 00 53 89 54 24 ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? E8");
                if (!pattern.empty())
                {
                    Ebisu::EbisuStartup = injector::GetBranchDestination(pattern.get_first(26)).get<std::remove_pointer_t<decltype(Ebisu::EbisuStartup)>>();
                    injector::MakeCALL(pattern.get_first(26), Ebisu::EbisuStartupHook, true);
                }

                // UI.SkipSignIn
                pattern = hook::pattern("6A ? E8 ? ? ? ? 5F 8A C3 5B 81 C4 ? ? ? ? C2 ? ? 8B 84 24");
                injector::WriteMemory<uint8_t>(pattern.get_first(1), 1, true);

                // UI.SkipBootflow
                pattern = hook::pattern("6A 00 8B C8 8B 42 ? 68 23 70 49 AF");
                injector::WriteMemory<uint8_t>(pattern.get_first(1), 1, true);

                // for safety, if any movie is playing, then we're already far enough
                pattern = hook::pattern("89 6C 24 ? 89 7C 24 ? 89 7C 24 ? 89 7C 24");
                static auto playMovieHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    bTriedToSignIn = true;
                    bSignInFlowComplete = true;
                });

                WFP::onGameProcessEvent() += []()
                {
                    if (bSignInFlowComplete)
                    {
                        Blaze::ProtoFire::shConnect.reset();
                        fb::UIInputManager::shProcessConceptInput.reset();

                        bSignInFlowComplete = false;
                    }
                };
            }
        };
    }
} UI;
