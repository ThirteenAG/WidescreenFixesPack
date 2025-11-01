module;

#include "stdafx.h"
#include <d3dx9.h>

export module WindowedMode;

import <map>;
import <functional>;
import <numeric>;
import <algorithm>;
import <functional>;
import <typeindex>;
import <typeinfo>;
import <vector>;
import <string>;
import <list>;
import <thread>;

class VTBL
{
private:
    virtual std::vector<const char*>& GetMethods() = 0;
public:
    const auto GetNumberOfMethods() {
        return GetMethods().size();
    }
    const auto GetIndex(const char* name) {
        auto iter = std::find(std::begin(GetMethods()), std::end(GetMethods()), name);
        if (iter != std::end(GetMethods()))
            return (int32_t)std::distance(std::begin(GetMethods()), iter);
        else
            return (int32_t)-1;
    }
    const auto GetMethod(int32_t index) {
        if (index < static_cast<int32_t>(GetNumberOfMethods()))
            return GetMethods()[index];
        else
            return "";
    }
};

class IDirect3D9VTBL : public VTBL
{
private:
    std::vector<const char*>& GetMethods() override {
        static std::vector<const char*> methodsNames{
            "QueryInterface",
            "AddRef",
            "Release",
            "RegisterSoftwareDevice",
            "GetAdapterCount",
            "GetAdapterIdentifier",
            "GetAdapterModeCount",
            "EnumAdapterModes",
            "GetAdapterDisplayMode",
            "CheckDeviceType",
            "CheckDeviceFormat",
            "CheckDeviceMultiSampleType",
            "CheckDepthStencilMatch",
            "CheckDeviceFormatConversion",
            "GetDeviceCaps",
            "GetAdapterMonitor",
            "CreateDevice",
        };
        return methodsNames;
    }
};

class IDirect3DDevice9VTBL : public VTBL
{
private:
    std::vector<const char*>& GetMethods() override {
        static std::vector<const char*> methodsNames{
            "QueryInterface",
            "AddRef",
            "Release",
            "TestCooperativeLevel",
            "GetAvailableTextureMem",
            "EvictManagedResources",
            "GetDirect3D",
            "GetDeviceCaps",
            "GetDisplayMode",
            "GetCreationParameters",
            "SetCursorProperties",
            "SetCursorPosition",
            "ShowCursor",
            "CreateAdditionalSwapChain",
            "GetSwapChain",
            "GetNumberOfSwapChains",
            "Reset",
            "Present",
            "GetBackBuffer",
            "GetRasterStatus",
            "SetDialogBoxMode",
            "SetGammaRamp",
            "GetGammaRamp",
            "CreateTexture",
            "CreateVolumeTexture",
            "CreateCubeTexture",
            "CreateVertexBuffer",
            "CreateIndexBuffer",
            "CreateRenderTarget",
            "CreateDepthStencilSurface",
            "UpdateSurface",
            "UpdateTexture",
            "GetRenderTargetData",
            "GetFrontBufferData",
            "StretchRect",
            "ColorFill",
            "CreateOffscreenPlainSurface",
            "SetRenderTarget",
            "GetRenderTarget",
            "SetDepthStencilSurface",
            "GetDepthStencilSurface",
            "BeginScene",
            "EndScene",
            "Clear",
            "SetTransform",
            "GetTransform",
            "MultiplyTransform",
            "SetViewport",
            "GetViewport",
            "SetMaterial",
            "GetMaterial",
            "SetLight",
            "GetLight",
            "LightEnable",
            "GetLightEnable",
            "SetClipPlane",
            "GetClipPlane",
            "SetRenderState",
            "GetRenderState",
            "CreateStateBlock",
            "BeginStateBlock",
            "EndStateBlock",
            "SetClipStatus",
            "GetClipStatus",
            "GetTexture",
            "SetTexture",
            "GetTextureStageState",
            "SetTextureStageState",
            "GetSamplerState",
            "SetSamplerState",
            "ValidateDevice",
            "SetPaletteEntries",
            "GetPaletteEntries",
            "SetCurrentTexturePalette",
            "GetCurrentTexturePalette",
            "SetScissorRect",
            "GetScissorRect",
            "SetSoftwareVertexProcessing",
            "GetSoftwareVertexProcessing",
            "SetNPatchMode",
            "GetNPatchMode",
            "DrawPrimitive",
            "DrawIndexedPrimitive",
            "DrawPrimitiveUP",
            "DrawIndexedPrimitiveUP",
            "ProcessVertices",
            "CreateVertexDeclaration",
            "SetVertexDeclaration",
            "GetVertexDeclaration",
            "SetFVF",
            "GetFVF",
            "CreateVertexShader",
            "SetVertexShader",
            "GetVertexShader",
            "SetVertexShaderConstantF",
            "GetVertexShaderConstantF",
            "SetVertexShaderConstantI",
            "GetVertexShaderConstantI",
            "SetVertexShaderConstantB",
            "GetVertexShaderConstantB",
            "SetStreamSource",
            "GetStreamSource",
            "SetStreamSourceFreq",
            "GetStreamSourceFreq",
            "SetIndices",
            "GetIndices",
            "CreatePixelShader",
            "SetPixelShader",
            "GetPixelShader",
            "SetPixelShaderConstantF",
            "GetPixelShaderConstantF",
            "SetPixelShaderConstantI",
            "GetPixelShaderConstantI",
            "SetPixelShaderConstantB",
            "GetPixelShaderConstantB",
            "DrawRectPatch",
            "DrawTriPatch",
            "DeletePatch",
            "CreateQuery",
            "SetConvolutionMonoKernel",
            "ComposeRects",
            "PresentEx",
            "GetGPUThreadPriority",
            "SetGPUThreadPriority",
            "WaitForVBlank",
            "CheckResourceResidency",
            "SetMaximumFrameLatency",
            "GetMaximumFrameLatency",
            "CheckDeviceState",
            "CreateRenderTargetEx",
            "CreateOffscreenPlainSurfaceEx",
            "CreateDepthStencilSurfaceEx",
            "ResetEx",
            "GetDisplayModeEx",
        };
        return methodsNames;
    }
};

export SafetyHookInline CreateDeviceOriginal = {};
export SafetyHookInline BeginSceneOriginal = {};
export SafetyHookInline EndSceneOriginal = {};
export SafetyHookInline ResetOriginal = {};
export SafetyHookInline ResetExOriginal = {};
export SafetyHookInline PresentOriginal = {};
export SafetyHookInline PresentExOriginal = {};
export SafetyHookInline SetVertexShaderConstantFOriginal = {};
export SafetyHookInline SetPixelShaderConstantFOriginal = {};
export SafetyHookInline CreateTextureOriginal = {};
export SafetyHookInline SetTextureOriginal = {};
export SafetyHookInline DrawPrimitiveOriginal = {};
export SafetyHookInline SetPixelShaderOriginal = {};
export SafetyHookInline SetVertexShaderOriginal = {};
export SafetyHookInline CreatePixelShaderOriginal = {};
export SafetyHookInline CreateVertexShaderOriginal = {};

class FusionDxHook
{
private:
    static inline std::map<const HMODULE, std::map<std::type_index, std::vector<uintptr_t*>>> deviceMethods;
    static inline std::vector<std::pair<WORD, ULONG_PTR>> WndProcList;

    static inline HWND g_hFocusWindow = NULL;
    static inline HMODULE g_hWrapperModule = NULL;

