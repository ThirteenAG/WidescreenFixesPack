#include "stdafx.h"
#include <d3d9.h>
#include <d3dx9.h>
#include "dxsdk\d3dvtbl.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    float fCustomAspectRatioHor;
    float fCustomAspectRatioVer;
    int32_t Width43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fCutOffArea;
    float fFMVScale;
    bool bSignatureEdition;
    uintptr_t pBarsPtr;
} Screen;


SafetyHookInline shsub_A2A350 = {};
void __fastcall sub_A2A350(void* _this, void* edx, int a2, int a3, void* a4)
{
    if (Screen.pBarsPtr && (*(uint32_t*)((*(uintptr_t*)Screen.pBarsPtr) + 0xCC) == 3))
        return shsub_A2A350.unsafe_fastcall(_this, edx, a2, a3, a4);
}

typedef HRESULT(STDMETHODCALLTYPE* CreateDevice_t)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
CreateDevice_t RealD3D9CreateDevice = NULL;
typedef HRESULT(STDMETHODCALLTYPE* DrawPrimitive_t)(LPDIRECT3DDEVICE9, D3DPRIMITIVETYPE, UINT, UINT);
DrawPrimitive_t RealDrawPrimitive = NULL;
typedef HRESULT(STDMETHODCALLTYPE* SetStreamSource_t)(LPDIRECT3DDEVICE9, UINT, IDirect3DVertexBuffer9*, UINT, UINT);
SetStreamSource_t RealSetStreamSource = NULL;

IDirect3DVertexBuffer9* g_pCurrentVB = NULL;
UINT g_CurrentOffset = 0;
UINT g_CurrentStride = 0;

HRESULT WINAPI HookedSetStreamSource(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride)
{
    if (StreamNumber == 0)
    {
        g_pCurrentVB = pStreamData;
        g_CurrentOffset = OffsetInBytes;
        g_CurrentStride = Stride;
    }
    return RealSetStreamSource(pDevice, StreamNumber, pStreamData, OffsetInBytes, Stride);
}

float fMaxObjectLength = 50.0f;
HRESULT WINAPI DrawPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
    bool isSuspectType = (PrimitiveType == D3DPT_TRIANGLESTRIP || PrimitiveType == D3DPT_TRIANGLELIST);
    if (isSuspectType && StartVertex == 0 && PrimitiveCount > 300 && g_pCurrentVB && g_CurrentStride >= 12)
    {
        void* pData = NULL;
        UINT numVerts;
        if (PrimitiveType == D3DPT_TRIANGLESTRIP)
        {
            numVerts = PrimitiveCount + 2;
        }
        else
        {  // D3DPT_TRIANGLELIST
            numVerts = PrimitiveCount * 3;
        }
        UINT lockOffset = g_CurrentOffset + StartVertex * g_CurrentStride;
        UINT lockSize = numVerts * g_CurrentStride;

        if (SUCCEEDED(g_pCurrentVB->Lock(lockOffset, lockSize, &pData, D3DLOCK_READONLY)))
        {
            D3DXVECTOR3* verts = (D3DXVECTOR3*)((BYTE*)pData + 0);
            D3DXVECTOR3 minV = verts[0];
            D3DXVECTOR3 maxV = verts[0];

            for (UINT i = 1; i < numVerts; ++i)
            {
                D3DXVec3Minimize(&minV, &minV, &verts[i]);
                D3DXVec3Maximize(&maxV, &maxV, &verts[i]);
            }

            D3DXVECTOR3 extent;
            D3DXVec3Subtract(&extent, &maxV, &minV);
            float length = D3DXVec3Length(&extent);
            g_pCurrentVB->Unlock();

            if (length > fMaxObjectLength)
            {
                return D3D_OK;
            }
        }
    }

    return RealDrawPrimitive(pDevice, PrimitiveType, StartVertex, PrimitiveCount);
}

