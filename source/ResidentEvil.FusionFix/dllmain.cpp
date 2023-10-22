#include "stdafx.h"
#include "LEDEffects.h"
#include <vector>
#include <algorithm>
#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3dx9.lib")

static bool bLogiLedInitialized = false;

struct StringRes
{
    const char* entry;
    uint32_t id;
}; std::vector<StringRes> resListString;

struct NumRes
{
    uint32_t x;
    uint32_t y;
    uint32_t id;
}; std::vector<NumRes> resListNum;

std::vector<std::string> resList;
const char* __fastcall GetResString(void* _this, int edx, unsigned int id)
{
    return resList[std::clamp(id, 0u, resList.size() - 1)].c_str();
}

int sprintfHook(char* str, const char* format, int x, int y)
{
    auto result = sprintf(str, format, x, y);
    if (x >= 1024 || y >= 768)
        resList.push_back(str);
    return result;
}

int __stdcall GetResID(int a1, int a2)
{
    auto it = std::find_if(resListNum.begin(), resListNum.end(), [&] (auto& m) -> bool { return m.x == a1 && m.y == a2; });
    if (it != resListNum.end())
        return it->id;
    return 0;
}

char __fastcall sub_41CD80(uint32_t* _this, void* edx)
{
    _this[33] = 3;
    _this[31] = 0;
    return 0;
}

uint32_t* ammoInClip = nullptr;
uint32_t* maxAmmoInClip = nullptr;
void AmmoInClip()
{
    if (!ammoInClip || !maxAmmoInClip)
        return;

    static std::vector<LogiLed::KeyName> keys = {
        LogiLed::KeyName::F1, LogiLed::KeyName::F2, LogiLed::KeyName::F3,
        LogiLed::KeyName::F4, LogiLed::KeyName::F5, LogiLed::KeyName::F6,
        LogiLed::KeyName::F7, LogiLed::KeyName::F8, LogiLed::KeyName::F9,
        LogiLed::KeyName::F10, LogiLed::KeyName::F11, LogiLed::KeyName::F12,
        LogiLed::KeyName::PRINT_SCREEN, LogiLed::KeyName::SCROLL_LOCK,
        LogiLed::KeyName::PAUSE_BREAK,
    };

    static auto oldAmmoInClip = -1;
    if (*ammoInClip != oldAmmoInClip)
    {
        auto maxAmmo = *maxAmmoInClip;
        if (maxAmmo == 255) // grenade launcher
            maxAmmo = *ammoInClip;

        float ammoInClipPercent = ((float)*ammoInClip / (float)maxAmmo) * 100.0f;

        for (size_t i = 0; i < keys.size(); i++)
        {
            auto indexInPercent = ((float)i / (float)keys.size()) * 100.0f;
            if (ammoInClipPercent > indexInPercent)
                LogiLedSetLightingForKeyWithKeyName(keys[i], 100, 100, 100);
            else
                LogiLedSetLightingForKeyWithKeyName(keys[i], 10, 10, 10);
        }
    }
    oldAmmoInClip = *ammoInClip;
}

void __fastcall sub_65F6A0(uint32_t* _this, void* edx, int ammo)
{
    ammoInClip = &_this[1023];
    maxAmmoInClip = &_this[1024];

    auto& CurrentAmmoInClip = _this[1023];
    auto& CurrentAmmoInClipMax = _this[1024];
    auto& CurrentAmmoInClipUsage = _this[1025];
    CurrentAmmoInClip = (ammo > CurrentAmmoInClipMax) ? CurrentAmmoInClipMax : ammo;
}

int __fastcall sub_663820(uint32_t* _this, void* edx)
{
    ammoInClip = &_this[1023];
    maxAmmoInClip = &_this[1024];

    auto& nVar1 = _this[1];
    auto& CurrentAmmoInClip = _this[1023];
    auto& CurrentAmmoInClipMax = _this[1024];
    auto& CurrentAmmoInClipUsage = _this[1025];
    int result;

    if (nVar1)
    {
        result = nVar1 - 1;
        if (result)
        {
            if (!--result && CurrentAmmoInClip > 0)
                nVar1 = 0;
        }
        else if (CurrentAmmoInClip <= 0)
        {
            nVar1 = 0;
        }
    }
    else
    {
        if (CurrentAmmoInClip > 0)
        {
            result = (*(int(__thiscall**)(void*, int, float, float, float, float))(_this[0] + 104))(_this, 0, 4.0f, 0.0f, 1.0f, 0);
            nVar1 = 1;
        }
        else
        {
            result = (*(int(__thiscall**)(void*, int, float, float, float, float))(_this[0] + 104))(_this, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f);
            nVar1 = 2;
        }
    }
    return result;
}

