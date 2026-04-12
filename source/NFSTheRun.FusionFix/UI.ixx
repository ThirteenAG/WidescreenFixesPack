module;

#include <stdafx.h>

export module UI;

import ComVars;
import Speedhack;

bool bSignInFlowComplete = false;
bool bTriedToSignIn = false;
bool bProcessesInput = false;

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

namespace fb
{
    namespace UIInputManager
    {
        SafetyHookInline shProcessConceptInput = {};
        void __fastcall processConceptInput(void** inputManager, void* edx, float a2, float a3)
        {
            bProcessesInput = true;

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

class UI
{
public:
    UI()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            auto bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;

            if (bSkipIntro)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? 85 C0 74 ? 8B CE E8 ? ? ? ? 8B 4E ? 8D 46");
                Blaze::ProtoFire::shConnect = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), Blaze::ProtoFire::connect);

                pattern = hook::pattern("E8 ? ? ? ? ? ? ? ? 57 51 8B CE ? ? ? E8");
                fb::UIInputManager::shProcessConceptInput = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), fb::UIInputManager::processConceptInput);

                pattern = hook::pattern("E8 ? ? ? ? ? ? 6A ? 6A ? 8B C8 8B 42 ? 68 ? ? ? ? FF D0 6A ? E8");
                fb::UIOnlineSignInFlowComplete::shexecute = safetyhook::create_inline(pattern.get_first(), fb::UIOnlineSignInFlowComplete::execute);

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
                    if (bProcessesInput)
                    {
                        // Somehow increasing process speed affects loading times
                        static bool bSpeedhackInitialized = false;
                        if (!bSpeedhackInitialized)
                        {
                            InitSpeedhack();
                            bSpeedhackInitialized = true;
                        }
                    }

                    if (bSignInFlowComplete)
                    {
                        Blaze::ProtoFire::shConnect.reset();
                        fb::UIInputManager::shProcessConceptInput.reset();

                        DisableSpeedhack();

                        bSignInFlowComplete = false;
                    }
                };
            }
        };
    }
} UI;