HRESULT WINAPI CreateDevice(IDirect3D9* d3ddev, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface)
{
    HRESULT retval = RealD3D9CreateDevice(d3ddev, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
    UINT_PTR* pVTable = (UINT_PTR*)(*((UINT_PTR*)*ppReturnedDeviceInterface));

    RealDrawPrimitive = (DrawPrimitive_t)pVTable[IDirect3DDevice9VTBL::DrawPrimitive];
    injector::WriteMemory(&pVTable[IDirect3DDevice9VTBL::DrawPrimitive], &DrawPrimitive, true);

    RealSetStreamSource = (SetStreamSource_t)pVTable[IDirect3DDevice9VTBL::SetStreamSource];
    injector::WriteMemory(&pVTable[IDirect3DDevice9VTBL::SetStreamSource], &HookedSetStreamSource, true);

    return retval;
}

void InitSettings()
{
    auto pattern = hook::pattern("75 66 8D 4C 24 04 51");
    injector::MakeNOP(pattern.get_first(0), 2, true); //0x40BD3F

    pattern = hook::pattern("75 39 83 7C 24 08 01 75 32 8B"); //0x40BD6C
    struct RegHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = *(uint32_t*)(regs.esp + 0x118);
            regs.edx = (regs.esp + 0x0C);

            GetModuleFileNameA(NULL, (char*)regs.edx, MAX_PATH);
            *(strrchr((char*)regs.edx, '\\') + 1) = '\0';
        }
    }; injector::MakeInline<RegHook>(pattern.get_first(0), pattern.get_first(20));
}

void* retAddrFading = nullptr;
bool bSkipFOVHook = false;
SafetyHookInline shsub_9B1C80 = {};
void __cdecl sub_9B1C80(float* a1)
{
    auto raddr = _ReturnAddress();
    if (raddr == retAddrFading)
        bSkipFOVHook = true;

    shsub_9B1C80.unsafe_ccall(a1);
    bSkipFOVHook = false;
}

