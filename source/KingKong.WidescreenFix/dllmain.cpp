#include "stdafx.h"
#include <sstream>
#include <iomanip>
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
    float fRawInputMouse;
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
typedef HRESULT(STDMETHODCALLTYPE* CreatePixelShader_t)(LPDIRECT3DDEVICE9, const DWORD*, IDirect3DPixelShader9**);
CreatePixelShader_t RealCreatePixelShader = NULL;

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

float fMaxObjectLength = 3005.0f;
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
        else // D3DPT_TRIANGLELIST
        {
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

            if (std::isinf(length) || length > fMaxObjectLength)
            {
                return D3D_OK;
            }
        }
    }

    return RealDrawPrimitive(pDevice, PrimitiveType, StartVertex, PrimitiveCount);
}

HRESULT WINAPI HookedCreatePixelShader(LPDIRECT3DDEVICE9 pDevice, const DWORD* pFunction, IDirect3DPixelShader9** ppShader)
{
    HRESULT result = RealCreatePixelShader(pDevice, pFunction, ppShader);
    if (FAILED(result))
        return result;

    if (*ppShader != nullptr)
    {
        static std::vector<uint8_t> pbFunc;
        UINT len;
        (*ppShader)->GetFunction(nullptr, &len);
        if (pbFunc.size() < len)
            pbFunc.resize(len);

        (*ppShader)->GetFunction(pbFunc.data(), &len);

        if (crc32(0, pbFunc.data(), len) == 0x15BF4BA3)
        {
            HMODULE hModule;
            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&HookedCreatePixelShader, &hModule);
            HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(IDR_BLUR), RT_RCDATA);
            if (hResource)
            {
                HGLOBAL hGlob = LoadResource(hModule, hResource);
                if (hGlob)
                {
                    LPVOID pData = LockResource(hGlob);
                    DWORD size = SizeofResource(hModule, hResource);
                    if (pData && size > 0)
                    {
                        LPDWORD shader_data = (LPDWORD)pData;
                        IDirect3DPixelShader9* newShader = nullptr;
                        HRESULT createResult = RealCreatePixelShader(pDevice, shader_data, &newShader);
                        if (SUCCEEDED(createResult))
                        {
                            (*ppShader)->Release();
                            *ppShader = newShader;
                        }
                    }
                }
            }
        }
    }

    return result;
}

HRESULT WINAPI CreateDevice(IDirect3D9* d3ddev, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface)
{
    HRESULT retval = RealD3D9CreateDevice(d3ddev, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
    UINT_PTR* pVTable = (UINT_PTR*)(*((UINT_PTR*)*ppReturnedDeviceInterface));

    RealDrawPrimitive = (DrawPrimitive_t)pVTable[IDirect3DDevice9VTBL::DrawPrimitive];
    injector::WriteMemory(&pVTable[IDirect3DDevice9VTBL::DrawPrimitive], &DrawPrimitive, true);

    RealSetStreamSource = (SetStreamSource_t)pVTable[IDirect3DDevice9VTBL::SetStreamSource];
    injector::WriteMemory(&pVTable[IDirect3DDevice9VTBL::SetStreamSource], &HookedSetStreamSource, true);

    RealCreatePixelShader = (CreatePixelShader_t)pVTable[IDirect3DDevice9VTBL::CreatePixelShader];
    injector::WriteMemory(&pVTable[IDirect3DDevice9VTBL::CreatePixelShader], &HookedCreatePixelShader, true);

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

GameRef<float> aMouseX;
GameRef<float> aMouseY;
void* pSettings = nullptr;

SafetyHookInline shCameraRotationX = {};
void __cdecl CameraRotationX(void* a1, float a2)
{
    float fMouseSensitivity = 1.0f;
    auto Settings = *(uintptr_t*)pSettings;
    if (Settings)
        fMouseSensitivity = *(float*)(Settings + 0x4DBC);
    auto deltaX = -aMouseX.get() * Screen.fRawInputMouse * fMouseSensitivity;
    return shCameraRotationX.unsafe_ccall(a1, deltaX);
}

SafetyHookInline shCameraRotationY = {};
void __cdecl CameraRotationY(void* a1, float a2)
{
    int bMouseInverted = 0;
    float fMouseSensitivity = 1.0f;
    auto Settings = *(uintptr_t*)pSettings;
    if (Settings)
    {
        bMouseInverted = *(int*)(*(uintptr_t*)pSettings + 0x4DB8);
        fMouseSensitivity = *(float*)(*(uintptr_t*)pSettings + 0x4DBC);
    }
    auto deltaY = -aMouseY.get() * Screen.fRawInputMouse * (bMouseInverted ? -fMouseSensitivity : fMouseSensitivity);
    return shCameraRotationY.unsafe_ccall(a1, deltaY);
}

SafetyHookInline shWndProc = {};
LRESULT WINAPI WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Screen.fRawInputMouse > 0.0f)
    {
        static bool bOnce = false;
        if (!bOnce)
        {
            if (Screen.Width)
            {
                RawCursorHandler<float>::Initialize(hWnd, Screen.fRawInputMouse);
                bOnce = true;
            }
        }
    }
    return shWndProc.unsafe_stdcall<LRESULT>(hWnd, Msg, wParam, lParam);
}