bool bDisableNoise = false;
bool bDisableColorCorrection = false;
uintptr_t pD3D9DeviceAddr;
IDirect3DPixelShader9* __stdcall CreatePixelShaderHook(const DWORD** a1)
{
    if (!a1)
        return nullptr;

    auto pDevice = (IDirect3DDevice9*)*(uint32_t*)(*(uint32_t*)pD3D9DeviceAddr + 152);
    IDirect3DPixelShader9* pShader = nullptr;
    pDevice->CreatePixelShader(a1[2], &pShader);

    if (pShader != nullptr)
    {
        static constexpr unsigned char dummyShader[] = {
            0x00, 0x03, 0xFF, 0xFF, 0xFE, 0xFF, 0x16, 0x00, 0x43, 0x54, 0x41, 0x42, 0x1C, 0x00, 0x00, 0x00,
            0x23, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x01, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x70, 0x73, 0x5F, 0x33, 0x5F, 0x30, 0x00, 0x4D,
            0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53,
            0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72, 0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65,
            0x72, 0x20, 0x39, 0x2E, 0x32, 0x39, 0x2E, 0x39, 0x35, 0x32, 0x2E, 0x33, 0x31, 0x31, 0x31, 0x00,
            0x51, 0x00, 0x00, 0x05, 0x00, 0x00, 0x0F, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x08, 0x0F, 0x80,
            0x00, 0x00, 0x00, 0xA0, 0xFF, 0xFF, 0x00, 0x00
        };
   
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
                "mov oC0, r0\n";

            LPD3DXBUFFER pCode;
            LPD3DXBUFFER pErrorMsgs;
            LPDWORD shader_data;
            auto result = D3DXAssembleShader(shader_text, strlen(shader_text), NULL, NULL, 0, &pCode, &pErrorMsgs);
            if (FAILED(result)) {
                shader_data = (DWORD*)&dummyShader[0];
            }
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
        else if (crc == 0x2C3893DD && bDisableNoise)
        {
            const char* shader_text =
                "ps_3_0\n"
                "dcl_texcoord v0\n"
                "dcl_2d s0\n"
                "dcl_2d s1\n"
                "dcl_2d s2\n"
                "texld r0, v0, s2\n"
                "mov oC0.xyz, r0\n"
                "mov oC0.w, c0.y\n";

            LPD3DXBUFFER pCode;
            LPD3DXBUFFER pErrorMsgs;
            LPDWORD shader_data;
            auto result = D3DXAssembleShader(shader_text, strlen(shader_text), NULL, NULL, 0, &pCode, &pErrorMsgs);
            if (FAILED(result)) {
                shader_data = (DWORD*)&dummyShader[0];
            }
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

    return pShader;
}

void Init()
{
    CIniReader iniReader("");
    auto bUnlockAllResolutions = iniReader.ReadInteger("MAIN", "UnlockAllResolutions", 1) != 0;
    static auto nHideMouseCursorAfterMs = iniReader.ReadInteger("MAIN", "HideMouseCursorAfterMs", 5000);
    auto bDoorSkip = iniReader.ReadInteger("MAIN", "DoorSkip", 1) != 0;
    auto bBorderlessWindowed = iniReader.ReadInteger("MAIN", "BorderlessWindowed", 1) != 0;
    auto bLightSyncRGB = iniReader.ReadInteger("MAIN", "LightSyncRGB", 1) != 0;
    bDisableNoise = iniReader.ReadInteger("MAIN", "DisableNoise", 0) != 0;
    bDisableColorCorrection = iniReader.ReadInteger("MAIN", "DisableColorCorrection", 0) != 0;

    if (bUnlockAllResolutions)
    {
        // Gather a resolution list from the game
        auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 20 47");
        injector::MakeCALL(pattern.get_first(), sprintfHook, true);

        // Hook right after sprintf loop
        pattern = hook::pattern("33 DB 33 ED 89 5C 24 58");
        struct ResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ebx = 0;
                regs.ebp = 0;
                *(uint32_t*)(regs.esp + 0x58) = regs.ebx;

                std::sort(resList.begin(), resList.end(), [](const std::string& lhs, const std::string& rhs)
                {
                    int32_t x1, y1, x2, y2;
                    sscanf_s(lhs.c_str(), "%dx%d", &x1, &y1);
                    sscanf_s(rhs.c_str(), "%dx%d", &x2, &y2);
                    return (x1 != x2) ? (x1 < x2) : (x1 * y1 < x2 * y2);
                });
                resList.erase(std::unique(std::begin(resList), std::end(resList)), resList.end());

                for (auto idx = 0; idx < resList.size(); idx++)
                {
                    int x = 0, y = 0;
                    auto str = resList[idx].c_str();
                    sscanf_s(str, "%dx%d", &x, &y);
                    resListString.emplace_back(str, idx);
                    resListNum.emplace_back(x, y, idx);
                }

                auto pattern = hook::pattern("B9 ? ? ? ? 89 44 24 0C 89 74 24 28");
                injector::WriteMemory(pattern.get_first(1), resList.size(), true);
                pattern = hook::pattern("BA ? ? ? ? 89 44 24 0C 89 4C 24 14");
                injector::WriteMemory(pattern.get_first(1), resList.size(), true);
                pattern = hook::pattern("83 FE 0C 72 C2");
                injector::WriteMemory<uint8_t>(pattern.get_first(2), resList.size(), true);
                pattern = hook::pattern("83 F8 0B 77 65 53");
                injector::WriteMemory<uint8_t>(pattern.get_first(2), resList.size() - 1, true);
                pattern = hook::pattern("83 C6 08 83 FE 60");
                static auto loc_47D471 = hook::get_pattern("8B 8E ? ? ? ? 8B C7 8D A4 24");
                struct LoopBreakHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.esi += 8;
                        if (regs.esi >= resList.size() * 8)
                            *(void**)(regs.esp - 4) = loc_47D471;
                    }
                }; injector::MakeInline<LoopBreakHook>(pattern.get_first(0), pattern.get_first(8));

                pattern = hook::pattern("BE ? ? ? ? B9 ? ? ? ? 89 44 24 0C");
                injector::WriteMemory(pattern.get_first(1), resListString.data(), true);
                pattern = hook::pattern("B9 ? ? ? ? BA ? ? ? ? 89 44 24 0C");
                injector::WriteMemory(pattern.get_first(1), resListString.data(), true);
                pattern = hook::pattern("8B 04 F5 ? ? ? ? 8B CF 8D A4 24 ? ? ? ? 8A 11 3A 10 75 19");
                injector::WriteMemory(pattern.get_first(3), resListString.data(), true);
                pattern = hook::pattern("8B 04 C5 ? ? ? ? 57");
                injector::WriteMemory(pattern.get_first(3), resListString.data(), true);
                pattern = hook::pattern("8B 8E ? ? ? ? 8B C7 8D A4 24");
                injector::WriteMemory(pattern.get_first(2), resListString.data(), true);
                pattern = hook::pattern("8B 0D ? ? ? ? 8B 07");
                injector::WriteMemory(pattern.get_first(2), resListString.data(), true);

                pattern = hook::pattern("FF 34 85 ? ? ? ? E8 ? ? ? ? 6A 01 6A 20");
                injector::WriteMemory(pattern.get_first(3), &resListNum[0].id, true);

                pattern = hook::pattern("E8 ? ? ? ? 6A 01 6A 20");
                injector::MakeCALL(pattern.get_first(), GetResString, true);
                pattern = hook::pattern("E8 ? ? ? ? 6A 08 89 83");
                injector::MakeCALL(pattern.get_first(), GetResID, true);
                pattern = hook::pattern("0F 87 ? ? ? ? FF 24 85 ? ? ? ? C7 04 24");
                struct GetResValueHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *(uint32_t*)(regs.esp + 0x00) = 1024;
                        *(uint32_t*)(regs.esp + 0x04) = 768;

                        auto it = std::find_if(resListNum.begin(), resListNum.end(), [&](auto& m) -> bool { return m.id == regs.eax; });
                        if (it != resListNum.end())
                        {
                            *(uint32_t*)(regs.esp + 0x00) = it->x;
                            *(uint32_t*)(regs.esp + 0x04) = it->y;
                        }
                    }
                }; injector::MakeInline<GetResValueHook>(pattern.get_first(0), pattern.get_first(28));
            }
        }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(8));
    }

    if (nHideMouseCursorAfterMs)
    {
        auto pattern = hook::pattern("80 BB ? ? ? ? ? 74 13");
        struct MouseCursorHook
        {
            void operator()(injector::reg_pack& regs)
            {
                //if (*(uint8_t*)(regs.ebx + 0x2684))
                //{
                //    SetCursor(*(HCURSOR*)(regs.ebx + 0x2680));
                //    *(uint8_t*)(regs.ebx + 0x2684) = 0;
                //}
                //else
                {
                    static auto TIMERA = LEDEffects::Timer();
                    static bool bHideCur = false;
                    static POINT oldPoint = {};
                    POINT curPoint = {};
                    GetCursorPos(&curPoint);
                    if (curPoint.x != oldPoint.x || curPoint.y != oldPoint.y)
                    {
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
                    oldPoint = curPoint;

                    if (bHideCur)
                        SetCursor(NULL);
                }
            }
        }; injector::MakeInline<MouseCursorHook>(pattern.get_first(0), pattern.get_first(28));
    }

    if (bDoorSkip)
    {
        auto pattern = hook::pattern("E8 ? ? ? ? 84 C0 0F 85 ? ? ? ? 6A 00 51 8B 0D");
        injector::MakeCALL(pattern.get_first(), sub_41CD80, true);
        injector::MakeJMP(hook::get_pattern("C7 46 ? ? ? ? ? C7 86 ? ? ? ? ? ? ? ? C7 86 ? ? ? ? ? ? ? ? 3B DA"), hook::get_pattern("8B 46 04 57 33 FF"));
        std::vector<uint8_t> DoorEventReturn = { 0x5F, 0xC7, 0x86, 0x84, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x5E, 0x5D, 0x5B, 0xC2, 0x10, 0x00 };
        injector::WriteMemoryRaw(hook::get_pattern("81 7E ? ? ? ? ? 75 07"), DoorEventReturn.data(), DoorEventReturn.size(), true);
        injector::WriteMemory<uint8_t>(hook::get_pattern("68 FB 00 00 00 EB 1D", 1), 0xFA, true); // Lift Fix
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
        pD3D9DeviceAddr = (uintptr_t)*hook::get_pattern<uint32_t>("A1 ? ? ? ? 56 66 0F 6E 80", 1);
        injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 89 47 08 66 8B 4C 24 ? 0F B7 C1 43 83 C5 0C 3B D8 72 C7 33 C0"), CreatePixelShaderHook, true);
    }

    if (bLightSyncRGB)
    {
        bLogiLedInitialized = LogiLedInit();

        if (bLogiLedInitialized)
        {
            auto pattern = hook::pattern("8B 44 24 04 8B 91 ? ? ? ? 89 81 ? ? ? ? 3B C2");
            injector::MakeJMP(pattern.get_first(), sub_65F6A0, true);
            pattern = hook::pattern("56 8B F1 8B 46 04 83 E8 00 74 25");
            injector::MakeJMP(pattern.get_first(), sub_663820, true);

            static auto sub_4898C0 = (int(__fastcall*) (void* _this, int edx))hook::get_pattern("8B D1 56 8B 42 24");
            static auto dword_D7C938 = *hook::get_pattern<void**>("8B 0D ? ? ? ? E8 ? ? ? ? 8B D0 89 54 24 18", 2);
            std::thread t([]()
            {
                while (true)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));

                    if (bLogiLedInitialized)
                    {
                        if (!*dword_D7C938)
                            continue;

                        auto sPlayerPtr = sub_4898C0(*dword_D7C938, 0);
                        if (sPlayerPtr)
                        {
                            auto Player1Health = PtrWalkthrough<int32_t>(&sPlayerPtr, 0x13BC);
                            auto Player1Poisoned = PtrWalkthrough<int32_t>(&sPlayerPtr, 0x1584);

                            if (Player1Health)
                            {
                                auto health1 = *Player1Health;
                                auto poisoned = *Player1Poisoned;
                                if (health1 > 1)
                                {
                                    if (health1 < 240) {
                                        if (!poisoned)
                                            LEDEffects::SetLighting(26, 4, 4, true, false, true); //red
                                        else
                                            LEDEffects::SetLighting(51, 4, 53, true, false, true); //purple
                                        LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0); //red
                                    }
                                    else if (health1 < 720) {
                                        if (!poisoned)
                                            LEDEffects::SetLighting(50, 30, 4, true, false, true); //orange
                                        else
                                            LEDEffects::SetLighting(51, 4, 53, true, false, true); //purple
                                        LEDEffects::DrawCardiogram(67, 0, 0, 0, 0, 0); //orange
                                    }
                                    else {
                                        if (!poisoned)
                                            LEDEffects::SetLighting(10, 30, 4, true, false, true);  //green
                                        else
                                            LEDEffects::SetLighting(51, 4, 53, true, false, true); //purple
                                        LEDEffects::DrawCardiogram(0, 100, 0, 0, 0, 0); //green
                                    }

                                    AmmoInClip();
                                }
                                else
                                {
                                    LEDEffects::SetLighting(26, 4, 4, false, true); //red
                                    LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0, true);
                                }
                            }
                            else
                            {
                                LogiLedStopEffects();
                                LEDEffects::SetLighting(93, 12, 14); //umbrella red
                            }
                        }
                        else
                        {
                            LogiLedStopEffects();
                            LEDEffects::SetLighting(93, 12, 14); //umbrella red
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
        CallbackHandler::RegisterCallback(Init, hook::pattern("8B 04 C5 ? ? ? ? 57"));
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