void Init()
{
    CIniReader iniReader("");
    static bool bCustomAR;
    auto szForceAspectRatio = iniReader.ReadString("MAIN", "ForceAspectRatio", "auto");
    static bool bFullscreenFMVs = iniReader.ReadInteger("MAIN", "FullscreenFMVs", 1) != 0;
    static float fMouseSensitivityFactor = iniReader.ReadFloat("MAIN", "MouseSensitivityFactor", 0.0f);
    static float fFOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 0.0f);
    static bool bHideUntexturedObjects = iniReader.ReadInteger("MISC", "HideUntexturedObjects", 0) != 0;
    static bool bWindowedMode = iniReader.ReadInteger("MISC", "WindowedMode", 1) != 0;

    if (strncmp(szForceAspectRatio.c_str(), "auto", 4) != 0)
    {
        Screen.fCustomAspectRatioHor = static_cast<float>(std::stoi(szForceAspectRatio.c_str()));
        Screen.fCustomAspectRatioVer = static_cast<float>(std::stoi(strchr(szForceAspectRatio.c_str(), ':') + 1));
        bCustomAR = true;
    }

    if (bWindowedMode)
    {
        IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
            std::forward_as_tuple("CreateWindowExA", WindowedModeWrapper::CreateWindowExA_Hook),
            std::forward_as_tuple("CreateWindowExW", WindowedModeWrapper::CreateWindowExW_Hook),
            std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
            std::forward_as_tuple("SetWindowLongW", WindowedModeWrapper::SetWindowLongW_Hook),
            std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
            std::forward_as_tuple("SetWindowPos", WindowedModeWrapper::SetWindowPos_Hook),
            std::forward_as_tuple("MoveWindow", WindowedModeWrapper::MoveWindow_Hook)
        );
    }

    static auto SetScreenVars = [](int width, int height)
    {
        Screen.Width = width;
        Screen.Height = height;
        Screen.fWidth = static_cast<float>(Screen.Width);
        Screen.fHeight = static_cast<float>(Screen.Height);
        Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
        Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
        Screen.fWidth43 = static_cast<float>(Screen.Width43);
        Screen.fHudOffset = (1.0f / (Screen.fHeight * (4.0f / 3.0f))) * ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);
        Screen.fHudScale = Screen.fHeight / Screen.fWidth;
        Screen.fFieldOfView = 1.0f * (((4.0f / 3.0f)) / (Screen.fAspectRatio));
        Screen.fCutOffArea = 0.5f / Screen.fFieldOfView;
        Screen.fFMVScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));

        if (bCustomAR)
        {
            Screen.fAspectRatio = Screen.fCustomAspectRatioHor / Screen.fCustomAspectRatioVer;
            Screen.fHudScale = Screen.fCustomAspectRatioVer / Screen.fCustomAspectRatioHor;
            Screen.fFieldOfView = 1.0f * (((4.0f / 3.0f)) / (Screen.fAspectRatio));
            Screen.fCutOffArea = 0.5f / Screen.fFieldOfView;
        }

        if (fFOVFactor)
        {
            Screen.fFieldOfView /= fFOVFactor;
            Screen.fCutOffArea = 0.5f / Screen.fFieldOfView;
        }
    };

    auto pattern = hook::pattern("89 86 BC 02 00 00 89 8E C0 02 00 00 89 96 C4 02 00 00"); //0x9F2161
    if (!pattern.empty())
    {
        struct ResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                int width = regs.eax;
                int height = regs.ecx;

                *(uint32_t*)(regs.esi + 0x2BC) = width;
                *(uint32_t*)(regs.esi + 0x2C0) = height;
                *(uint32_t*)(regs.esi + 0x2C4) = regs.edx;

                if (bWindowedMode)
                    *(uint32_t*)(regs.esi + 0x2C8) = 1;

                SetScreenVars(width, height);
            }
        }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(18));
    }
    else
    {
        pattern = hook::pattern("8B 46 64 8B 4E 68");
        if (!pattern.empty())
        {
            struct ResHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.eax = *(uint32_t*)(regs.esi + 0x64);
                    regs.ecx = *(uint32_t*)(regs.esi + 0x68);

                    SetScreenVars(regs.eax, regs.ecx);
                }
            }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(6));
        }
        else
        {
            pattern = hook::pattern("8B 56 68 8B 44 24 44");
            if (!pattern.empty())
            {
                struct ResHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.eax = *(uint32_t*)(regs.esp + 0x44);

                        regs.ecx = *(uint32_t*)(regs.esi + 0x64);
                        regs.edx = *(uint32_t*)(regs.esi + 0x68);

                        SetScreenVars(regs.ecx, regs.edx);
                    }
                }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(10));
            }
            else
            {
                pattern = hook::pattern("8B 4E ? 8B 56 ? 53");
                if (!pattern.empty())
                {
                    struct ResHook
                    {
                        void operator()(injector::reg_pack& regs)
                        {
                            regs.ecx = *(uint32_t*)(regs.esi + 0x64);
                            regs.edx = *(uint32_t*)(regs.esi + 0x68);

                            SetScreenVars(regs.ecx, regs.edx);
                        }
                    }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(6));
                }
            }
        }
    }

    pattern = hook::pattern("D9 04 8D ? ? ? ? D9 5C 24 18 EB 0A 8B 90 B8 00 00 00");
    if (!pattern.empty())
    {
        auto dword_temp = *pattern.get_first<void*>(3);
        pattern = hook::pattern(pattern_str(0xD9, 0x04, '?', to_bytes(dword_temp))); //0xA01C67
        for (size_t i = 0; i < pattern.size(); i++)
        {
            injector::MakeNOP(pattern.get(i).get<uint32_t>(0), 1, true);
            injector::WriteMemory<uint16_t>(pattern.get(i).get<uint32_t>(1), 0x05D9, true);
            injector::WriteMemory(pattern.get(i).get<uint32_t>(3), &Screen.fHudScale, true);
        }
    }
    else
    {
        pattern = hook::pattern("8B 14 8D ? ? ? ? 89 54 24");
        auto dword_temp = *pattern.get_first<void*>(3);
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0x90, true);
        injector::WriteMemory<uint16_t>(pattern.get_first(1), 0x158B, true);
        injector::WriteMemory(pattern.get_first(3), &Screen.fHudScale, true);

        pattern = hook::pattern(pattern_str(0xD9, 0x04, '?', to_bytes(dword_temp)));
        for (size_t i = 0; i < pattern.size(); i++)
        {
            injector::MakeNOP(pattern.get(i).get<uint32_t>(0), 1, true);
            injector::WriteMemory<uint16_t>(pattern.get(i).get<uint32_t>(1), 0x05D9, true);
            injector::WriteMemory(pattern.get(i).get<uint32_t>(3), &Screen.fHudScale, true);
        }

        pattern = hook::pattern("D8 0C 95 ? ? ? ? E8");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0x90, true);
        injector::WriteMemory<uint16_t>(pattern.get_first(1), 0x0DD8, true);
        injector::WriteMemory(pattern.get_first(3), &Screen.fHudScale, true);
    }

    // FOV
    pattern = find_pattern("83 EC 1C 53 55 56 8B 74 24 ? 8B 46", "A1 ? ? ? ? 81 EC ? ? ? ? 83 F8 03", "A1 ? ? ? ? 81 EC A4 00 00 00 53");
    shsub_9B1C80 = safetyhook::create_inline(pattern.get_first(), sub_9B1C80);

    pattern = find_pattern("E8 ? ? ? ? D9 44 24 ? A1 ? ? ? ? 8A 88");
    if (!pattern.empty())
    {
        static auto FOVHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
        {
            if (bSkipFOVHook)
                *(float*)(regs.esp + 0x34) = 1.0f / tan(*(float*)(regs.esp + 0x28 + 4) * 0.5f);
            else
                *(float*)(regs.esp + 0x34) = Screen.fFieldOfView / tan(*(float*)(regs.esp + 0x28 + 4) * 0.5f);
        });

        // Some interface elements
        //pattern = hook::pattern("83 C4 04 8B 15 ? ? ? ? 81 C2 D0 00 00 00 52 E8 ? ? ? ? 83 C4 04 A1 ? ? ? ? 05 9C 01 00 00 50 8B 0D ? ? ? ? 81 C1 B8 03 00 00");
        //retAddrMenuInterface = pattern.get_first();
    }
    else
    {
        pattern = hook::pattern("E8 ? ? ? ? D9 44 24 ? 8B ? ? ? ? ? 8A");
        static auto FOVHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
        {
            if (bSkipFOVHook)
                *(float*)(regs.esp + 0x28) = 1.0f / *(float*)(regs.esp + 0x18 + 4);
            else
                *(float*)(regs.esp + 0x28) = Screen.fFieldOfView / *(float*)(regs.esp + 0x18 + 4);
        });

        // Some interface elements
        pattern = hook::pattern("A1 ? ? ? ? 56 05 C8 00 00 00");
        retAddrFading = pattern.get_first();

        //pattern = hook::pattern("83 C4 04 8B 15 ? ? ? ? 81 C2 C8 00 00 00 52 E8 ? ? ? ? 83 C4 04 A1 ? ? ? ? 05 94 01 00 00 50 8B 0D ? ? ? ? 81 C1 B0 03 00 00");
        //retAddrMenuInterface = pattern.get_first();
    }

    //objects disappear at screen edges with hor+, fixing that @0098B8CF
    pattern = hook::pattern("D8 0D ? ? ? ? D9 5C 24 10 8B 5C 24 10 53"); //0x98B8CF
    injector::WriteMemory(pattern.get_first(2), &Screen.fCutOffArea, true);

    //FMVs
    pattern = hook::pattern("89 50 18 8B 06 8B CE FF 50 14 8B 4F 08"); //0xA25984
    if (!pattern.empty())
    {
        struct FMVHook
        {
            void operator()(injector::reg_pack& regs)
            {
                //hor
                *(float*)(regs.eax - 0x54) /= Screen.fFMVScale;
                *(float*)(regs.eax - 0x38) /= Screen.fFMVScale;

                *(float*)(regs.eax - 0x1C) /= Screen.fFMVScale;
                *(float*)(regs.eax + 0x00) /= Screen.fFMVScale;

                if (bFullscreenFMVs)
                {
                    static const float fFMVSize = (640.0f / 386.0f) / (480.0f / 386.0f);
                    //hor
                    *(float*)(regs.eax - 0x54) *= fFMVSize;
                    *(float*)(regs.eax - 0x38) *= fFMVSize;

                    *(float*)(regs.eax - 0x1C) *= fFMVSize;
                    *(float*)(regs.eax + 0x00) *= fFMVSize;

                    //ver
                    *(float*)(regs.eax - 0x50) *= fFMVSize;
                    *(float*)(regs.eax - 0x18) *= fFMVSize;

                    *(float*)(regs.eax - 0x34) *= fFMVSize;
                    *(float*)(regs.eax + 0x04) *= fFMVSize;
                }

                *(uintptr_t*)(regs.eax + 0x18) = regs.edx;
                regs.eax = *(uintptr_t*)(regs.esi);
            }
        }; injector::MakeInline<FMVHook>(pattern.get_first(0));
    }

    auto text_pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 50 55 56 E8 ? ? ? ? 0F B7 4F 06 0F B7 57 04"); //0x992C4C
    static auto unk_CC0E20 = *text_pattern.count(1).get(0).get<char*>(1);
    struct TextHookGamersEdition
    {
        void operator()(injector::reg_pack& regs)
        {
            if (strncmp(unk_CC0E20, "16/9", 4) == 0)
            {
                strncpy(unk_CC0E20, "Bars", 4);
            }
            else
            {
                if (strncmp(unk_CC0E20, "4/3", 3) == 0)
                {
                    strncpy(unk_CC0E20, "Std", 4);
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
            if (strncmp(unk_CC0E20, "16/9", 4) == 0)
            {
                strncpy(unk_CC0E20, "Auto", 4);
            }
            else if (strncmp(unk_CC0E20, "4/3", 4) == 0)
            {
                strncpy(unk_CC0E20, "Std", 4);
            }
            else if (strncmp(unk_CC0E20, "4/3 ", 4) == 0)
            {
                strcpy(unk_CC0E20, "Black Bands");
            }

            regs.ecx = *(uint16_t*)(regs.edi + 6);
            regs.edx = *(uint16_t*)(regs.edi + 4);
        }
    };

    pattern = find_pattern("A1 ? ? ? ? 8B 88 00 08 00 00 81 C1 B4 68 06 00", "A1 ? ? ? ? 89 45 FC 8B 0D ? ? ? ? 89 0D");
    Screen.pBarsPtr = *pattern.get_first<uintptr_t>(1); //0xCBFBE0

    pattern = hook::pattern("8B 41 10 6A 00");
    if (!pattern.empty())
    {
        shsub_A2A350 = safetyhook::create_inline(pattern.get_first(), sub_A2A350);
        injector::MakeInline<TextHookGamersEdition>(text_pattern.get_first(18), text_pattern.get_first(18 + 8));
    }
    else
    {
        Screen.bSignatureEdition = true;
        injector::MakeInline<TextHookSignatureEdition>(text_pattern.get_first(18), text_pattern.get_first(18 + 8));
    }

    //if (fMouseSensitivityFactor)
    //{
    //	pattern = hook::pattern("D9 85 ? ? ? ? D8 1D ? ? ? ? DF E0 F6 C4 41 75 1D D9 85 4C"); //0x45F048
    //	struct MouseSensHook
    //	{
    //		void operator()(injector::reg_pack& regs)
    //		{
    //			*(float*)(regs.ebp - 0x1B0) *= fMouseSensitivityFactor;
    //			*(float*)(regs.ebp - 0x1B4) *= fMouseSensitivityFactor;
    //
    //			float temp = *(float*)(regs.ebp - 0x1B4);
    //			_asm fld     dword ptr[temp]
    //		}
    //	}; injector::MakeInline<MouseSensHook>(pattern.get_first(0), pattern.get_first(6));
    //}

    static std::string defaultCmd("/B /lang:01  /spg:50 /GDBShaders KKMaps.bf");
    if (Screen.bSignatureEdition)
        defaultCmd = "/B /TX /lang:01 /spg:50 KingKongTheGame.bf";

    pattern = find_pattern("C7 45 ? ? ? ? ? 68 04 01 00 00 8D 85 D8 FB FF FF 50", "C7 45 FC 00 00 00 00 E8 ? ? ? ? 8B 45 10"); //0x401A10
    struct StartupHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ebp - 0x4) = 0;
            std::string_view lpCmdLine(*(char**)(regs.ebp + 0x10));
            if (lpCmdLine.empty())
                *(uint32_t*)(regs.ebp + 0x10) = (uint32_t)defaultCmd.data();
        }
    }; injector::MakeInline<StartupHook>(pattern.get_first(0), pattern.get_first(7));

    if (true) //windowed mode text fix
    {
        auto [ResX, ResY] = GetDesktopRes();
        HKEY phkResult;
        DWORD cbData, Type;
        BYTE Data[4];

        if (!Screen.bSignatureEdition)
        {
            if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Ubisoft\\KingKong\\{2C391F94-B8B9-4832-9C57-3AFC332CC037}\\Basic video", 0, KEY_READ | KEY_SET_VALUE, &phkResult))
                RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Ubisoft\\KingKong\\{2C391F94-B8B9-4832-9C57-3AFC332CC037}\\Basic video", &phkResult);
        }
        else
        {
            if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Ubisoft\\KingKong\\{111E336D-30BF-4CD4-8D69-4541732AFB27}\\Basic video", 0, KEY_READ | KEY_SET_VALUE, &phkResult))
                RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Ubisoft\\KingKong\\{111E336D-30BF-4CD4-8D69-4541732AFB27}\\Basic video", &phkResult);
        }

        cbData = 4;
        if (!RegQueryValueExA(phkResult, "ResolutionWidth", 0, &Type, Data, &cbData) && Type == 4 && cbData == 4)
            ResX = *(int32_t*)Data;
        else
            RegSetValueExA(phkResult, "ResolutionWidth", 0, REG_DWORD, (const BYTE*)&ResX, cbData);
        cbData = 4;
        if (!RegQueryValueExA(phkResult, "ResolutionHeight", 0, &Type, Data, &cbData) && Type == 4 && cbData == 4)
            ResY = *(int32_t*)Data;
        else
            RegSetValueExA(phkResult, "ResolutionHeight", 0, REG_DWORD, (const BYTE*)&ResY, cbData);
        RegCloseKey(phkResult);

        pattern = hook::pattern("68 E0 01 00 00 68 80 02 00 00");
        for (size_t i = 0; i < pattern.size(); ++i)
        {
            injector::WriteMemory(pattern.get(i).get<uint32_t>(6), ResX, true);
            injector::WriteMemory(pattern.get(i).get<uint32_t>(1), ResY, true);
        }
    }

    if (bHideUntexturedObjects)
    {
        pattern = hook::pattern("8B 84 24 90 04 00 00 50 8B CD E8 ? ? ? ? 8B 45 00 8B 08 50");
        if (!pattern.empty())
        {
            struct Direct3DDeviceHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    auto pID3D9 = *(IDirect3D9**)(regs.ebp + 0x00);
                    UINT_PTR* pVTable = (UINT_PTR*)(*((UINT_PTR*)pID3D9));
                    RealD3D9CreateDevice = (CreateDevice_t)pVTable[IDirect3D9VTBL::CreateDevice];
                    injector::WriteMemory(&pVTable[IDirect3D9VTBL::CreateDevice], &CreateDevice, true);
                    regs.eax = *(uint32_t*)(regs.esp + 0x490);
                }
            }; injector::MakeInline<Direct3DDeviceHook>(pattern.get_first(0), pattern.get_first(7));
        }
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("33 DB 89 5D E0 53"));
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitSettings, hook::pattern("75 66 8D 4C 24 04 51"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}