    static inline bool bForceWindowedMode = true;
    static inline bool bUsePrimaryMonitor = false;
    static inline bool bCenterWindow = true;
    static inline bool bAlwaysOnTop = false;
    static inline bool bDoNotNotifyOnTaskSwitch = false;
    static inline int nForceWindowStyle = 0; // 0: normal windowed, 1: borderless fullscreen
    static inline int nFullScreenRefreshRateInHz = 0;

    typedef BOOL(WINAPI* LPFN_ISTOPLEVELWINDOW)(HWND);
    static inline LPFN_ISTOPLEVELWINDOW fnIsTopLevelWindow = NULL;

    static BOOL _fnIsTopLevelWindow(HWND hWnd)
    {
        /*  IsTopLevelWindow is not available on all versions of Windows.
         *  Use GetModuleHandle to get a handle to the DLL that contains the function
         *  and GetProcAddress to get a pointer to the function if available.
         */
        if (fnIsTopLevelWindow == NULL)
        {
            fnIsTopLevelWindow = (LPFN_ISTOPLEVELWINDOW)GetProcAddress(GetModuleHandleA("user32"), "IsTopLevelWindow");
            if (fnIsTopLevelWindow == NULL)
            {
                fnIsTopLevelWindow = (LPFN_ISTOPLEVELWINDOW)-1;
            }
        }
        if (fnIsTopLevelWindow != (LPFN_ISTOPLEVELWINDOW)-1)
        {
            return fnIsTopLevelWindow(hWnd);
        }
        /* If no avail, use older method which is available in Win2000+ */
        return (GetAncestor(hWnd, GA_ROOT) == hWnd);
    }

