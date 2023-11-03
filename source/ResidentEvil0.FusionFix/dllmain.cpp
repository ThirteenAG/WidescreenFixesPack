#include "stdafx.h"
#include "LEDEffects.h"
#include <vector>
#include <algorithm>
#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3dx9.lib")

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

bool bDisableNoise = false;
bool bDisableColorCorrection = false;
uintptr_t pD3D9DeviceAddr;
IDirect3DPixelShader9* __stdcall CreatePixelShaderHook(const DWORD** a1)
{
    if (!a1)
        return nullptr;

    auto pDevice = *(IDirect3DDevice9**)(*(uint32_t*)pD3D9DeviceAddr + 152);
    IDirect3DPixelShader9* pShader = nullptr;
    pDevice->CreatePixelShader(a1[2], &pShader);

    if (pShader != nullptr)
    {
        static std::vector<uint8_t> pbFunc;
        UINT len;
        pShader->GetFunction(nullptr, &len);
        if (pbFunc.size() < len)
            pbFunc.resize(len);

        pShader->GetFunction(pbFunc.data(), &len);

        auto crc = crc32(0, pbFunc.data(), len);

        if (crc == 0x59FA1317 && bDisableColorCorrection)
        {
            const char* shader_text =
                "ps_3_0\n"
                "dcl_texcoord v0.xy\n"
                "dcl_2d s0\n"
                "dcl_2d s1\n"
                "texld r0, v0, s0\n"
                "mul r0.xyz, r0, r0\n"
                "mov oC0.w, r0.w\n"
                "mul r1.xyz, r0.y, c2\n"
                "mad r1.xyz, r0.x, c1, r1\n"
                "mad r1.xyz, r0.z, c3, r1\n"
                "add r1.xyz, r1, c4\n"
                "texld r2, r1.x, s1\n"
                "mul r2.x, r2.x, r2.x\n"
                "texld r3, r1.y, s1\n"
                "texld r1, r1.z, s1\n"
                "mul r2.z, r1.z, r1.z\n"
                "mul r2.y, r3.y, r3.y\n"
                "rsq r0.x, r0.x\n"
                "rcp oC0.x, r0.x\n"
                "rsq r0.x, r0.y\n"
                "rsq r0.y, r0.z\n"
                "rcp oC0.z, r0.y\n"
                "rcp oC0.y, r0.x\n";

            LPD3DXBUFFER pCode;
            LPD3DXBUFFER pErrorMsgs;
            LPDWORD shader_data;
            auto result = D3DXAssembleShader(shader_text, strlen(shader_text), NULL, NULL, 0, &pCode, &pErrorMsgs);
            if (SUCCEEDED(result))
            {
                shader_data = (DWORD*)pCode->GetBufferPointer();
                IDirect3DPixelShader9* shader = nullptr;
                result = pDevice->CreatePixelShader(shader_data, &shader);
                if (FAILED(result)) {
                    return pShader;
                }
                else
                {
                    pShader->Release();
                    return shader;
                }
            }
        }
        else if (crc == 0x2C3893DD && bDisableNoise)
        {
            const char* shader_text =
                "ps_3_0\n"
                "def c0, -0.5, 1, -1, -0\n"
                "def c7, 0.298999995, 0.587000012, 0.114, 0\n"
                "def c8, -0.169, -0.331, 0.5, 0\n"
                "def c9, 0.5, -0.419, -0.0810000002, 0\n"
                "def c10, 1, 1.40199995, 0, 1.77199996\n"
                "def c11, 1, -0.344000012, -0.713999987, 0\n"
                "dcl_texcoord v0\n"
                "dcl_2d s0\n"
                "dcl_2d s1\n"
                "dcl_2d s2\n"
                "mov r0.x, c3.w\n"
                "mov r0.y, v0.w\n"
                "texld r0, r0, s0\n"
                "add r0.x, r0.x, c0.x\n"
                "dp2add r0.x, r0.x, c3.x, v0.x\n"
                "add r0.y, c3.y, v0.y\n"
                "rcp r1.x, c4.x\n"
                "rcp r1.y, c4.y\n"
                "mul r0.xy, r0, r1\n"
                "frc r0.xy, r0\n"
                "mov r1.y, c0.y\n"
                "mad r0.zw, r0.xyxy, -c4.xyxy, r1.y\n"
                "mul r0.xy, r0, c4\n"
                "cmp r0.w, r0.w, c0.z, c0.w\n"
                "cmp r0.w, r0.y, r0.w, c0.w\n"
                "cmp r0.z, r0.z, r0.w, c0.w\n"
                "cmp r0.z, r0.x, r0.z, c0.w\n"
                "lrp r1.xz, r0.xyyw, c6.zyww, c6.xyyw\n"
                "texld r2, r1.xzzw, s2\n"
                "mul r0.xyw, r2.xyzz, r2.xyzz\n"
                "cmp r0.xyz, r0.z, c5, r0.xyww\n"
                "dp3 r0.w, r0, c7\n"
                "mov r2, c2.xyxy\n"
                "texld r3, r2, s0\n"
                "texld r2, r2.zwzw, s0\n"
                "add r1.xz, r2.yyzw, c0.x\n"
                "add r1.w, r3.x, c0.x\n"
                "add r2.x, r1.y, c3.z\n"
                "mul r1.xzw, r1, r2.x\n"
                "mov r2.x, r0.w\n"
                "dp3 r0.w, r0, c8\n"
                "dp3 r0.x, r0, c9\n"
                "mov r2.z, r0.x\n"
                "mov r2.y, r0.w\n"
                "dp3 r0.y, r2, c11\n"
                "dp2add r0.z, r2, c10.xwzw, c10.z\n"
                "dp2add r0.x, r2.xzzw, c10, c10.z\n"
                "rcp r0.w, c2.z\n"
                "mul r1.xz, r0.w, v0.zyww\n"
                "texld r2, r1.xzzw, s1\n"
                "add r0.w, -r2.x, c0.y\n"
                "mad r0.w, r0.w, -c2.w, r1.y\n"
                "mul r0.xyz, r0.w, r0\n"
                "rsq r0.x, r0.x\n"
                "rcp oC0.x, r0.x\n"
                "rsq r0.x, r0.y\n"
                "rsq r0.y, r0.z\n"
                "rcp oC0.z, r0.y\n"
                "rcp oC0.y, r0.x\n"
                "mov oC0.w, c0.y\n";

            LPD3DXBUFFER pCode;
            LPD3DXBUFFER pErrorMsgs;
            LPDWORD shader_data;
            auto result = D3DXAssembleShader(shader_text, strlen(shader_text), NULL, NULL, 0, &pCode, &pErrorMsgs);
            if (SUCCEEDED(result))
            {
                shader_data = (DWORD*)pCode->GetBufferPointer();
                IDirect3DPixelShader9* shader = nullptr;
                result = pDevice->CreatePixelShader(shader_data, &shader);
                if (FAILED(result)) {
                    return pShader;
                }
                else
                {
                    pShader->Release();
                    return shader;
                }
            }
        }
    }

    return pShader;
}