namespace XeTexturePacker
{
    const std::string textureCacheDir = "Textures";

    uint32_t currentTextureID;

    std::string GetHexFilename(uint32_t texID)
    {
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << texID;
        return ss.str() + ".bin";
    }

    bool GetCachedData(uint32_t texID, char* dstBuffer, size_t dataSize)
    {
        std::string filename = textureCacheDir + "/" + GetHexFilename(texID);
        std::ifstream file(filename, std::ios::binary | std::ios::in);
        if (file.is_open())
        {
            file.read(dstBuffer, dataSize);
            if (static_cast<size_t>(file.gcount()) == dataSize)
            {
                return true;
            }
        }
        return false;
    }

    struct UncompressParams
    {
        uint32_t field0;
        uint32_t field1;
        uint32_t field2;
        uint32_t field3;
        uint32_t field4;
    };

    int(__cdecl** XeTexture_UncompressTexture)(void*, int, UncompressParams*) = nullptr;

    SafetyHookInline shUncompressTexture = {};
    int __cdecl UncompressTexture(void* srcBuffer, int srcSize, void* info)
    {
        int result = 0;

        auto width = *(uint16_t*)((uintptr_t)info + 4);
        auto height = *(uint16_t*)((uintptr_t)info + 6);
        auto dstBuffer = *(char**)((uintptr_t)info + 44);
        auto dataSize = 4 * width * height;

        if (GetCachedData(currentTextureID, dstBuffer, dataSize))
        {
            return 1;
        }

        UncompressParams uncompressParams = { width, height, 0, 1, 0 };
        if ((*XeTexture_UncompressTexture)(srcBuffer, srcSize, &uncompressParams))
        {
            void* srcData = *(void**)(uncompressParams.field4 + 4);
            memcpy(dstBuffer, srcData, dataSize);
            result = 1;

            std::string dir = textureCacheDir;
            std::filesystem::create_directories(dir);
            std::string filename = dir + "/" + GetHexFilename(currentTextureID);
            std::ofstream file(filename, std::ios::binary | std::ios::out);
            if (file.is_open())
            {
                file.write(static_cast<const char*>(srcData), dataSize);
            }
        }
        currentTextureID = uint32_t(-1);
        return result;
    }

    SafetyHookInline shTEXhardwareload = {};
    void __cdecl TEXhardwareload(void* a1, uint32_t* texinfo, void* metadata, int index)
    {
        currentTextureID = texinfo[0];
        return shTEXhardwareload.unsafe_ccall(a1, texinfo, metadata, index);
    }
}