    static LRESULT WINAPI CustomWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, int idx)
    {
        if (hWnd == g_hFocusWindow || _fnIsTopLevelWindow(hWnd)) // skip child windows like buttons, edit boxes, etc.
        {
            if (bAlwaysOnTop)
            {
                if ((GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST) == 0)
                    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
            }
            switch (uMsg)
            {
            case WM_ACTIVATE:
                if (bDoNotNotifyOnTaskSwitch && LOWORD(wParam) == WA_INACTIVE)
                {
                    if ((HWND)lParam == NULL)
                        return 0;
                    DWORD dwPID = 0;
                    GetWindowThreadProcessId((HWND)lParam, &dwPID);
                    if (dwPID != GetCurrentProcessId())
                        return 0;
                }
                break;
            case WM_NCACTIVATE:
                if (bDoNotNotifyOnTaskSwitch && LOWORD(wParam) == WA_INACTIVE)
                    return 0;
                break;
            case WM_ACTIVATEAPP:
                if (bDoNotNotifyOnTaskSwitch && wParam == FALSE)
                    return 0;
                break;
            case WM_KILLFOCUS:
                if (bDoNotNotifyOnTaskSwitch)
                {
                    if ((HWND)wParam == NULL)
                        return 0;
                    DWORD dwPID = 0;
                    GetWindowThreadProcessId((HWND)wParam, &dwPID);
                    if (dwPID != GetCurrentProcessId())
                        return 0;
                }
                break;
            case WM_SETFOCUS:
            case WM_MOUSEACTIVATE:
                break;
            default:
                break;
            }
        }
        WNDPROC OrigProc = WNDPROC(WndProcList[idx].second);
        return OrigProc(hWnd, uMsg, wParam, lParam);
    }

    static LRESULT WINAPI CustomWndProcA(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        WORD wClassAtom = GetClassWord(hWnd, GCW_ATOM);
        if (wClassAtom)
        {
            for (unsigned int i = 0; i < WndProcList.size(); i++)
            {
                if (WndProcList[i].first == wClassAtom)
                {
                    return CustomWndProc(hWnd, uMsg, wParam, lParam, i);
                }
            }
        }
        // We should never reach here, but having safeguards anyway is good
        return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }

    static LRESULT WINAPI CustomWndProcW(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        WORD wClassAtom = GetClassWord(hWnd, GCW_ATOM);
        if (wClassAtom)
        {
            for (unsigned int i = 0; i < WndProcList.size(); i++)
            {
                if (WndProcList[i].first == wClassAtom)
                {
                    return CustomWndProc(hWnd, uMsg, wParam, lParam, i);
                }
            }
        }
        // We should never reach here, but having safeguards anyway is good
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    typedef ATOM(__stdcall* RegisterClassA_fn)(const WNDCLASSA*);
    typedef ATOM(__stdcall* RegisterClassW_fn)(const WNDCLASSW*);
    typedef ATOM(__stdcall* RegisterClassExA_fn)(const WNDCLASSEXA*);
    typedef ATOM(__stdcall* RegisterClassExW_fn)(const WNDCLASSEXW*);
    static inline RegisterClassA_fn oRegisterClassA = NULL;
    static inline RegisterClassW_fn oRegisterClassW = NULL;
    static inline RegisterClassExA_fn oRegisterClassExA = NULL;
    static inline RegisterClassExW_fn oRegisterClassExW = NULL;

    static inline BOOL IsValueIntAtom(DWORD dw)
    {
        return (HIWORD(dw) == 0 && LOWORD(dw) < 0xC000);
    }

    static BOOL IsSystemClassNameA(LPCSTR classNameA)
    {
        if (!lstrcmpiA(classNameA, "BUTTON"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "COMBOBOX"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "EDIT"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "LISTBOX"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "MDICLIENT"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "RICHEDIT"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "RICHEDIT_CLASS"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "SCROLLBAR"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "STATIC"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "ANIMATE_CLASS"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "DATETIMEPICK_CLASS"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "HOTKEY_CLASS"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "LINK_CLASS"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "MONTHCAL_CLASS"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "NATIVEFNTCTL_CLASS"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "PROGRESS_CLASS"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "REBARCLASSNAME"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "STANDARD_CLASSES"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "STATUSCLASSNAME"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "TOOLBARCLASSNAME"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "TOOLTIPS_CLASS"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "TRACKBAR_CLASS"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "UPDOWN_CLASS"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "WC_BUTTON"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "WC_COMBOBOX"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "WC_COMBOBOXEX"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "WC_EDIT"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "WC_HEADER"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "WC_LISTBOX"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "WC_IPADDRESS"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "WC_LINK"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "WC_LISTVIEW"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "WC_NATIVEFONTCTL"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "WC_PAGESCROLLER"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "WC_SCROLLBAR"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "WC_STATIC"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "WC_TABCONTROL"))
            return TRUE;
        if (!lstrcmpiA(classNameA, "WC_TREEVIEW"))
            return TRUE;
        return FALSE;
    }

    static BOOL IsSystemClassNameW(LPCWSTR classNameW)
    {
        if (!lstrcmpiW(classNameW, L"BUTTON"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"COMBOBOX"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"EDIT"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"LISTBOX"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"MDICLIENT"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"RICHEDIT"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"RICHEDIT_CLASS"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"SCROLLBAR"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"STATIC"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"ANIMATE_CLASS"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"DATETIMEPICK_CLASS"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"HOTKEY_CLASS"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"LINK_CLASS"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"MONTHCAL_CLASS"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"NATIVEFNTCTL_CLASS"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"PROGRESS_CLASS"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"REBARCLASSNAME"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"STANDARD_CLASSES"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"STATUSCLASSNAME"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"TOOLBARCLASSNAME"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"TOOLTIPS_CLASS"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"TRACKBAR_CLASS"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"UPDOWN_CLASS"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"WC_BUTTON"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"WC_COMBOBOX"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"WC_COMBOBOXEX"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"WC_EDIT"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"WC_HEADER"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"WC_LISTBOX"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"WC_IPADDRESS"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"WC_LINK"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"WC_LISTVIEW"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"WC_NATIVEFONTCTL"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"WC_PAGESCROLLER"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"WC_SCROLLBAR"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"WC_STATIC"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"WC_TABCONTROL"))
            return TRUE;
        if (!lstrcmpiW(classNameW, L"WC_TREEVIEW"))
            return TRUE;
        return FALSE;
    }

    static ATOM __stdcall hk_RegisterClassA(WNDCLASSA* lpWndClass)
    {
        if (!IsValueIntAtom(DWORD(lpWndClass->lpszClassName)))
        { // argument is a class name
            if (IsSystemClassNameA(lpWndClass->lpszClassName))
            { // skip system classes like buttons, common controls, etc.
                return oRegisterClassA(lpWndClass);
            }
        }
        ULONG_PTR pWndProc = ULONG_PTR(lpWndClass->lpfnWndProc);
        lpWndClass->lpfnWndProc = CustomWndProcA;
        WORD wClassAtom = oRegisterClassA(lpWndClass);
        if (wClassAtom != 0)
        {
            WndProcList.emplace_back(wClassAtom, pWndProc);
        }
        return wClassAtom;
    }
    static ATOM __stdcall hk_RegisterClassW(WNDCLASSW* lpWndClass)
    {
        if (!IsValueIntAtom(DWORD(lpWndClass->lpszClassName)))
        { // argument is a class name
            if (IsSystemClassNameW(lpWndClass->lpszClassName))
            { // skip system classes like buttons, common controls, etc.
                return oRegisterClassW(lpWndClass);
            }
        }
        ULONG_PTR pWndProc = ULONG_PTR(lpWndClass->lpfnWndProc);
        lpWndClass->lpfnWndProc = CustomWndProcW;
        WORD wClassAtom = oRegisterClassW(lpWndClass);
        if (wClassAtom != 0)
        {
            WndProcList.emplace_back(wClassAtom, pWndProc);
        }
        return wClassAtom;
    }
    static ATOM __stdcall hk_RegisterClassExA(WNDCLASSEXA* lpWndClass)
    {
        if (!IsValueIntAtom(DWORD(lpWndClass->lpszClassName)))
        { // argument is a class name
            if (IsSystemClassNameA(lpWndClass->lpszClassName))
            { // skip system classes like buttons, common controls, etc.
                return oRegisterClassExA(lpWndClass);
            }
        }
        ULONG_PTR pWndProc = ULONG_PTR(lpWndClass->lpfnWndProc);
        lpWndClass->lpfnWndProc = CustomWndProcA;
        WORD wClassAtom = oRegisterClassExA(lpWndClass);
        if (wClassAtom != 0)
        {
            WndProcList.emplace_back(wClassAtom, pWndProc);
        }
        return wClassAtom;
    }
    static ATOM __stdcall hk_RegisterClassExW(WNDCLASSEXW* lpWndClass)
    {
        if (!IsValueIntAtom(DWORD(lpWndClass->lpszClassName)))
        { // argument is a class name
            if (IsSystemClassNameW(lpWndClass->lpszClassName))
            { // skip system classes like buttons, common controls, etc.
                return oRegisterClassExW(lpWndClass);
            }
        }
        ULONG_PTR pWndProc = ULONG_PTR(lpWndClass->lpfnWndProc);
        lpWndClass->lpfnWndProc = CustomWndProcW;
        WORD wClassAtom = oRegisterClassExW(lpWndClass);
        if (wClassAtom != 0)
        {
            WndProcList.emplace_back(wClassAtom, pWndProc);
        }
        return wClassAtom;
    }

    typedef HWND(__stdcall* GetForegroundWindow_fn)(void);
    static inline GetForegroundWindow_fn oGetForegroundWindow = NULL;

    static HWND __stdcall hk_GetForegroundWindow()
    {
        if (g_hFocusWindow && IsWindow(g_hFocusWindow))
            return g_hFocusWindow;
        return oGetForegroundWindow();
    }

    typedef HWND(__stdcall* GetActiveWindow_fn)(void);
    static inline GetActiveWindow_fn oGetActiveWindow = NULL;

    static HWND __stdcall hk_GetActiveWindow(void)
    {
        HWND hWndActive = oGetActiveWindow();
        if (g_hFocusWindow && hWndActive == NULL && IsWindow(g_hFocusWindow))
        {
            if (GetCurrentThreadId() == GetWindowThreadProcessId(g_hFocusWindow, NULL))
                return g_hFocusWindow;
        }
        return hWndActive;
    }

    typedef HWND(__stdcall* GetFocus_fn)(void);
    static inline GetFocus_fn oGetFocus = NULL;

    static HWND __stdcall hk_GetFocus(void)
    {
        HWND hWndFocus = oGetFocus();
        if (g_hFocusWindow && hWndFocus == NULL && IsWindow(g_hFocusWindow))
        {
            if (GetCurrentThreadId() == GetWindowThreadProcessId(g_hFocusWindow, NULL))
                return g_hFocusWindow;
        }
        return hWndFocus;
    }

    typedef HMODULE(__stdcall* LoadLibraryA_fn)(LPCSTR lpLibFileName);
    static inline LoadLibraryA_fn oLoadLibraryA;

    static HMODULE __stdcall hk_LoadLibraryA(LPCSTR lpLibFileName)
    {
        HMODULE hmod = oLoadLibraryA(lpLibFileName);
        if (hmod)
        {
            HookModule(hmod);
        }
        return hmod;
    }

    typedef HMODULE(__stdcall* LoadLibraryW_fn)(LPCWSTR lpLibFileName);
    static inline LoadLibraryW_fn oLoadLibraryW;

    static HMODULE __stdcall hk_LoadLibraryW(LPCWSTR lpLibFileName)
    {
        HMODULE hmod = oLoadLibraryW(lpLibFileName);
        if (hmod)
        {
            HookModule(hmod);
        }
        return hmod;
    }

    typedef HMODULE(__stdcall* LoadLibraryExA_fn)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
    static inline LoadLibraryExA_fn oLoadLibraryExA;

    static HMODULE __stdcall hk_LoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
    {
        HMODULE hmod = oLoadLibraryExA(lpLibFileName, hFile, dwFlags);
        if (hmod && ((dwFlags & (LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE | LOAD_LIBRARY_AS_IMAGE_RESOURCE)) == 0))
        {
            HookModule(hmod);
        }
        return hmod;
    }

    typedef HMODULE(__stdcall* LoadLibraryExW_fn)(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
    static inline LoadLibraryExW_fn oLoadLibraryExW;

    static HMODULE __stdcall hk_LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
    {
        HMODULE hmod = oLoadLibraryExW(lpLibFileName, hFile, dwFlags);
        if (hmod && ((dwFlags & (LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE | LOAD_LIBRARY_AS_IMAGE_RESOURCE)) == 0))
        {
            HookModule(hmod);
        }
        return hmod;
    }

    typedef BOOL(__stdcall* FreeLibrary_fn)(HMODULE hLibModule);
    static inline FreeLibrary_fn oFreeLibrary;

    static BOOL __stdcall hk_FreeLibrary(HMODULE hLibModule)
    {
        if (hLibModule == g_hWrapperModule)
            return TRUE; // We will stay in memory, thank you very much

        return oFreeLibrary(hLibModule);
    }

    static FARPROC __stdcall hk_GetProcAddress(HMODULE hModule, LPCSTR lpProcName)
    {
        __try
        {
            if (!lstrcmpA(lpProcName, "RegisterClassA"))
            {
                if (oRegisterClassA == NULL)
                    oRegisterClassA = (RegisterClassA_fn)GetProcAddress(hModule, lpProcName);
                return (FARPROC)hk_RegisterClassA;
            }
            if (!lstrcmpA(lpProcName, "RegisterClassW"))
            {
                if (oRegisterClassW == NULL)
                    oRegisterClassW = (RegisterClassW_fn)GetProcAddress(hModule, lpProcName);
                return (FARPROC)hk_RegisterClassW;
            }
            if (!lstrcmpA(lpProcName, "RegisterClassExA"))
            {
                if (oRegisterClassExA == NULL)
                    oRegisterClassExA = (RegisterClassExA_fn)GetProcAddress(hModule, lpProcName);
                return (FARPROC)hk_RegisterClassExA;
            }
            if (!lstrcmpA(lpProcName, "RegisterClassExW"))
            {
                if (oRegisterClassExW == NULL)
                    oRegisterClassExW = (RegisterClassExW_fn)GetProcAddress(hModule, lpProcName);
                return (FARPROC)hk_RegisterClassExW;
            }
            if (!lstrcmpA(lpProcName, "GetForegroundWindow"))
            {
                if (oGetForegroundWindow == NULL)
                    oGetForegroundWindow = (GetForegroundWindow_fn)GetProcAddress(hModule, lpProcName);
                return (FARPROC)hk_GetForegroundWindow;
            }
            if (!lstrcmpA(lpProcName, "GetActiveWindow"))
            {
                if (oGetActiveWindow == NULL)
                    oGetActiveWindow = (GetActiveWindow_fn)GetProcAddress(hModule, lpProcName);
                return (FARPROC)hk_GetActiveWindow;
            }
            if (!lstrcmpA(lpProcName, "GetFocus"))
            {
                if (oGetFocus == NULL)
                    oGetFocus = (GetFocus_fn)GetProcAddress(hModule, lpProcName);
                return (FARPROC)hk_GetFocus;
            }
            if (!lstrcmpA(lpProcName, "LoadLibraryA"))
            {
                if (oLoadLibraryA == NULL)
                    oLoadLibraryA = (LoadLibraryA_fn)GetProcAddress(hModule, lpProcName);
                return (FARPROC)hk_LoadLibraryA;
            }
            if (!lstrcmpA(lpProcName, "LoadLibraryW"))
            {
                if (oLoadLibraryW == NULL)
                    oLoadLibraryW = (LoadLibraryW_fn)GetProcAddress(hModule, lpProcName);
                return (FARPROC)hk_LoadLibraryW;
            }
            if (!lstrcmpA(lpProcName, "LoadLibraryExA"))
            {
                if (oLoadLibraryExA == NULL)
                    oLoadLibraryExA = (LoadLibraryExA_fn)GetProcAddress(hModule, lpProcName);
                return (FARPROC)hk_LoadLibraryExA;
            }
            if (!lstrcmpA(lpProcName, "LoadLibraryExW"))
            {
                if (oLoadLibraryExW == NULL)
                    oLoadLibraryExW = (LoadLibraryExW_fn)GetProcAddress(hModule, lpProcName);
                return (FARPROC)hk_LoadLibraryExW;
            }
            if (!lstrcmpA(lpProcName, "FreeLibrary"))
            {
                if (oFreeLibrary == NULL)
                    oFreeLibrary = (FreeLibrary_fn)GetProcAddress(hModule, lpProcName);
                return (FARPROC)hk_FreeLibrary;
            }
        }
        __except ((GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
        }

        return GetProcAddress(hModule, lpProcName);
    }

    static inline char WinDir[MAX_PATH + 1];
    static void HookModule(HMODULE hmod)
    {
        char modpath[MAX_PATH + 1];
        if (hmod == g_hWrapperModule) // don't hook ourselves
            return;

        if (GetModuleFileNameA(hmod, modpath, MAX_PATH))
        {
            if (!_strnicmp(modpath, WinDir, strlen(WinDir)))
            { // skip system modules
                return;
            }
        }

        // user32.dll imports
        auto originalsUser32 = IATHook::Replace(
            hmod, "user32.dll",
            std::make_tuple("RegisterClassA", (void*)hk_RegisterClassA),
            std::make_tuple("RegisterClassW", (void*)hk_RegisterClassW),
            std::make_tuple("RegisterClassExA", (void*)hk_RegisterClassExA),
            std::make_tuple("RegisterClassExW", (void*)hk_RegisterClassExW),
            std::make_tuple("GetForegroundWindow", (void*)hk_GetForegroundWindow),
            std::make_tuple("GetActiveWindow", (void*)hk_GetActiveWindow),
            std::make_tuple("GetFocus", (void*)hk_GetFocus)
        );

        if (oRegisterClassA == NULL)
        {
            auto it = originalsUser32.find("RegisterClassA");   if (it != originalsUser32.end())  oRegisterClassA = (RegisterClassA_fn)it->second.get();
        }
        if (oRegisterClassW == NULL)
        {
            auto it = originalsUser32.find("RegisterClassW");   if (it != originalsUser32.end())  oRegisterClassW = (RegisterClassW_fn)it->second.get();
        }
        if (oRegisterClassExA == NULL)
        {
            auto it = originalsUser32.find("RegisterClassExA"); if (it != originalsUser32.end())  oRegisterClassExA = (RegisterClassExA_fn)it->second.get();
        }
        if (oRegisterClassExW == NULL)
        {
            auto it = originalsUser32.find("RegisterClassExW"); if (it != originalsUser32.end())  oRegisterClassExW = (RegisterClassExW_fn)it->second.get();
        }
        if (oGetForegroundWindow == NULL)
        {
            auto it = originalsUser32.find("GetForegroundWindow"); if (it != originalsUser32.end()) oGetForegroundWindow = (GetForegroundWindow_fn)it->second.get();
        }
        if (oGetActiveWindow == NULL)
        {
            auto it = originalsUser32.find("GetActiveWindow");     if (it != originalsUser32.end()) oGetActiveWindow = (GetActiveWindow_fn)it->second.get();
        }
        if (oGetFocus == NULL)
        {
            auto it = originalsUser32.find("GetFocus");            if (it != originalsUser32.end()) oGetFocus = (GetFocus_fn)it->second.get();
        }

        // kernel32.dll imports
        auto originalsKernel32 = IATHook::Replace(
            hmod, "kernel32.dll",
            std::make_tuple("LoadLibraryA", (void*)hk_LoadLibraryA),
            std::make_tuple("LoadLibraryW", (void*)hk_LoadLibraryW),
            std::make_tuple("LoadLibraryExA", (void*)hk_LoadLibraryExA),
            std::make_tuple("LoadLibraryExW", (void*)hk_LoadLibraryExW),
            std::make_tuple("FreeLibrary", (void*)hk_FreeLibrary),
            std::make_tuple("GetProcAddress", (void*)hk_GetProcAddress)
        );

        if (oLoadLibraryA == NULL)
        {
            auto it = originalsKernel32.find("LoadLibraryA");   if (it != originalsKernel32.end()) oLoadLibraryA = (LoadLibraryA_fn)it->second.get();
        }
        if (oLoadLibraryW == NULL)
        {
            auto it = originalsKernel32.find("LoadLibraryW");   if (it != originalsKernel32.end()) oLoadLibraryW = (LoadLibraryW_fn)it->second.get();
        }
        if (oLoadLibraryExA == NULL)
        {
            auto it = originalsKernel32.find("LoadLibraryExA"); if (it != originalsKernel32.end()) oLoadLibraryExA = (LoadLibraryExA_fn)it->second.get();
        }
        if (oLoadLibraryExW == NULL)
        {
            auto it = originalsKernel32.find("LoadLibraryExW"); if (it != originalsKernel32.end()) oLoadLibraryExW = (LoadLibraryExW_fn)it->second.get();
        }
        if (oFreeLibrary == NULL)
        {
            auto it = originalsKernel32.find("FreeLibrary");    if (it != originalsKernel32.end()) oFreeLibrary = (FreeLibrary_fn)it->second.get();
        }
    }

    static void HookImportedModules()
    {
        HMODULE hModule;
        HMODULE hm;

        hModule = GetModuleHandle(0);

        PIMAGE_DOS_HEADER img_dos_headers = (PIMAGE_DOS_HEADER)hModule;
        PIMAGE_NT_HEADERS img_nt_headers = (PIMAGE_NT_HEADERS)((BYTE*)img_dos_headers + img_dos_headers->e_lfanew);
        PIMAGE_IMPORT_DESCRIPTOR img_import_desc = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)img_dos_headers + img_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
        if (img_dos_headers->e_magic != IMAGE_DOS_SIGNATURE)
            return;

        for (IMAGE_IMPORT_DESCRIPTOR* iid = img_import_desc; iid->Name != 0; iid++)
        {
            char* mod_name = (char*)((size_t*)(iid->Name + (size_t)hModule));
            hm = GetModuleHandleA(mod_name);
            // ual check
            if (hm && !(GetProcAddress(hm, "DirectInput8Create") != NULL && GetProcAddress(hm, "DirectSoundCreate8") != NULL && GetProcAddress(hm, "InternetOpenA") != NULL))
            {
                HookModule(hm);
            }
        }
    }

    class HookWindow
    {
    public:
        HookWindow(std::wstring_view className = L"FusionDxHook", std::wstring windowName = L"FusionDxHook")
        {
            windowClass.cbSize = sizeof(WNDCLASSEX);
            windowClass.style = CS_HREDRAW | CS_VREDRAW;
            windowClass.lpfnWndProc = DefWindowProc;
            windowClass.cbClsExtra = 0;
            windowClass.cbWndExtra = 0;
            windowClass.hInstance = GetModuleHandle(NULL);
            windowClass.hIcon = NULL;
            windowClass.hCursor = NULL;
            windowClass.hbrBackground = NULL;
            windowClass.lpszMenuName = NULL;
            windowClass.lpszClassName = className.data();
            windowClass.hIconSm = NULL;
            RegisterClassExW(&windowClass);
            window = CreateWindowW(windowClass.lpszClassName, windowName.data(), WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, windowClass.hInstance, NULL);
        }
        ~HookWindow()
        {
            DestroyWindow(window);
            UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
        }
        static HWND GetHookWindow(HookWindow& hw)
        {
            return hw.window;
        }
    private:
        WNDCLASSEX windowClass{};
        HWND window{};
    };

    template <class... Ts>
    static inline void copyMethods(HMODULE mod, Ts&& ... inputs)
    {
        ([&] {
            auto vtbl_info = std::get<0>(inputs);
            auto ptr = std::get<1>(inputs);
            deviceMethods[mod][typeid(vtbl_info)].resize(vtbl_info.GetNumberOfMethods());
            std::memcpy(deviceMethods.at(mod).at(typeid(vtbl_info)).data(), ptr, vtbl_info.GetNumberOfMethods() * sizeof(uintptr_t));
        } (), ...);
    }

    static void ForceWindowed(D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode = NULL)
    {
        HWND hwnd = pPresentationParameters->hDeviceWindow ? pPresentationParameters->hDeviceWindow : g_hFocusWindow;
        HMONITOR monitor = MonitorFromWindow((!bUsePrimaryMonitor && hwnd) ? hwnd : GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
        MONITORINFO info;
        info.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo(monitor, &info);
        int DesktopResX = info.rcMonitor.right - info.rcMonitor.left;
        int DesktopResY = info.rcMonitor.bottom - info.rcMonitor.top;

        int left = (int)info.rcMonitor.left;
        int top = (int)info.rcMonitor.top;

        if (nForceWindowStyle != 1) // not borderless fullscreen
        {
            left += (int)(((float)DesktopResX / 2.0f) - ((float)pPresentationParameters->BackBufferWidth / 2.0f));
            top += (int)(((float)DesktopResY / 2.0f) - ((float)pPresentationParameters->BackBufferHeight / 2.0f));
        }

        pPresentationParameters->Windowed = 1;

        // This must be set to default (0) on windowed mode as per D3D9 spec
        pPresentationParameters->FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

        // If exists, this must match the rate in PresentationParameters
        if (pFullscreenDisplayMode != NULL)
            pFullscreenDisplayMode->RefreshRate = D3DPRESENT_RATE_DEFAULT;

        // This flag is not available on windowed mode as per D3D9 spec
        pPresentationParameters->PresentationInterval &= ~D3DPRESENT_DONOTFLIP;

        if (hwnd != NULL)
        {
            int cx, cy;
            UINT uFlags = SWP_SHOWWINDOW;
            LONG lOldStyle = GetWindowLong(hwnd, GWL_STYLE);
            LONG lOldExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
            LONG lNewStyle, lNewExStyle;

            lOldExStyle &= ~(WS_EX_TOPMOST);

            if (nForceWindowStyle == 1)
            {
                cx = DesktopResX;
                cy = DesktopResY;
            }
            else
            {
                cx = pPresentationParameters->BackBufferWidth;
                cy = pPresentationParameters->BackBufferHeight;

                if (!bCenterWindow)
                    uFlags |= SWP_NOMOVE;
            }

            switch (nForceWindowStyle)
            {
            case 1: // borderless fullscreen
                lNewStyle = lOldStyle & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_DLGFRAME);
                lNewStyle |= (lOldStyle & WS_CHILD) ? 0 : WS_POPUP;
                lNewExStyle = lOldExStyle & ~(WS_EX_CONTEXTHELP | WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE | WS_EX_TOOLWINDOW);
                lNewExStyle |= WS_EX_APPWINDOW;
                break;
            case 0: // default windowed (keep original style)
            default:
                lNewStyle = lOldStyle;
                lNewExStyle = lOldExStyle;
                break;
            }

            if (nForceWindowStyle != 0)
            {
                if (lNewStyle != lOldStyle)
                {
                    SetWindowLong(hwnd, GWL_STYLE, lNewStyle);
                    uFlags |= SWP_FRAMECHANGED;
                }
                if (lNewExStyle != lOldExStyle)
                {
                    SetWindowLong(hwnd, GWL_EXSTYLE, lNewExStyle);
                    uFlags |= SWP_FRAMECHANGED;
                }
            }
            SetWindowPos(hwnd, bAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, left, top, cx, cy, uFlags);

            if (bDoNotNotifyOnTaskSwitch)
            {
                WORD wClassAtom = GetClassWord(hwnd, GCW_ATOM);
                if (wClassAtom != 0)
                {
                    bool found = false;
                    for (unsigned int i = 0; i < WndProcList.size(); i++)
                    {
                        if (WndProcList[i].first == wClassAtom)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        LONG_PTR wndproc = GetWindowLongPtr(hwnd, GWLP_WNDPROC);
                        if (wndproc && !IsBadCodePtr((FARPROC)wndproc))
                        {
                            WndProcList.emplace_back(wClassAtom, wndproc);
                            SetWindowLongPtr(hwnd, GWLP_WNDPROC, IsWindowUnicode(hwnd) ? (LONG_PTR)CustomWndProcW : (LONG_PTR)CustomWndProcA);
                        }
                    }
                }
            }
        }
    }

    static void ForceFullScreenRefreshRateInHz(D3DPRESENT_PARAMETERS* pPresentationParameters)
    {
        if (!pPresentationParameters->Windowed)
        {
            std::vector<int> list;
            DISPLAY_DEVICE dd;
            dd.cb = sizeof(DISPLAY_DEVICE);
            DWORD deviceNum = 0;
            while (EnumDisplayDevices(NULL, deviceNum, &dd, 0))
            {
                DISPLAY_DEVICE newdd = { 0 };
                newdd.cb = sizeof(DISPLAY_DEVICE);
                DWORD monitorNum = 0;
                DEVMODE dm = { 0 };
                while (EnumDisplayDevices(dd.DeviceName, monitorNum, &newdd, 0))
                {
                    for (auto iModeNum = 0; EnumDisplaySettings(NULL, iModeNum, &dm) != 0; iModeNum++)
                        list.emplace_back(dm.dmDisplayFrequency);
                    monitorNum++;
                }
                deviceNum++;
            }

            std::sort(list.begin(), list.end());
            if (nFullScreenRefreshRateInHz > list.back() || nFullScreenRefreshRateInHz < list.front() || nFullScreenRefreshRateInHz < 0)
                pPresentationParameters->FullScreen_RefreshRateInHz = list.back();
            else
                pPresentationParameters->FullScreen_RefreshRateInHz = nFullScreenRefreshRateInHz;
        }
    }

public:
    FusionDxHook()
    {
        CIniReader iniReader("");
        bForceWindowedMode = iniReader.ReadInteger("WINDOWEDMODE", "ForceWindowedMode", 0) != 0;
        nFullScreenRefreshRateInHz = iniReader.ReadInteger("WINDOWEDMODE", "FullScreenRefreshRateInHz", 0);
        bUsePrimaryMonitor = iniReader.ReadInteger("WINDOWEDMODE", "UsePrimaryMonitor", 0) != 0;
        bCenterWindow = iniReader.ReadInteger("WINDOWEDMODE", "CenterWindow", 1) != 0;
        bAlwaysOnTop = iniReader.ReadInteger("WINDOWEDMODE", "AlwaysOnTop", 0) != 0;
        nForceWindowStyle = iniReader.ReadInteger("WINDOWEDMODE", "ForceWindowStyle", 0);
        bDoNotNotifyOnTaskSwitch = iniReader.ReadInteger("WINDOWEDMODE", "DoNotNotifyOnTaskSwitch", 0) != 0;

        if (!bForceWindowedMode)
            return;

        CallbackHandler::RegisterCallback(L"d3d9.dll", []()
        {
            std::thread([]()
            {
                auto hD3D9 = GetModuleHandleW(L"d3d9.dll");
                if (!hD3D9) return;
                auto Direct3DCreate9 = GetProcAddress(hD3D9, "Direct3DCreate9");
                if (!Direct3DCreate9) return;
                auto Direct3D9 = ((LPDIRECT3D9(WINAPI*)(uint32_t))(Direct3DCreate9))(D3D_SDK_VERSION);
                if (!Direct3D9) return;

                auto hookWindowCtor = HookWindow(L"FusionDxHookD3D9", L"FusionDxHookD3D9");
                auto hWnd = HookWindow::GetHookWindow(hookWindowCtor);

                D3DPRESENT_PARAMETERS params;
                params.BackBufferWidth = 0;
                params.BackBufferHeight = 0;
                params.BackBufferFormat = D3DFMT_UNKNOWN;
                params.BackBufferCount = 0;
                params.MultiSampleType = D3DMULTISAMPLE_NONE;
                params.MultiSampleQuality = NULL;
                params.SwapEffect = D3DSWAPEFFECT_DISCARD;
                params.hDeviceWindow = hWnd;
                params.Windowed = 1;
                params.EnableAutoDepthStencil = 0;
                params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
                params.Flags = NULL;
                params.FullScreen_RefreshRateInHz = 0;
                params.PresentationInterval = 0;

                LPDIRECT3DDEVICE9 Device;
                if (Direct3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT, &params, &Device) >= 0)
                {
                    copyMethods(hD3D9,
                        std::forward_as_tuple(IDirect3DDevice9VTBL(), *(uintptr_t**)Device),
                        std::forward_as_tuple(IDirect3D9VTBL(), *(uintptr_t**)Direct3D9)
                    );

                    auto D3D9CreateDevice = [](LPDIRECT3D9 pDirect3D9, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface) -> HRESULT
                    {
                        g_hFocusWindow = hFocusWindow ? hFocusWindow : pPresentationParameters->hDeviceWindow;
                        if (bForceWindowedMode)
                        {
                            FusionDxHook::ForceWindowed(pPresentationParameters);
                        }
                        if (nFullScreenRefreshRateInHz)
                        {
                            FusionDxHook::ForceFullScreenRefreshRateInHz(pPresentationParameters);
                        }
                        return CreateDeviceOriginal.unsafe_stdcall<HRESULT>(pDirect3D9, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
                    };

                    auto D3D9BeginScene = [](LPDIRECT3DDEVICE9 pDevice) -> HRESULT
                    {
                        return BeginSceneOriginal.unsafe_stdcall<HRESULT>(pDevice);
                    };

                    auto D3D9EndScene = [](LPDIRECT3DDEVICE9 pDevice) -> HRESULT
                    {
                        return EndSceneOriginal.unsafe_stdcall<HRESULT>(pDevice);
                    };

                    auto D3D9Reset = [](LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) -> HRESULT
                    {
                        if (bForceWindowedMode)
                        {
                            FusionDxHook::ForceWindowed(pPresentationParameters);
                        }
                        if (nFullScreenRefreshRateInHz)
                        {
                            FusionDxHook::ForceFullScreenRefreshRateInHz(pPresentationParameters);
                        }
                        return ResetOriginal.unsafe_stdcall<HRESULT>(pDevice, pPresentationParameters);
                    };

                    auto D3D9ResetEx = [](LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode) -> HRESULT
                    {
                        if (bForceWindowedMode)
                        {
                            FusionDxHook::ForceWindowed(pPresentationParameters, pFullscreenDisplayMode);
                        }
                        if (nFullScreenRefreshRateInHz)
                        {
                            FusionDxHook::ForceFullScreenRefreshRateInHz(pPresentationParameters);
                        }
                        return ResetExOriginal.unsafe_stdcall<HRESULT>(pDevice, pPresentationParameters, pFullscreenDisplayMode);
                    };

                    auto D3D9Present = [](LPDIRECT3DDEVICE9 pDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) -> HRESULT
                    {
                        return PresentOriginal.unsafe_stdcall<HRESULT>(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
                    };

                    auto D3D9PresentEx = [](LPDIRECT3DDEVICE9 pDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags) -> HRESULT
                    {
                        return PresentExOriginal.unsafe_stdcall<HRESULT>(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
                    };

                    auto D3D9SetVertexShaderConstantF = [](LPDIRECT3DDEVICE9 pDevice, UINT StartRegister, float* pConstantData, UINT Vector4fCount) -> HRESULT
                    {
                        return SetVertexShaderConstantFOriginal.unsafe_stdcall<HRESULT>(pDevice, StartRegister, pConstantData, Vector4fCount);
                    };

                    auto D3D9SetPixelShaderConstantF = [](LPDIRECT3DDEVICE9 pDevice, UINT StartRegister, float* pConstantData, UINT Vector4fCount) -> HRESULT
                    {
                        return SetPixelShaderConstantFOriginal.unsafe_stdcall<HRESULT>(pDevice, StartRegister, pConstantData, Vector4fCount);
                    };

                    auto D3D9CreateTexture = [](LPDIRECT3DDEVICE9 pDevice, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle) ->HRESULT
                    {
                        return CreateTextureOriginal.unsafe_stdcall<HRESULT>(pDevice, Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);
                    };

                    auto D3D9SetTexture = [](LPDIRECT3DDEVICE9 pDevice, DWORD Stage, IDirect3DBaseTexture9* pTexture) -> HRESULT
                    {
                        return SetTextureOriginal.unsafe_stdcall<HRESULT>(pDevice, Stage, pTexture);
                    };

                    auto D3D9DrawPrimitive = [](LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) -> HRESULT
                    {
                        return DrawPrimitiveOriginal.unsafe_stdcall<HRESULT>(pDevice, PrimitiveType, StartVertex, PrimitiveCount);
                    };

                    auto D3D9SetPixelShader = [](LPDIRECT3DDEVICE9 pDevice, IDirect3DPixelShader9* pShader) -> HRESULT
                    {
                        return SetPixelShaderOriginal.unsafe_stdcall<HRESULT>(pDevice, pShader);
                    };

                    auto D3D9SetVertexShader = [](LPDIRECT3DDEVICE9 pDevice, IDirect3DVertexShader9* pShader) -> HRESULT
                    {
                        return SetVertexShaderOriginal.unsafe_stdcall<HRESULT>(pDevice, pShader);
                    };

                    auto D3D9CreatePixelShader = [](LPDIRECT3DDEVICE9 pDevice, DWORD* pFunction, IDirect3DPixelShader9** ppShader) -> HRESULT
                    {
                        return CreatePixelShaderOriginal.unsafe_stdcall<HRESULT>(pDevice, pFunction, ppShader);
                    };

                    auto D3D9CreateVertexShader = [](LPDIRECT3DDEVICE9 pDevice, DWORD* pFunction, IDirect3DVertexShader9** ppShader) -> HRESULT
                    {
                        return CreateVertexShaderOriginal.unsafe_stdcall<HRESULT>(pDevice, pFunction, ppShader);
                    };

                    static HRESULT(WINAPI* CreateDevice)(LPDIRECT3D9, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**) = D3D9CreateDevice;
                    //static HRESULT(WINAPI* BeginScene)(LPDIRECT3DDEVICE9) = D3D9BeginScene;
                    //static HRESULT(WINAPI* EndScene)(LPDIRECT3DDEVICE9) = D3D9EndScene;
                    static HRESULT(WINAPI* Reset)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*) = D3D9Reset;
                    static HRESULT(WINAPI* ResetEx)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*, D3DDISPLAYMODEEX*) = D3D9ResetEx;
                    //static HRESULT(WINAPI* Present)(LPDIRECT3DDEVICE9, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*) = D3D9Present;
                    //static HRESULT(WINAPI* PresentEx)(LPDIRECT3DDEVICE9, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*, DWORD) = D3D9PresentEx;
                    //static HRESULT(WINAPI* SetVertexShaderConstantF)(LPDIRECT3DDEVICE9, UINT, float*, UINT) = D3D9SetVertexShaderConstantF;
                    //static HRESULT(WINAPI* SetPixelShaderConstantF)(LPDIRECT3DDEVICE9, UINT, float*, UINT) = D3D9SetPixelShaderConstantF;
                    //static HRESULT(WINAPI* CreateTexture)(LPDIRECT3DDEVICE9, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DTexture9**, HANDLE*) = D3D9CreateTexture;
                    //static HRESULT(WINAPI* SetTexture)(LPDIRECT3DDEVICE9, DWORD, IDirect3DBaseTexture9*) = D3D9SetTexture;
                    //static HRESULT(WINAPI* DrawPrimitive)(LPDIRECT3DDEVICE9, D3DPRIMITIVETYPE, UINT, UINT) = D3D9DrawPrimitive;
                    //static HRESULT(WINAPI* SetPixelShader)(LPDIRECT3DDEVICE9, IDirect3DPixelShader9*) = D3D9SetPixelShader;
                    //static HRESULT(WINAPI* SetVertexShader)(LPDIRECT3DDEVICE9, IDirect3DVertexShader9*) = D3D9SetVertexShader;
                    //static HRESULT(WINAPI* CreatePixelShader)(LPDIRECT3DDEVICE9, DWORD*, IDirect3DPixelShader9**) = D3D9CreatePixelShader;
                    //static HRESULT(WINAPI* CreateVertexShader)(LPDIRECT3DDEVICE9, DWORD*, IDirect3DVertexShader9**) = D3D9CreateVertexShader;

                    bind(hD3D9, typeid(IDirect3D9VTBL), IDirect3D9VTBL().GetIndex("CreateDevice"), CreateDevice, CreateDeviceOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("BeginScene"), BeginScene, BeginSceneOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("EndScene"), EndScene, EndSceneOriginal);
                    bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("Reset"), Reset, ResetOriginal);
                    bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("ResetEx"), ResetEx, ResetExOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("Present"), Present, PresentOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("PresentEx"), PresentEx, PresentExOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("SetVertexShaderConstantF"), SetVertexShaderConstantF, SetVertexShaderConstantFOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("SetPixelShaderConstantF"), SetPixelShaderConstantF, SetPixelShaderConstantFOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("CreateTexture"), CreateTexture, CreateTextureOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("SetTexture"), SetTexture, SetTextureOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("DrawPrimitive"), DrawPrimitive, DrawPrimitiveOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("SetPixelShader"), SetPixelShader, SetPixelShaderOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("SetVertexShader"), SetVertexShader, SetVertexShaderOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("CreatePixelShader"), CreatePixelShader, CreatePixelShaderOriginal);
                    //bind(hD3D9, typeid(IDirect3DDevice9VTBL), IDirect3DDevice9VTBL().GetIndex("CreateVertexShader"), CreateVertexShader, CreateVertexShaderOriginal);

                    Device->Release();
                }
                Direct3D9->Release();
            }).detach();
        });

        if (bDoNotNotifyOnTaskSwitch)
        {
            GetSystemWindowsDirectoryA(WinDir, MAX_PATH);

            HMODULE mainModule = GetModuleHandleA(nullptr);

            // Hook main module user32.dll imports
            {
                auto originalsUser32 = IATHook::Replace(
                    mainModule, "user32.dll",
                    std::make_tuple("RegisterClassA", (void*)hk_RegisterClassA),
                    std::make_tuple("RegisterClassW", (void*)hk_RegisterClassW),
                    std::make_tuple("RegisterClassExA", (void*)hk_RegisterClassExA),
                    std::make_tuple("RegisterClassExW", (void*)hk_RegisterClassExW),
                    std::make_tuple("GetForegroundWindow", (void*)hk_GetForegroundWindow),
                    std::make_tuple("GetActiveWindow", (void*)hk_GetActiveWindow),
                    std::make_tuple("GetFocus", (void*)hk_GetFocus)
                );

                if (oRegisterClassA == NULL)
                {
                    auto it = originalsUser32.find("RegisterClassA");   if (it != originalsUser32.end())  oRegisterClassA = (RegisterClassA_fn)it->second.get();
                }
                if (oRegisterClassW == NULL)
                {
                    auto it = originalsUser32.find("RegisterClassW");   if (it != originalsUser32.end())  oRegisterClassW = (RegisterClassW_fn)it->second.get();
                }
                if (oRegisterClassExA == NULL)
                {
                    auto it = originalsUser32.find("RegisterClassExA"); if (it != originalsUser32.end())  oRegisterClassExA = (RegisterClassExA_fn)it->second.get();
                }
                if (oRegisterClassExW == NULL)
                {
                    auto it = originalsUser32.find("RegisterClassExW"); if (it != originalsUser32.end())  oRegisterClassExW = (RegisterClassExW_fn)it->second.get();
                }
                if (oGetForegroundWindow == NULL)
                {
                    auto it = originalsUser32.find("GetForegroundWindow"); if (it != originalsUser32.end()) oGetForegroundWindow = (GetForegroundWindow_fn)it->second.get();
                }
                if (oGetActiveWindow == NULL)
                {
                    auto it = originalsUser32.find("GetActiveWindow");     if (it != originalsUser32.end()) oGetActiveWindow = (GetActiveWindow_fn)it->second.get();
                }
                if (oGetFocus == NULL)
                {
                    auto it = originalsUser32.find("GetFocus");            if (it != originalsUser32.end()) oGetFocus = (GetFocus_fn)it->second.get();
                }
            }

            // Hook main module kernel32.dll imports (including GetProcAddress)
            {
                auto originalsKernel32 = IATHook::Replace(
                    mainModule, "kernel32.dll",
                    std::make_tuple("LoadLibraryA", (void*)hk_LoadLibraryA),
                    std::make_tuple("LoadLibraryW", (void*)hk_LoadLibraryW),
                    std::make_tuple("LoadLibraryExA", (void*)hk_LoadLibraryExA),
                    std::make_tuple("LoadLibraryExW", (void*)hk_LoadLibraryExW),
                    std::make_tuple("FreeLibrary", (void*)hk_FreeLibrary),
                    std::make_tuple("GetProcAddress", (void*)hk_GetProcAddress)
                );

                if (oLoadLibraryA == NULL)
                {
                    auto it = originalsKernel32.find("LoadLibraryA");   if (it != originalsKernel32.end()) oLoadLibraryA = (LoadLibraryA_fn)it->second.get();
                }
                if (oLoadLibraryW == NULL)
                {
                    auto it = originalsKernel32.find("LoadLibraryW");   if (it != originalsKernel32.end()) oLoadLibraryW = (LoadLibraryW_fn)it->second.get();
                }
                if (oLoadLibraryExA == NULL)
                {
                    auto it = originalsKernel32.find("LoadLibraryExA"); if (it != originalsKernel32.end()) oLoadLibraryExA = (LoadLibraryExA_fn)it->second.get();
                }
                if (oLoadLibraryExW == NULL)
                {
                    auto it = originalsKernel32.find("LoadLibraryExW"); if (it != originalsKernel32.end()) oLoadLibraryExW = (LoadLibraryExW_fn)it->second.get();
                }
                if (oFreeLibrary == NULL)
                {
                    auto it = originalsKernel32.find("FreeLibrary");    if (it != originalsKernel32.end()) oFreeLibrary = (FreeLibrary_fn)it->second.get();
                }
            }

            // Ensure d3d9.dll's IAT calls route through our hooks as well
            //if (d3d9dll)
            //{
            //    IATHook::Replace(d3d9dll, "kernel32.dll",
            //        std::make_tuple("GetProcAddress", (void*)hk_GetProcAddress)
            //    );
            //
            //    auto u32_d3d9 = IATHook::Replace(d3d9dll, "user32.dll",
            //        std::make_tuple("GetForegroundWindow", (void*)hk_GetForegroundWindow)
            //    );
            //    if (oGetForegroundWindow == NULL)
            //    {
            //        auto it = u32_d3d9.find("GetForegroundWindow");
            //        if (it != u32_d3d9.end()) oGetForegroundWindow = (GetForegroundWindow_fn)it->second.get();
            //    }
            //}

            // Hook ole32.dll's imports of user32 where applicable
            if (HMODULE ole32 = GetModuleHandleA("ole32.dll"))
            {
                auto originalsOleUser32 = IATHook::Replace(
                    ole32, "user32.dll",
                    std::make_tuple("RegisterClassA", (void*)hk_RegisterClassA),
                    std::make_tuple("RegisterClassW", (void*)hk_RegisterClassW),
                    std::make_tuple("RegisterClassExA", (void*)hk_RegisterClassExA),
                    std::make_tuple("RegisterClassExW", (void*)hk_RegisterClassExW),
                    std::make_tuple("GetActiveWindow", (void*)hk_GetActiveWindow)
                );

                if (oRegisterClassA == NULL)
                {
                    auto it = originalsOleUser32.find("RegisterClassA");   if (it != originalsOleUser32.end())  oRegisterClassA = (RegisterClassA_fn)it->second.get();
                }
                if (oRegisterClassW == NULL)
                {
                    auto it = originalsOleUser32.find("RegisterClassW");   if (it != originalsOleUser32.end())  oRegisterClassW = (RegisterClassW_fn)it->second.get();
                }
                if (oRegisterClassExA == NULL)
                {
                    auto it = originalsOleUser32.find("RegisterClassExA"); if (it != originalsOleUser32.end())  oRegisterClassExA = (RegisterClassExA_fn)it->second.get();
                }
                if (oRegisterClassExW == NULL)
                {
                    auto it = originalsOleUser32.find("RegisterClassExW"); if (it != originalsOleUser32.end())  oRegisterClassExW = (RegisterClassExW_fn)it->second.get();
                }
                if (oGetActiveWindow == NULL)
                {
                    auto it = originalsOleUser32.find("GetActiveWindow");  if (it != originalsOleUser32.end())  oGetActiveWindow = (GetActiveWindow_fn)it->second.get();
                }
            }

            HookImportedModules();
        }
    }

public:
    static inline void bind(HMODULE module, std::type_index type_index, uint16_t func_index, void* function, SafetyHookInline& hook)
    {
        auto target = deviceMethods.at(module).at(type_index).at(func_index);
        hook = safetyhook::create_inline(target, function);
    }
    static inline void unbind(SafetyHookInline& hook)
    {
        hook.reset();
    }
} FusionDxHook;