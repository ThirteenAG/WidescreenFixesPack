#include "stdafx.h"
#include "LEDEffects.h"
#include <vector>
#include <algorithm>
#include <ranges>

static bool bLogiLedInitialized = false;

uint32_t* sPlayer1Ptr = nullptr;
uint32_t* sPlayer2Ptr = nullptr;
void __fastcall sub_529250(uint32_t* _this, void* edx, int32_t a2)
{
    if (*(uint32_t*)(_this + 0x40) == 3)
        sPlayer1Ptr = _this;
    else if (*(uint32_t*)(_this + 0x40) == 5)
        sPlayer2Ptr = _this;
    _this[0x40C] = a2;
}

void Init()
{
    CIniReader iniReader("");
    static auto nHideMouseCursorAfterMs = iniReader.ReadInteger("MAIN", "HideMouseCursorAfterMs", 5000);
    auto bDoorSkip = iniReader.ReadInteger("MAIN", "DoorSkip", 1) != 0;
    auto bBorderlessWindowed = iniReader.ReadInteger("MAIN", "BorderlessWindowed", 1) != 0;
    auto bLightSyncRGB = iniReader.ReadInteger("MAIN", "LightSyncRGB", 1) != 0;

    if (nHideMouseCursorAfterMs)
    {
        auto pattern = hook::pattern("80 BE ? ? ? ? ? 74 13 FF B6");
        struct MouseCursorHook
        {
            void operator()(injector::reg_pack& regs)
            {
                //if (*(uint8_t*)(regs.esi + 0x2684))
                //{
                //    SetCursor(*(HCURSOR*)(regs.esi + 0x2680));
                //    *(uint8_t*)(regs.esi + 0x2684) = 0;
                //}
                //else
                {
                    static auto TIMERA = LEDEffects::Timer();
                    static bool bHideCur = false;
                    static CURSORINFO hiddenPoint = {};
                    static POINT oldPoint = {};
                    POINT curPoint = {};
                    GetCursorPos(&curPoint);
                    if ((curPoint.x != oldPoint.x || curPoint.y != oldPoint.y) && (curPoint.x != hiddenPoint.ptScreenPos.x && curPoint.y != hiddenPoint.ptScreenPos.y))
                    {
                        if (bHideCur)
                            SetCursorPos(oldPoint.x, oldPoint.y);
                        bHideCur = false;
                        TIMERA.reset();
                    }
                    else
                    {
                        if (TIMERA > nHideMouseCursorAfterMs)
                        {
                            bHideCur = true;
                            TIMERA.reset();
                        }
                    }
                    if (curPoint.x != hiddenPoint.ptScreenPos.x && curPoint.y != hiddenPoint.ptScreenPos.y)
                        oldPoint = curPoint;

                    if (bHideCur)
                    {
                        SetCursorPos(9999, 9999);
                        hiddenPoint.cbSize = sizeof(CURSORINFO);
                        GetCursorInfo(&hiddenPoint);
                        SetCursorPos(hiddenPoint.ptScreenPos.x - 1, hiddenPoint.ptScreenPos.x - 1);
                    }
                }
            }
        }; injector::MakeInline<MouseCursorHook>(pattern.get_first(0), pattern.get_first(28));
    }

    if (bDoorSkip)
    {
        injector::MakeNOP(hook::get_pattern("0F 84 CC 00 00 00 8B 47 3C"), 6, true);
        auto pattern = hook::pattern("F3 0F 11 47 ? 81 49 ? ? ? ? ? EB 07");
        struct DoorkSkipHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.edi + 0x2C) = -1.0f;
            }
        }; injector::MakeInline<DoorkSkipHook>(pattern.get_first(0));
    }

    if (bBorderlessWindowed)
    {
        IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
            std::forward_as_tuple("CreateWindowExA", WindowedModeWrapper::CreateWindowExA_Hook),
            std::forward_as_tuple("CreateWindowExW", WindowedModeWrapper::CreateWindowExW_Hook),
            std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
            std::forward_as_tuple("SetWindowLongW", WindowedModeWrapper::SetWindowLongW_Hook),
            std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
            std::forward_as_tuple("SetWindowPos", WindowedModeWrapper::SetWindowPos_Hook)
        );
    }

    if (bLightSyncRGB)
    {
        bLogiLedInitialized = LogiLedInit();

        if (bLogiLedInitialized)
        {
            auto pattern = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 85 C0 75 09");
            injector::MakeJMP(injector::GetBranchDestination(pattern.get_first()), sub_529250, true);

            std::thread t([]()
            {
                while (true)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));

                    if (bLogiLedInitialized)
                    {
                        auto Player1ID = PtrWalkthrough<int32_t>(&sPlayer1Ptr, 0x100);
                        auto Player2ID = PtrWalkthrough<int32_t>(&sPlayer2Ptr, 0x100);
                        auto Player1Health = PtrWalkthrough<int32_t>(&sPlayer1Ptr, 0x1030);
                        auto Player2Health = PtrWalkthrough<int32_t>(&sPlayer2Ptr, 0x1030);
                        auto Player1Poisoned = PtrWalkthrough<int32_t>(&sPlayer1Ptr, 0x68C4);
                        auto Player2Poisoned = PtrWalkthrough<int32_t>(&sPlayer2Ptr, 0x68C4);

                        if (Player1ID && *Player1ID == 3 && Player1Health)
                        {
                            auto health1 = *Player1Health;
                            auto poisoned1 = *Player1Poisoned == 1;
                            if (health1 > 1)
                            {
                                if (health1 < 40) {
                                    if (!poisoned1)
                                        LEDEffects::SetLightingLeftSide(26, 4, 4, true, false); //red
                                    else
                                        LEDEffects::SetLightingLeftSide(51, 4, 53, true, false); //purple
                                    LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0); //red
                                }
                                else if (health1 < 130) {
                                    if (!poisoned1)
                                        LEDEffects::SetLightingLeftSide(50, 30, 4, true, false); //orange
                                    else
                                        LEDEffects::SetLightingLeftSide(51, 4, 53, true, false); //purple
                                    LEDEffects::DrawCardiogram(67, 0, 0, 0, 0, 0); //orange
                                }
                                else {
                                    if (!poisoned1)
                                        LEDEffects::SetLightingLeftSide(10, 30, 4, true, false);  //green
                                    else
                                        LEDEffects::SetLightingLeftSide(51, 4, 53, true, false);  //purple
                                    LEDEffects::DrawCardiogram(0, 100, 0, 0, 0, 0); //green
                                }
                            }
                            else
                            {
                                LEDEffects::SetLightingLeftSide(26, 4, 4, false, true); //red
                                LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0, true);
                            }

                            if (Player2ID && *Player2ID == 5 && Player2Health)
                            {
                                auto health2 = *Player2Health;
                                auto poisoned2 = *Player2Poisoned == 1;
                                if (health2 > 1)
                                {
                                    if (health2 < 40) {
                                        if (!poisoned2)
                                            LEDEffects::SetLightingRightSide(26, 4, 4, true, false); //red
                                        else
                                            LEDEffects::SetLightingRightSide(51, 4, 53, true, false); //purple
                                        LEDEffects::DrawCardiogramNumpad(100, 0, 0, 0, 0, 0); //red
                                    }
                                    else if (health2 < 130) {
                                        if (!poisoned2)
                                            LEDEffects::SetLightingRightSide(50, 30, 4, true, false); //orange
                                        else
                                            LEDEffects::SetLightingRightSide(51, 4, 53, true, false); //purple
                                        LEDEffects::DrawCardiogramNumpad(67, 0, 0, 0, 0, 0); //orange
                                    }
                                    else {
                                        if (!poisoned2)
                                            LEDEffects::SetLightingRightSide(10, 30, 4, true, false);  //green
                                        else
                                            LEDEffects::SetLightingRightSide(51, 4, 53, true, false);  //purple
                                        LEDEffects::DrawCardiogramNumpad(0, 100, 0, 0, 0, 0); //green
                                    }
                                }
                                else
                                {
                                    LEDEffects::SetLightingRightSide(26, 4, 4, false, true); //red
                                    LEDEffects::DrawCardiogramNumpad(100, 0, 0, 0, 0, 0, true);
                                }
                            }
                            else
                            {
                                LEDEffects::SetLightingRightSide(32, 10, 4); // rebecca's red cross
                            }
                        }
                        else
                        {
                            LogiLedStopEffects();
                            LEDEffects::SetLighting(32, 10, 4); // rebecca's red cross
                        }
                    }
                    else
                        break;
                }
            });

            t.detach();

            IATHook::Replace(GetModuleHandleA(NULL), "KERNEL32.DLL",
                std::forward_as_tuple("ExitProcess", static_cast<void(__stdcall*)(UINT)>([](UINT uExitCode) {
                    if (bLogiLedInitialized) {
                        LogiLedShutdown();
                        bLogiLedInitialized = false;
                    }
                    ExitProcess(uExitCode); 
                }))
            );
        }
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("56 8B F1 57 8B 06 8B 7C 24 0C 3B C7"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        if (bLogiLedInitialized) {
            LogiLedShutdown();
            bLogiLedInitialized = false;
        }
    }
    return TRUE;
}