void Init()
{
    CIniReader iniReader("");
    static bool bCustomAR;
    auto szForceAspectRatio = iniReader.ReadString("MAIN", "ForceAspectRatio", "auto");
    static bool bFullscreenFMVs = iniReader.ReadInteger("MAIN", "FullscreenFMVs", 1) != 0;
    Screen.fRawInputMouse = iniReader.ReadFloat("MAIN", "RawInputMouse", 1.0f);
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

    if (Screen.fRawInputMouse > 0.0f)
    {
        auto pattern = hook::pattern("53 8B 5C 24 ? 55 8B 6C 24 ? 56 8B 74 24 ? 57 8B 7C 24 ? 8D 44 24 ? 50 56 57 53 55");
        shWndProc = safetyhook::create_inline(pattern.get_first(), WndProc);

        pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? EB 21 D8 15 ? ? ? ? DF E0 F6 C4 41 75 ? DD D8");
        aMouseX.SetAddress(*pattern.get_first<float*>(2));

        pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C3 D8 15 ? ? ? ? DF E0 F6 C4 41 75 ? DD D8 C7 05");
        aMouseY.SetAddress(*pattern.get_first<float*>(2));

        pattern = hook::pattern("8B 0D ? ? ? ? 8B 91 ? ? ? ? 89 95 ? ? ? ? 8B 85 ? ? ? ? 83 E8 06");
        pSettings = *pattern.get_first<void*>(2);

        pattern = hook::pattern("55 8B EC 83 EC 34 D9 05 ? ? ? ? D9 45 ? DA E9 DF E0 F6 C4 44");
        shCameraRotationY = safetyhook::create_inline(pattern.count(6).get(0).get<void*>(0), CameraRotationY);
        shCameraRotationX = safetyhook::create_inline(pattern.count(6).get(5).get<void*>(0), CameraRotationX);

        static auto SetCursor = [](float* pMouseX, float* pMouseY)
        {
            RECT clientRect;
            GetClientRect(RawCursorHandler<float>::UpdateMouseInput(true), &clientRect);

            // Normalize to 0.0 - 1.0 range
            float clientWidth = static_cast<float>(clientRect.right - clientRect.left);
            float clientHeight = static_cast<float>(clientRect.bottom - clientRect.top);

            *pMouseX = RawCursorHandler<float>::MouseCursorX / clientWidth;
            *pMouseY = RawCursorHandler<float>::MouseCursorY / clientHeight;
        };

        pattern = hook::pattern("8B 55 ? 8B 82 ? ? ? ? 89 45 ? D9 45 ? D8 1D ? ? ? ? DF E0 F6 C4 41 75 ? C7 45 ? ? ? ? ? EB 1F D9 45 ? D8 1D ? ? ? ? DF E0 F6 C4 05 7A ? C7 45 ? ? ? ? ? EB 06 8B 4D ? 89 4D ? 8B 55 ? 8B 45 ? 89 82 ? ? ? ? 8B 4D");
        static auto MouseCursorHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            SetCursor((float*)(regs.eax + 0x1D4), (float*)(regs.eax + 0x1D8));
        });

        pattern = hook::pattern("8B 45 ? 8B 88 ? ? ? ? 89 4D ? D9 45 ? D8 1D ? ? ? ? DF E0 F6 C4 41 75 ? C7 45 ? ? ? ? ? EB 1F D9 45 ? D8 1D ? ? ? ? DF E0 F6 C4 05 7A ? C7 45 ? ? ? ? ? EB 06 8B 55 ? 89 55 ? 8B 45 ? 8B 4D ? 89 88 ? ? ? ? 8B 55");
        static int32_t offset = *pattern.get_first<int32_t>(-4);
        static auto MouseCursorHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            SetCursor((float*)(regs.eax + offset - 4), (float*)(regs.eax + offset));
        });
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
    if (!pattern.empty()) // Gamer's Edition
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
        if (!pattern.empty()) // 9d
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

            pattern = hook::pattern("39 6E ? 75 ? 8B 4E");
            static auto WindowedHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (bWindowedMode)
                {
                    *(uint32_t*)(regs.esi + 0x10) = 1;
                    regs.ebp = 1;
                }
            });
        }
        else
        {
            pattern = hook::pattern("8B 56 68 8B 44 24 44");
            if (!pattern.empty()) // 9
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

                pattern = hook::pattern("8B 46 ? 8D 4C 24 ? 51 8B 4E");
                static auto WindowedHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    if (bWindowedMode)
                    {
                        *(uint32_t*)(regs.esi + 0x10) = 1;
                        regs.eax = 1;
                    }
                });
            }
            else
            {
                pattern = hook::pattern("8B 4E ? 8B 56 ? 53");
                if (!pattern.empty()) // 8
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

                pattern = hook::pattern("8B 46 ? 8D 54 24 ? 52 8B 16");
                static auto WindowedHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    if (bWindowedMode)
                    {
                        *(uint32_t*)(regs.esi + 0x10) = 1;
                        regs.eax = 1;
                    }
                });
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

    //grass at screen edges
    static float f0 = 0.0f;
    pattern = hook::pattern("D8 0D ? ? ? ? D9 C0 D9 FF D9 9C 24");
    if (!pattern.empty())
        injector::WriteMemory(pattern.get_first(2), &f0, true);
    else
    {
        pattern = hook::pattern("D8 0D ? ? ? ? D9 9D ? ? ? ? D9 85 ? ? ? ? 83 EC 08 DD 1C 24 E8 ? ? ? ? 83 C4 08 D9 9D ? ? ? ? D9 85 ? ? ? ? D8 3D ? ? ? ? D9 9D ? ? ? ? 8B 85 ? ? ? ? D9 05 ? ? ? ? D9 80 ? ? ? ? DA E9 DF E0 F6 C4 44 7A ? 8B 8D ? ? ? ? C7 81 ? ? ? ? ? ? ? ? 8B 15 ? ? ? ? 89 95 ? ? ? ? A1 ? ? ? ? 6B C0 18");
        injector::WriteMemory(pattern.get_first(2), &f0, true);
    }

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

    pattern = find_pattern("74 ? FF D0 85 C0");
    if (!pattern.empty())
    {
        injector::WriteMemory<uint8_t>(pattern.get_first(), 0xEB, true);
    }

    // Texture caching for Gamer's Edition
    pattern = hook::pattern("FF 15 ? ? ? ? 83 C4 0C 85 C0 0F 95 C0");
    if (!pattern.empty())
    {
        XeTexturePacker::XeTexture_UncompressTexture = *(decltype(XeTexturePacker::XeTexture_UncompressTexture)*)pattern.get_first(2);

        pattern = hook::pattern("53 55 56 6A 14");
        XeTexturePacker::shUncompressTexture = safetyhook::create_inline(pattern.get_first(), XeTexturePacker::UncompressTexture);

        pattern = hook::pattern("55 8B EC 83 E4 F8 81 EC B4 00 00 00 53");
        XeTexturePacker::shTEXhardwareload = safetyhook::create_inline(pattern.get_first(), XeTexturePacker::TEXhardwareload);
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