std::vector<HWND> windows;
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    DWORD lpdwProcessId;
    GetWindowThreadProcessId(hwnd, &lpdwProcessId);
    if (lpdwProcessId == lParam)
    {
        if (IsWindowVisible(hwnd))
            windows.push_back(hwnd);
        return FALSE;
    }
    return TRUE;
}

void Init()
{
    CIniReader iniReader("");
    static auto nHideMouseCursorAfterMs = iniReader.ReadInteger("MAIN", "HideMouseCursorAfterMs", 5000);
    auto bDoorSkip = iniReader.ReadInteger("MAIN", "DoorSkip", 1) != 0;
    auto bBorderlessWindowed = iniReader.ReadInteger("MAIN", "BorderlessWindowed", 1) != 0;
    auto bLightSyncRGB = iniReader.ReadInteger("MAIN", "LightSyncRGB", 1) != 0;
    bDisableNoise = iniReader.ReadInteger("MAIN", "DisableNoise", 0) != 0;
    bDisableColorCorrection = iniReader.ReadInteger("MAIN", "DisableColorCorrection", 0) != 0;

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
                    if (windows.empty())
                        EnumWindows(EnumWindowsProc, GetCurrentProcessId());
                    else if (windows.front() == GetFocus())
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
                            if (hiddenPoint.ptScreenPos.x == 0 || hiddenPoint.ptScreenPos.y == 0)
                            {
                                SetCursorPos(99999, 99999);
                                hiddenPoint.cbSize = sizeof(CURSORINFO);
                                GetCursorInfo(&hiddenPoint);
                                hiddenPoint.ptScreenPos.x -= 1;
                                hiddenPoint.ptScreenPos.y -= 1;
                                SetCursorPos(hiddenPoint.ptScreenPos.x, hiddenPoint.ptScreenPos.y);
                            }
                            CURSORINFO Point = {};
                            Point.cbSize = sizeof(CURSORINFO);
                            GetCursorInfo(&Point);
                            if (Point.ptScreenPos.x != hiddenPoint.ptScreenPos.x || Point.ptScreenPos.y != hiddenPoint.ptScreenPos.y)
                                SetCursorPos(hiddenPoint.ptScreenPos.x, hiddenPoint.ptScreenPos.x);
                        }
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

    if (bDisableNoise || bDisableColorCorrection)
    {
        pD3D9DeviceAddr = (uintptr_t)*hook::get_pattern<uint32_t>("8B 0D ? ? ? ? E8 ? ? ? ? 0F B6 C0 F7 D8", 2);
        injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 89 46 08 66 8B 4C 24 ? 0F B7 C1 43 83 C5 0C 3B D8 72 C7 33 C0"), CreatePixelShaderHook, true);
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
                                LEDEffects::SetLightingRightSide(64, 20, 8); // rebecca's red cross
                            }
                        }
                        else
                        {
                            LogiLedStopEffects();
                            LEDEffects::SetLighting(64, 20, 8); // rebecca's red cross
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
