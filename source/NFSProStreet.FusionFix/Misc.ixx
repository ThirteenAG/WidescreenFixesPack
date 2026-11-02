module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/NFS/FieldOfView.hpp"
#include <d3d9.h>

export module Misc;

import ComVars;
import Input;
import PostFX;
import Options;

// Points the memory operand of a game instruction at a value of the fix
template<typename T>
void Redirect(void* operand, T* value)
{
    injector::WriteMemory<T*>(operand, value, true);
}

// Damage memory pools: they leaked a little with every race, so long sessions ran out of memory.
// They're recreated once the game is back in the front end.
namespace DamageFix
{
    inline uintptr_t ptrbFree = 0;
    inline intptr_t ptrMemoryPoolClose = 0;
    inline uintptr_t ptr_g_AttachmentVBMemoryPool = 0;
    inline uintptr_t ptr_g_AttachmentMemoryPool = 0;

    void* ptrDamageVBPool = 0;
    void* ptrDamagePool = 0;

    inline const char* aDamagevbpool = "OptimizedDamageVBPool";
    inline const char* aDamagepool = "OptimizedDamagePool";

    inline intptr_t bGetFreeMemoryPoolNum = 0;
    inline intptr_t bWareMalloc = 0;
    inline uintptr_t MemoryPoolInfoTable = 0;
    inline uintptr_t MemoryPoolMem = 0;
    inline uintptr_t MemoryPools = 0;
    inline uintptr_t MemoryPool_Init = 0;

    void __declspec(naked) DamagePublic_Init()
    {
        // clang-format off
        _asm
        {
            push    ebx
            push    esi
            call    bGetFreeMemoryPoolNum; bGetFreeMemoryPoolNum(void)
            xor ebx, ebx
            push    ebx; allocation_params
            push    1000000h; size
            mov ebx, ptr_g_AttachmentVBMemoryPool
            mov     dword ptr[ebx], eax; int g_AttachmentVBMemoryPool
            call    bWareMalloc; bWareMalloc(int, char const*, int, int)
            mov ptrDamageVBPool, eax
            mov edx, ptr_g_AttachmentVBMemoryPool
            mov     edx, dword ptr[edx]; memory
            mov     ecx, edx
            shl     ecx, 4
            add     ecx, MemoryPoolInfoTable; MemoryPoolInfo* MemoryPoolInfoTable
            mov[ecx + 1], bl
            mov     dword ptr[ecx + 4], 0FFFFFFFFh
            mov     dword ptr[ecx + 8], 10h
            mov     ecx, edx
            imul    ecx, 7Ch; '|'; mempool
            add     esp, 8
            push    aDamagevbpool; "DamageVBPool"
            push    1000000h; memory_size
            mov esi, MemoryPoolMem
            lea     esi, dword ptr[esi + ecx]
            push    eax; memory
            mov eax, MemoryPools
            mov     dword ptr[eax + edx * 4], esi; MemoryPool** MemoryPools
            call    MemoryPool_Init; MemoryPool::Init(void*, int, char const*)
            mov eax, ptr_g_AttachmentVBMemoryPool
            mov eax, dword ptr[eax]
            mov     esi, eax
            and eax, 1Fh
            or eax, 40h
            push    eax; allocation_params
            push    10h; size
            call    bWareMalloc; bWareMalloc(int, char const*, int, int)
            mov edx, MemoryPools
            mov     edx, dword ptr[edx + esi * 4]; MemoryPool** MemoryPools
            push    eax
            mov[edx + 45h], bl
            call    ptrbFree; bFree(void*)
            call    bGetFreeMemoryPoolNum; bGetFreeMemoryPoolNum(void)
            push    ebx; allocation_params
            push    700000h; size
            mov ebx, ptr_g_AttachmentMemoryPool
            mov dword ptr[ebx], eax
            call    bWareMalloc; bWareMalloc(int, char const*, int, int)
            mov ptrDamagePool, eax
            mov edx, ptr_g_AttachmentMemoryPool
            mov edx, dword ptr[edx]
            mov     ecx, edx
            shl     ecx, 4
            add     ecx, MemoryPoolInfoTable; MemoryPoolInfo* MemoryPoolInfoTable
            mov[ecx + 1], bl
            mov     dword ptr[ecx + 4], 0FFFFFFFFh
            mov     dword ptr[ecx + 8], 10h
            mov     ecx, edx
            imul    ecx, 7Ch; '|'; mempool
            add     esp, 14h
            push    aDamagepool; "DamagePool"
            push    700000h; memory_size
            mov esi, MemoryPoolMem
            lea     esi, dword ptr[esi + ecx]
            push    eax; memory
            mov eax, MemoryPools
            mov     dword ptr[eax + edx * 4], esi; MemoryPool** MemoryPools
            call    MemoryPool_Init; MemoryPool::Init(void*, int, char const*)
            mov eax, ptr_g_AttachmentMemoryPool
            mov eax, dword ptr[eax]
            mov     esi, eax
            and eax, 1Fh
            or eax, 40h
            push    eax; allocation_params
            push    10h; size
            call    bWareMalloc; bWareMalloc(int, char const*, int, int)
            mov edx, MemoryPools
            mov     edx, dword ptr[edx + esi * 4]; MemoryPool** MemoryPools
            push    eax
            mov[edx + 45h], bl
            call    ptrbFree; bFree(void*)
            add     esp, 0Ch
            pop     esi
            pop     ebx
            retn
        }
        // clang-format on
    }

    bool bReInitFlag = false;
    void __stdcall QuitToFEHook()
    {
        bReInitFlag = true;
    }

    void GameLoopExecutor()
    {
        if (!bReInitFlag)
            return;

        if (IsFrontEnd())
        {
            reinterpret_cast<void(*)(uint32_t)>(ptrMemoryPoolClose)(g_AttachmentMemoryPool);   // MemoryPool::Close
            reinterpret_cast<void(*)(uint32_t)>(ptrMemoryPoolClose)(g_AttachmentVBMemoryPool); // MemoryPool::Close

            g_AttachmentVBMemoryPool = -1;
            g_AttachmentMemoryPool = -1;

            reinterpret_cast<void(*)(void*)>(ptrbFree)(ptrDamageVBPool); // bFree
            reinterpret_cast<void(*)(void*)>(ptrbFree)(ptrDamagePool);   // bFree
            DamagePublic_Init();
            bReInitFlag = false;
        }
    }

    void Init()
    {
        uintptr_t loc_6DAB0E = reinterpret_cast<uintptr_t>(hook::pattern("68 00 60 00 00 E8 ? ? ? ? D9 05 ? ? ? ?").get_first(0)) + 0x5A;
        uintptr_t loc_6A7976 = reinterpret_cast<uintptr_t>(hook::pattern("8B 11 8B 43 08 8B 92 54 01 00 00 50 FF D2").get_first(0)) - 0xB;
        uintptr_t loc_4351CC = reinterpret_cast<uintptr_t>(hook::pattern("7E 18 68 FF FF FF 7F").get_first(0));
        uintptr_t loc_4351E6 = loc_4351CC + 0x1A;
        uintptr_t loc_4482FC = reinterpret_cast<uintptr_t>(hook::pattern("66 F7 46 0E 00 20 0F").get_first(0)) + 6;
        uintptr_t loc_6DB10F = reinterpret_cast<uintptr_t>(hook::pattern("E8 ? ? ? ? E8 ? ? ? ? 8B 4C 24 10 64 89 0D 00 00 00 00 59 5E 83 C4 14 C3 CC").get_first(0)) - 0xA;
        ptrbFree = reinterpret_cast<uintptr_t>(hook::pattern("80 7E F5 22").get_first(0)) - 0x74;

        uintptr_t loc_7907EA = reinterpret_cast<uintptr_t>(hook::pattern("E8 ? ? ? ? 8B 4E 60 51").get_first(0));
        ptrMemoryPoolClose = loc_7907EA + 5 + *reinterpret_cast<intptr_t*>(loc_7907EA + 1);

        uintptr_t loc_65F770 = reinterpret_cast<uintptr_t>(hook::pattern("6A 47 68 00 40 16 00").get_first(0)) + 0x12;
        bGetFreeMemoryPoolNum = loc_65F770 + 5 + *reinterpret_cast<intptr_t*>(loc_65F770 + 1);

        uintptr_t loc_436538 = reinterpret_cast<uintptr_t>(hook::pattern("C1 E8 02 25 FF 07 00 00").get_first(0)) + 0x40;
        bWareMalloc = loc_436538 + 5 + *reinterpret_cast<intptr_t*>(loc_436538 + 1);

        uintptr_t loc_1C45036 = reinterpret_cast<uintptr_t>(hook::pattern("8B C8 C1 E1 04 81 C1 ? ? ? ?").get_first(0)); // SecuROM section; 0x00436188 in the DRM-free demo
        MemoryPoolInfoTable = *reinterpret_cast<uintptr_t*>(loc_1C45036 + 7);
        MemoryPoolMem = *reinterpret_cast<uintptr_t*>(loc_1C45036 + 0x2A);
        MemoryPools = *reinterpret_cast<uintptr_t*>(loc_1C45036 + 0x37);

        uintptr_t loc_435F84 = reinterpret_cast<uintptr_t>(hook::pattern("C6 05 ? ? ? ? 00 C7 05 ? ? ? ? FF FF FF FF C7 05 ? ? ? ? 10 00 00 00 89 35 ? ? ? ? E8").get_first(0)) + 0x21; // 2 hits in the DRM-free demo, which is OK
        MemoryPool_Init = loc_435F84 + 5 + *reinterpret_cast<intptr_t*>(loc_435F84 + 1);

        uintptr_t sub_438AE0 = reinterpret_cast<uintptr_t>(hook::pattern("A1 ? ? ? ? 83 E0 1F 0D 00 04 00 00 89 44 24 08 E9 ? ? ? ?").get_first(0));
        ptr_g_AttachmentVBMemoryPool = *reinterpret_cast<uintptr_t*>(sub_438AE0 + 1);
        g_AttachmentVBMemoryPool.SetAddress(ptr_g_AttachmentVBMemoryPool);

        uintptr_t loc_438962 = reinterpret_cast<uintptr_t>(hook::pattern("A1 ? ? ? ? 83 E0 1F 0D 00 04 00 00 50 68 C4 0A 00 00").get_first(0));
        ptr_g_AttachmentMemoryPool = *reinterpret_cast<uintptr_t*>(loc_438962 + 1);
        g_AttachmentMemoryPool.SetAddress(ptr_g_AttachmentMemoryPool);

        // DamagePublic init DRM-free code
        injector::MakeCALL(loc_6DAB0E, DamagePublic_Init, true);

        // Hook for EQuitToFE to tell the game it's ready to free memory
        injector::MakeCALL(loc_6A7976, QuitToFEHook, true);

        // remove free assert
        injector::MakeJMP(loc_4351CC, loc_4351E6, true);

        // force damage skin recreation
        injector::MakeNOP(loc_4482FC, 6, true);

        // game loop
        injector::MakeCALL(loc_6DB10F, GameLoopExecutor, true);
    }
}

// FE::Object::SetScript(object, script hash, start) could be called with an object whose memory was already
// released, which crashed in the script lookup. Objects in unmapped memory are skipped.
namespace FEScriptFix
{
    bool IsReadable(const void* ptr, size_t size)
    {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(ptr, &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) || mbi.Protect == PAGE_EXECUTE)
            return false;

        auto end = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        auto last = reinterpret_cast<uintptr_t>(ptr) + size;
        return last <= end || IsReadable(reinterpret_cast<const void*>(end), last - end);
    }

    SafetyHookInline shSetScript = {};
    void __cdecl SetScript(void* object, uint32_t scriptHash, bool start)
    {
        if (object && !IsReadable(object, 8))
            return;
        shSetScript.unsafe_ccall(object, scriptHash, start);
    }
}

// Sparks are drawn with additive blending, like on the Xbox 360: the sprite batch sets its texture and blend
// states, and the destination blend is made additive after that while the spark batch is drawn
namespace XenonEffects
{
    bool bInSparkRender = false;

    void(__thiscall* DrawBatchFunc)(void*, void*) = nullptr;
    void __fastcall DrawBatch(void* manager, void* edx, void* view)
    {
        bInSparkRender = true;
        DrawBatchFunc(manager, view);
        bInSparkRender = false;
    }

    void(__thiscall* SetTextureFunc)(void*, void*) = nullptr;
    void __fastcall SetTexture(void* self, void* edx, void* texture)
    {
        SetTextureFunc(self, texture);
        if (bInSparkRender && *Direct3DDevice && **Direct3DDevice)
            (**Direct3DDevice)->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
    }
}

// Focus. When the window loses focus the game mutes its sound (WM_ACTIVATE) and pauses (WM_ACTIVATEAPP: its main
// loop waits until the window is active again). Online and LAN races keep running
// in the background, and so does everything with MAIN.PauseOnFocusLoss off; input is blocked then while the window
// isn't focused.
namespace OnlineInputBlocker
{
    bool bPollingEnabled = true;
    const int* pPauseOnFocusLoss = nullptr;

    bool Polling()
    {
        return bPollingEnabled && (!pPauseOnFocusLoss || *pPauseOnFocusLoss || GetForegroundWindow() == hWnd);
    }

    uintptr_t ptrGameDevicePoll = 0;
    void __fastcall GameDevicePoll(uintptr_t device, void* edx)
    {
        if (Polling())
            reinterpret_cast<void(__thiscall*)(uintptr_t)>(ptrGameDevicePoll)(device);
    }

    uintptr_t ptrSteeringWheelDevicePoll = 0;
    uint32_t __fastcall SteeringWheelDevicePoll(uintptr_t device, void* edx)
    {
        if (!Polling())
            return 0;
        return reinterpret_cast<uint32_t(__thiscall*)(uintptr_t)>(ptrSteeringWheelDevicePoll)(device);
    }

    WNDPROC GameWndProc = nullptr;
    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        LRESULT result = 0;
        if (DeveloperOverlay::Message(hWnd, msg, wParam, lParam, result))
            return result;
        onWindowMessage().executeAll(hWnd, msg, wParam, lParam);

        if (msg == WM_ACTIVATEAPP || msg == WM_ACTIVATE)
        {
            bool inactive = (msg == WM_ACTIVATE) ? (LOWORD(wParam) == WA_INACTIVE) : (wParam == FALSE);
            if (!inactive)
                bPollingEnabled = true;
            else if (OnlineActiveGameManager.is_initialized() && OnlineActiveGameManager.get())
            {
                bPollingEnabled = false;
                return TRUE;
            }
            else if (pPauseOnFocusLoss && !*pPauseOnFocusLoss)
                return DefWindowProcA(hWnd, msg, wParam, lParam);
        }

        return GameWndProc(hWnd, msg, wParam, lParam);
    }
}

// Frame rate. The game simulates in fixed steps of 1/60 s and limits the frame rate with a coarse wait. The
// step is set to the frame rate limit, frames are paced to it precisely, and every frame gets one step. Real
// time is still kept: slower frames get more steps.
//
// The step time constants of the game (1/60 as a float and as a double, and ten steps for the full speed mode)
// are shared with code that has nothing to do with the simulation step: the conversion of the 60 Hz real time
// counter to seconds for the texture animations and for the clock of the speech system, the timer unit
// conversions and the sub-step limit of the attachment physics. Those keep their values, so the simulation's
// own instructions are pointed at values of the fix instead of changing the constants.
namespace FrameRate
{
    float fStepTime = 1.0f / 60.0f;
    double dStepTime = 1.0 / 60.0;
    float fFullSpeedTime = 10.0f / 60.0f;



    void PreciseWaitUntil(LARGE_INTEGER target)
    {
        static LARGE_INTEGER frequency = []() { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
        static HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double seconds = double(target.QuadPart - now.QuadPart) / double(frequency.QuadPart);

        constexpr double SpinTime = 0.0015;
        if (seconds > SpinTime)
        {
            LARGE_INTEGER due;
            due.QuadPart = -static_cast<LONGLONG>((seconds - SpinTime) * 10000000.0);
            if (timer && SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
                WaitForSingleObject(timer, INFINITE);
            else
                Sleep(static_cast<DWORD>((seconds - SpinTime) * 1000.0));
        }

        do
        {
            YieldProcessor();
            QueryPerformanceCounter(&now);
        } while (now.QuadPart < target.QuadPart);
    }

    // Called before every frame is shown
    void LimitFrameRate()
    {
        static LARGE_INTEGER frequency = []() { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
        static LARGE_INTEGER next = {};

        LONGLONG period = static_cast<LONGLONG>(double(frequency.QuadPart) * fStepTime);
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        if (next.QuadPart == 0 || now.QuadPart - next.QuadPart > period * 4)
        {
            // First frame, or far behind after a hitch or loading
            next.QuadPart = now.QuadPart + period;
            return;
        }

        PreciseWaitUntil(next);
        next.QuadPart += period;
    }

    // Sim::QueueEvents(dt) runs a step for every whole step time that has passed. With dt close to one
    // step, timing noise gave 0 or 2 steps instead of 1. The time is rounded to the nearest step here;
    // the difference is carried over, so the simulation keeps real time within half a step.
    float Carry = 0.0f;

    void PrepareStep(float& dt)
    {
        if (!*SimActive || dt <= 0.0f)
            return;

        float previous = *TimeBank;
        float due = previous + dt + Carry;
        int steps = static_cast<int>(std::floor(due / fStepTime + 0.5f));
        if (steps <= 0)
        {
            Carry = 0.0f;
            dt = due - previous;
            return;
        }

        // A little more than the steps, so that the game's truncation gives exactly this many
        float target = (static_cast<float>(steps) + 0.001f) * fStepTime;
        Carry = due - target;
        dt = target - previous;
    }
}

// The drag race burnout adds and takes away a fixed amount of boost grip on every simulation step, made for
// 60 steps per second. The amounts are scaled with the step time the vehicle's simulate function gets.
namespace BoostGrip
{
    double dOriginalAdder = 0.0, dOriginalSubtractor = 0.0;
    double dAdder = 0.0, dSubtractor = 0.0;
}

// Field of view
namespace FOV
{
    // Xbox 360 (1) or mathematically correct (2) horizontal and vertical FOV factors; 0 = the game's own
    double dOriginalHorizontal = 1.24, dOriginalVertical = 1.48;
    double dHorizontal = 1.24, dVertical = 1.48;
    const int* pScaling = nullptr;
    const float* pFovFactor = nullptr;     // Multiplier for the driving cameras

    void Update()
    {
        int scaling = std::clamp(*pScaling, 0, 2);
        dHorizontal = scaling == 0 ? dOriginalHorizontal : (scaling == 1 ? 1.26 : 1.30);
        dVertical = scaling == 0 ? dOriginalVertical : (scaling == 1 ? 1.50 : 1.48);
    }
}

namespace AspectRatio
{
    // The screen aspect ratio relative to 16:9
    float fScreenAspectRatio = 1.0f;
    double dHudWidth = 4.0 / 3.0;

    // Movies are 16:9 and shown whole: wider screens get pillarboxes, narrower ones letterboxes.
    float fMovieTop = 1.0f, fMovieBottom = -1.0f, fMovieLeft = -1.0f, fMovieRight = 1.0f;

    void Update()
    {
        fScreenAspectRatio = ScreenAspectRatio() * 0.5625f;
        dHudWidth = (4.0 / 3.0) / fScreenAspectRatio;

        float width = 1.0f / fScreenAspectRatio;
        float height = 1.0f;
        if (width > 1.0f)
        {
            height /= width;
            width = 1.0f;
        }
        fMovieLeft = -width;
        fMovieRight = width;
        fMovieTop = height;
        fMovieBottom = -height;
    }
}

namespace FEScale
{
    // The front end is made for 16:9 (or 4:3 on narrow screens); it's scaled down to fit narrower
    // screens, and movies keep their aspect ratio
    float fFEScale = 1.0f;
    bool bMovieShown = false;
}

// In-engine cutscenes (the ICE scenes: race intros, the kings' scenes and the endings) use the player view's field of
// view, widened for the screen like the game. Their data has a letterbox channel, which the game puts into the camera
// (+0x168, a part of the screen height, animated with the scene), but the PC version draws no bars from it; the
// console versions draw a black bar of that height at the top and at the bottom of the screen. MAIN.CutsceneBorders:
// 0 off (the whole picture, Hor+), 1 letterbox (the consoles' bars), 2 pillarbox (16:9 on wider screens), 3 both.
// The bars are drawn before the frame is shown while a scene runs (its flashes package is loaded; the race day hub's
// camera is a scene of its own, without one). Switched in the options menu at once.
namespace Cutscene
{
    const int* pBorders = nullptr;
    const int* pAnimated = nullptr;
    uint8_t* LetterboxCamera = nullptr; // the camera the letterbox channel was last written to
    SafetyHookInline shUpdateLetterbox = {};

    bool IsPlaying()
    {
        return TheICEManager.is_initialized() && TheICEManager.get() && *reinterpret_cast<uintptr_t*>(TheICEManager.get() + 0x70);
    }

    // The letterbox channel's frame update: it writes the height to the scene camera mover's camera (+4)
    void __fastcall UpdateLetterbox(void* channel, void* edx, int a1, int a2)
    {
        shUpdateLetterbox.unsafe_thiscall(channel, a1, a2);
        if (IsPlaying())
            LetterboxCamera = *reinterpret_cast<uint8_t**>(*reinterpret_cast<uintptr_t*>(TheICEManager.get() + 0x70) + 4);
    }

    void Clear(IDirect3DDevice9* device, const D3DSURFACE_DESC& desc, const D3DRECT* rects, DWORD count)
    {
        if (!count)
            return;
        D3DVIEWPORT9 viewport = {};
        device->GetViewport(&viewport);
        DWORD scissor = FALSE;
        device->GetRenderState(D3DRS_SCISSORTESTENABLE, &scissor);
        D3DVIEWPORT9 full = { 0, 0, desc.Width, desc.Height, 0.0f, 1.0f };
        device->SetViewport(&full);
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        device->Clear(count, rects, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, scissor);
        device->SetViewport(&viewport);
    }

    void Draw(IDirect3DDevice9* device)
    {
        int mode = *pBorders;
        if (mode == 0)
            return;

        IDirect3DSurface9* target = nullptr;
        if (FAILED(device->GetRenderTarget(0, &target)) || !target)
            return;
        D3DSURFACE_DESC desc = {};
        HRESULT hr = target->GetDesc(&desc);
        target->Release();
        if (FAILED(hr))
            return;

        // The render target's size: a resizable window keeps its back buffer and scales it to the window
        LONG width = static_cast<LONG>(desc.Width), height = static_cast<LONG>(desc.Height);
        D3DRECT rects[4]{};
        DWORD count = 0;

        if (mode == 1 || mode == 3)
        {
            constexpr float FullHeight = 0.125f;
            float part = LetterboxCamera ? *reinterpret_cast<float*>(LetterboxCamera + 0x168) : 0.0f;
            if (!std::isfinite(part) || part < 0.0f)
                part = 0.0f;
            if (part > 0.0f)
            {
                part = std::min(*pAnimated ? part : std::max(part, FullHeight), 0.45f);
                auto bar = static_cast<LONG>(static_cast<float>(height) * part + 0.5f);
                if (bar > 0)
                {
                    rects[count++] = { 0, 0, width, bar };
                    rects[count++] = { 0, height - bar, width, height };
                }
            }
        }

        if (mode == 2 || mode == 3)
        {
            LONG bar = static_cast<LONG>(std::lround(width * 0.5 * (1.0 - (16.0 / 9.0) / ScreenAspectRatio())));
            if (bar > 0)
            {
                rects[count++] = { 0, 0, bar, height };
                rects[count++] = { width - bar, 0, width, height };
            }
        }

        Clear(device, desc, rects, count);
    }

    // Black sides that keep a 16:9 picture on wider screens
    void Pillarbox(IDirect3DDevice9* device)
    {
        IDirect3DSurface9* target = nullptr;
        if (FAILED(device->GetRenderTarget(0, &target)) || !target)
            return;
        D3DSURFACE_DESC desc = {};
        HRESULT hr = target->GetDesc(&desc);
        target->Release();
        if (FAILED(hr))
            return;
        LONG width = static_cast<LONG>(desc.Width), height = static_cast<LONG>(desc.Height);
        LONG bar = static_cast<LONG>(std::lround(width * 0.5 * (1.0 - (16.0 / 9.0) / ScreenAspectRatio())));
        if (bar <= 0)
            return;
        D3DRECT rects[] = { { 0, 0, bar, height }, { width - bar, 0, width, height } };
        Clear(device, desc, rects, 2);
    }
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            FOV::pScaling = &FixSettings().Int("GRAPHICS.Scaling");
            FOV::pFovFactor = &FixSettings().Float("MAIN.FOVFactor");
            bool bSkipIntro = FixSettings().Bool("MAIN.SkipIntro");
            int nFPSLimit = FixSettings().Int("MAIN.FPSLimit");
            static const int* pDisableMotionBlur = &FixSettings().Int("GRAPHICS.DisableMotionBlur");
            int32_t nShadowsRes = FixSettings().Int("GRAPHICS.ShadowResolution");
            if (nShadowsRes != 0)
                nShadowsRes = std::clamp(nShadowsRes, 512, 16384);

            FreeConsole();

            // The game marks its settings with "FirstTime" while it runs and resets them on the next start when that
            // is still set, so every crash also reset the settings
            auto pattern = hook::pattern("C7 44 24 ? ? ? ? ? FF 15 ? ? ? ? 8D 54 24 0C 52");
            injector::WriteMemory(pattern.get_first(4), 0, true);

            // The title screen uses its HD background
            pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 8B F8 8B 46 0C 57");
            injector::WriteMemory(pattern.get_first(1), &"ENABLEHD", true);

            // Resolutions: every display mode of at least 640x480 is listed, instead of a few fixed ones
            pattern = hook::pattern("83 C0 08 83 F8 58 72 E8 E9 ? ? ? ? 8B 53 3C 8B 43 10");
            auto check = pattern.get_first<uint8_t>(-0x12);     // height in ecx, width in edx
            auto reject = injector::GetBranchDestination(pattern.get_first(8)).as_int();
            auto accept = reinterpret_cast<uintptr_t>(pattern.get_first(13));
            std::vector<uint8_t> code = { 0x81, 0xF9, 0xE0, 0x01, 0x00, 0x00 };     // cmp ecx, 480
            auto jb = [&](uintptr_t target)
            {
                int32_t rel = int32_t(target - (reinterpret_cast<uintptr_t>(check) + code.size() + 6));
                code.insert(code.end(), { 0x0F, 0x82 });
                code.insert(code.end(), reinterpret_cast<uint8_t*>(&rel), reinterpret_cast<uint8_t*>(&rel) + 4);
            };
            jb(reject);
            code.insert(code.end(), { 0x81, 0xFA, 0x80, 0x02, 0x00, 0x00 });     // cmp edx, 640
            jb(reject);
            int8_t toAccept = int8_t(accept - (reinterpret_cast<uintptr_t>(check) + code.size() + 2));
            code.insert(code.end(), { 0xEB, uint8_t(toAccept) });
            injector::WriteMemoryRaw(check, code.data(), code.size(), true);

            // TrackStreamer::HandleLoading: a section that doesn't fit into the streaming pool waits for space, but
            // the special section 2500 was then taken from the main pool instead (and always was), which could run
            // out of memory and crash. It's loaded through the streaming pool and waits like every other section.
            pattern = hook::pattern("80 7C 24 ? ? 74 10 68 ? ? ? ? 55");
            injector::MakeJMP(pattern.get_first(0), pattern.get_first(23), true);
            pattern = hook::pattern("75 11 01 6C 24 1C 83 43 28 01");
            injector::MakeNOP(pattern.get_first(0), 2, true);

            // FE::Object::SetScript
            pattern = hook::pattern("57 8B 7C 24 08 85 FF 74 2C 8B 44 24 0C 56");
            FEScriptFix::shSetScript = safetyhook::create_inline(pattern.get_first(), FEScriptFix::SetScript);

            // PunkBuster: its per frame update returns at once (the function's entry, before its SecuROM stub)
            pattern = hook::pattern("83 B9 80 03 00 00 00");
            injector::MakeRET(pattern.get_first(-0x3D), 0, true);

            DamageFix::Init();

            // The Xbox 360 achievement system, unused on PC: no manager, no updates and no checks
            pattern = hook::pattern("A1 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC 53 56 57 8B D9");
            auto updateAchievements = hook::pattern("8B F1 80 7E 20 00 75 05");
            auto checkAchievements = hook::pattern("8B F9 33 ED 80 3D ? ? ? ? 00 74 30");
            static auto RetZero = []() -> int32_t { return 0; };
            injector::MakeJMP(pattern.get_first(0), static_cast<int32_t(*)()>(RetZero), true);     // AchievementManager::Get
            injector::WriteMemoryRaw(updateAchievements.get_first(-0x1B), (void*)"\x58\x58\xC3", 3, true);  // AchievementManager::UpdateAchievements: pop, pop, ret
            injector::MakeRET(checkAchievements.get_first(-0x27), 4, true);                     // GProStreetAchievments::Check

            // The post race screen could get stuck on "Continue": mbScreenConstructed wasn't set
            pattern = hook::pattern("75 07 C6 86 C5 00 00 00 01");
            auto spVTable = hook::pattern("6A 2C C6 44 24 1C 01 C7 06 ? ? ? ?");
            auto onlineVTable = hook::pattern("C7 86 CC 00 00 00 ? ? ? ? C7 06 ? ? ? ? C7 86 CC 00 00 00 ? ? ? ? C7 86 D0 00 00");
            // Shadow and stat uploading leaked memory
            injector::MakeNOP(pattern.get_first(0), 2, true);
            injector::MakeJMP(pattern.get_first(9), pattern.get_first(0x4E), true);

            static uintptr_t SinglePlayerHandler = 0, OnlineHandler = 0;
            static auto Handler = [](uintptr_t original, uintptr_t self)
            {
                reinterpret_cast<void(__thiscall*)(uintptr_t)>(original)(self);
                *reinterpret_cast<bool*>(self + 0xC6) = true;
            };
            struct Hooks
            {
                static void __fastcall SinglePlayer(uintptr_t self, void*) { Handler(SinglePlayerHandler, self); }
                static void __fastcall Online(uintptr_t self, void*) { Handler(OnlineHandler, self); }
            };

            auto spSlot = *spVTable.get_first<uintptr_t>(9) + 0x128;
            auto onlineSlot = *onlineVTable.get_first<uintptr_t>(0xC) + 0x128;
            SinglePlayerHandler = *reinterpret_cast<uintptr_t*>(spSlot);
            OnlineHandler = *reinterpret_cast<uintptr_t*>(onlineSlot);
            injector::WriteMemory(spSlot, &Hooks::SinglePlayer, true);
            injector::WriteMemory(onlineSlot, &Hooks::Online, true);

            // The main loop's own limit during races (a busy wait for 33 ms since the last frame); frames are paced
            // in FrameRate instead
            pattern = hook::pattern("83 3D ? ? ? ? ? 75 05 E8 ? ? ? ? 8B 0D ? ? ? ? 85 C9 74 0F 6A 09");
            injector::MakeJMP(pattern.get_first(0), pattern.get_first(14), true);

            // The booster pack thanks screen on new profiles: the game goes on as if it was already shown
            pattern = hook::pattern("50 8B F1 68 E6 2E 00 00");
            injector::MakeJMP(pattern.get_first(-0xC + 2), pattern.get_first(-0xC + 0x23), true);
            injector::MakeNOP(pattern.get_first(-0xC + 0x25), 4, true);

            // LAN games: the data connection's UDP socket was made for any address family (AF_UNSPEC), which newer
            // Windows versions refuse, and its address lookup could return IPv6 addresses the game can't use. Both
            // are IPv4, the only family the game's 15 character address field can hold.
            pattern = hook::pattern("FF 15 ? ? ? ? 83 F8 FF 89 46 04 75 0F");
            static auto LANSocketHook = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
            {
                constexpr uint32_t AF_INET_ = 2;
                *reinterpret_cast<uint32_t*>(regs.esp + 0x00) = AF_INET_;   // socket(af, SOCK_DGRAM, IPPROTO_UDP)
                *reinterpret_cast<uint32_t*>(regs.esp + 0x14) = AF_INET_;   // the lookup hints' ai_family
            });

            // Aspect ratio, worked out by the game for every frame
            pattern = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 11 04 24 D9 04 24 83 C4 0C");
            static auto AspectRatioHook = safetyhook::create_mid(pattern.get_first(8), [](SafetyHookContext& regs)
            {
                AspectRatio::Update();
            });

            // FOV, HUD and movies are always worked out for 16:9 and corrected for the screen here
            {
                static float fRatio = 1.777777777f;
                pattern = hook::pattern("A1 ? ? ? ? 83 EC 0C 80 B8 ? ? ? ? 00 ? ? B9 ? ? ? ? E8");
                injector::WriteMemory<uint8_t>(pattern.get_first(15), 0xEB, true); // jmp
                injector::WriteMemory(pattern.get_first(89), &fRatio, true);
                pattern = hook::pattern("D9 C9 DF F1 DD D8 ? ? 0F B7 C3");
                injector::MakeNOP(pattern.get_first(6), 2, true);
                pattern = hook::pattern("74 ? 8B 16 8B 42 ? FF D0 5E 5B");
                injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jmp
            }

            // FOV width and height
            pattern = hook::pattern("0F 57 C0 F3 0F 10 ? ? ? ? ? F3 0F 10 ? ? ? F3 0F 10 ? ? ? ? ? 8D 7E");
            static auto FOVHook = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
            {
                if (*reinterpret_cast<int*>(regs.ebp + 0x08) <= 0x19)
                    *reinterpret_cast<float*>(regs.esp + 0x48) /= AspectRatio::fScreenAspectRatio;
            });

            // FOV scaling of the player views: the horizontal and vertical factors of the widescreen FOV, switched
            // in the options menu at once
            pattern = hook::pattern("DC 0D ? ? ? ? D9 5C 24 ? D9 44 24 ? DC 0D ? ? ? ? ? ? D9 44 24 ? DC 0D");
            OriginalHorizontalFov.SetAddress(*pattern.get_first<double*>(2));
            FOV::dOriginalHorizontal = OriginalHorizontalFov;
            OriginalVerticalFov.SetAddress(*pattern.get_first<double*>(16));
            FOV::dOriginalVertical = OriginalVerticalFov;
            FOV::Update();
            Redirect(pattern.get_first(2), &FOV::dHorizontal);
            Redirect(pattern.get_first(16), &FOV::dVertical);

            // Field of view of the driving cameras: DrivingCameraMover::Update writes its FOV (a 16 bit angle) to
            // the camera at the end of every update. The option multiplies it there, so the menus,
            // movies, the garage and the race intro cameras keep theirs. On screens wider than 4:3 the player view's
            // projection scales the half-angle by (1.175 - (degrees - 10) * 0.003) times the horizontal factor and
            // widens its tangent by the screen's aspect relative to 16:9.
            pattern = hook::pattern("8B 86 A8 00 00 00 8B 56 1C F3 0F 2C 08 66 89 8A 84 01 00 00");
            static std::vector<SafetyHookMid> DrivingFOVHooks;
            NFSFieldOfView::InstallDrivingHooks(pattern, 13, 1, DrivingFOVHooks, +[](SafetyHookContext& regs)
            {
                bool wide = ScreenAspectRatio() > 1.34f;
                float maximum = NFSFieldOfView::HorizontalLimit(150.0f * 65536.0f / 360.0f, AspectRatio::fScreenAspectRatio,
                    [wide](float angle)
                    {
                        return wide ? (1.175f - (angle * 360.0f / 65536.0f - 10.0f) * 0.003f) * float(FOV::dHorizontal) : 1.0f;
                    });
                regs.ecx = NFSFieldOfView::ScaleDrivingAngle(regs.ecx, *FOV::pFovFactor, maximum);
            });

            // HUD width: 4:3 / the screen aspect ratio
            pattern = hook::pattern("DC 3D ? ? ? ? D9 5C 24 0C F3 0F 10");
            injector::WriteMemory(pattern.get_first(2), &AspectRatio::dHudWidth, true);

            // Movies: 16:9, fitted into the screen
            pattern = hook::pattern("F3 0F 10 ? ? ? ? ? F3 0F 10 ? ? ? ? ? 0F 57 C0 F3 0F 10");
            injector::WriteMemory(pattern.get_first(4), &AspectRatio::fMovieTop, true);
            injector::WriteMemory(pattern.get_first(12), &AspectRatio::fMovieLeft, true);
            injector::WriteMemory(pattern.get_first(23), &AspectRatio::fMovieRight, true);
            injector::WriteMemory(pattern.get_first(65), &AspectRatio::fMovieBottom, true);

            // The front end is made for 16:9 or 4:3. The game normally switches at 1.34, which stretched 16:10;
            // it switches halfway between the two, and the front end is scaled down to fit the screen.
            pattern = hook::pattern("0F B6 C0 89 01 B0 01");
            if (pattern.empty())
            {
                pattern.get_first();
                std::abort();
            }
            if (pattern.size() != 7)
                std::abort();
            static auto WidescreenModeHook = safetyhook::create_mid(pattern.get(0).get<void>(0), [](SafetyHookContext& regs)
            {
                float aspect = ScreenAspectRatio();
                bool widescreen = aspect >= 1.5555555f;
                FEScale::fFEScale = std::min(1.0f, aspect / (widescreen ? (16.0f / 9.0f) : (4.0f / 3.0f)));
                regs.eax = widescreen ? 1 : 0;
            });

            pattern = hook::pattern("F3 0F 11 44 24 14 F3 0F 10 05 ? ? ? ? 6A 00");
            static auto FEScaleHook = safetyhook::create_mid(pattern.get_first(14), [](SafetyHookContext& regs)
            {
                // Movies are fitted by themselves
                float scale = FEScale::bMovieShown ? 1.0f : FEScale::fFEScale;
                *reinterpret_cast<float*>(regs.esp + 0x14) *= scale;
                regs.xmm0.f32[0] = scale;
            });

            // Movies started by the front end, except for its background movie
            pattern = hook::pattern("8D 94 24 7C 01 00 00 55");
            static auto FETransform = reinterpret_cast<void(*)()>(hook::get_pattern("8D 4C 24 18 51 68 89 13 00 00", -0x9E));
            static auto MovieStartHook = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
            {
                std::string_view filename = reinterpret_cast<const char*>(regs.esp + 0x13C);
                if (filename.find("FE_BG") == std::string_view::npos && !FEScale::bMovieShown)
                {
                    FEScale::bMovieShown = true;

                    // The front end transform is updated before the movie player locks it
                    FETransform();
                }
            });

            pattern = hook::pattern("C7 05 ? ? ? ? 01 00 00 00 A1 ? ? ? ? 85 C0 0F 84 ? ? ? ? 8B 35");
            static auto MovieShutdownHook = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
            {
                FEScale::bMovieShown = false;
            });

            // Cutscene borders: the ICE manager, from the letterbox channel's frame update
            pattern = hook::pattern("A1 ? ? ? ? 56 8B 70 70 85 F6 57 8B F9 74");
            Cutscene::pBorders = &FixSettings().Int("MAIN.CutsceneBorders");
            Cutscene::pAnimated = &FixSettings().Int("MAIN.AnimatedCutsceneBorders");
            TheICEManager.SetAddress(*pattern.get_first<uintptr_t*>(1));
            Cutscene::shUpdateLetterbox = safetyhook::create_inline(pattern.get_first(), Cutscene::UpdateLetterbox);
            WFP::onEndScene() += []()
            {
                if (!Cutscene::IsPlaying())
                {
                    Cutscene::LetterboxCamera = nullptr;
                    return;
                }
                if (FEPackageLoaded("HUD_NISFlashers.fng") && *Direct3DDevice && **Direct3DDevice)
                    Cutscene::Draw(**Direct3DDevice);
            };

            // Front end screens made for 16:9 (loading screens, the race match-up, the race day map, in-game dialogs) show
            // the world or parked objects of their package beside them on wider screens: the sides are black there.
            // The front end's dialogs dim the whole screen instead (their backdrop grows with it).
            WFP::onEndScene() += []()
            {
                constexpr const char* Screens[] = { "Loading.fng", "Loading_Tips.fng", "Loading_Controller.fng", "trackStreamerLoading.fng",
                    "FeEventMatchup.fng", "FERaceDayHubMap.fng", "InGameDialog.fng" };
                if (ScreenAspectRatio() <= 16.0f / 9.0f + 0.01f || !*Direct3DDevice || !**Direct3DDevice)
                    return;
                for (auto name : Screens)
                {
                    // Dialogs of the race's pause menu are over the full-width world
                    if (IsRacing() && FEPackageLoaded("FePauseMain.fng") && std::string_view(name) == "InGameDialog.fng")
                        continue;
                    if (FEPackageLoaded(name))
                    {
                        Cutscene::Pillarbox(**Direct3DDevice);
                        break;
                    }
                }
            };

            // Crowd culling distance, for the wider FOV
            pattern = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 11 44 24 7C ? ? ? ? ? ? 2B C3 83 E8 01");
            static auto CrowdCullHook = safetyhook::create_mid(pattern.get_first(8), [](SafetyHookContext& regs)
            {
                regs.xmm0.f32[0] = 0.6f / AspectRatio::fScreenAspectRatio;
            });

            // Brightness: the gamma ramp was built with the exponent's built-in default (0.81) at start, not with the
            // brightness setting, until the setting was changed. The ramp is built from the setting, with the game's
            // own mapping; without a setting the default is 50% (exponent 1.0).
            {
                auto gammaInteger = hook::pattern("A1 ? ? ? ? 8B 4C 24 04 8B D0 B8 67 66 66 66");
                if (gammaInteger.empty())
                    gammaInteger = hook::pattern("A1 ? ? ? ? 8B 4C 24 04 8B D0 83 C2 04 B8 CD CC CC CC");   // demo
                GammaInteger.SetAddress(*gammaInteger.get_first<int32_t*>(1));

                injector::WriteMemory<float>(*hook::pattern("F3 0F 11 44 24 04 D8 64 24 04 C6 ? 01 D9 5C").get_first<float*>(29), 1.0f, true);

                pattern = hook::pattern("55 8B EC 83 E4 F8 83 EC 10 ? ? ? ? ? ? ? ? 56 57 33 FF");
                static auto GammaHook = safetyhook::create_mid(pattern.get_first(17), [](SafetyHookContext& regs)
                {
                    // The mapping of the game's brightness setter: exponent = 1.5 - setting / 100 (setting 0 to 100)
                    if (GammaInteger.get() > 0)
                        regs.xmm0.f32[0] = 1.5f - std::clamp(GammaInteger.get() / 100.0f, 0.0f, 1.0f);
                });
            }

            if (bSkipIntro)
            {
                // EA bumper and attract movies
                pattern = hook::pattern("68 ? ? ? ? 6A ? FF D2 D9 EE");
                injector::WriteMemory(pattern.get_first(1), &"SkipThis", true);
                pattern = hook::pattern("68 ? ? ? ? 6A ? 8B CE FF D2");
                injector::WriteMemory(pattern.get_first(1), &"SkipThis", true);

                // The title screen: it continues by itself once it's shown, as if Start was pressed after its
                // minimum time. That also resets the profile data, before the profile is loaded.
                auto padStart = hook::pattern("51 56 8B F1 83 7E 04 11 74");
                auto screenTick = hook::pattern("83 EC 08 56 8B F1 83 7E 04 11 74");
                static auto HandlePadStart = reinterpret_cast<void(__thiscall*)(uint8_t*)>(padStart.get_first());
                RealTimer.SetAddress(*padStart.get_first<int32_t*>(0x2B));
                static SafetyHookInline shScreenTick = {};

                struct BootFlow
                {
                    static void __fastcall ScreenTick(uint8_t* self, void*)
                    {
                        constexpr int32_t SplashScreenState = 17;
                        static bool bDone = false;
                        if (!bDone && *reinterpret_cast<int32_t*>(self + 4) == SplashScreenState)
                        {
                            bDone = true;
                            *reinterpret_cast<int32_t*>(self + 0xC8) = *RealTimer - 4000 * 5;     // timer ticks, 4000 per second
                            HandlePadStart(self);
                            return;
                        }
                        shScreenTick.unsafe_thiscall(self);
                    }
                };
                shScreenTick = safetyhook::create_inline(screenTick.get_first(), BootFlow::ScreenTick);

                // The profile and career are loaded at startup like the Xbox 360 version does: only errors are
                // shown, not "Load complete" with a button to press
                auto load = hook::pattern("8B 44 24 04 56 8B F1 89 46 20 C6 05 ? ? ? ? 00 8B 46 20 85 C0 0F 84");
                auto bootFlowStateChange = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 51 56 A1 ? ? ? ? 33 C4 50 8D 44 24 0C 64 A3 00 00 00 00 8B F1 8B 8E D8 00 00 00 8B 01 8B 50 0C FF D2 8B 4E 04");
                static bool bBooting = true;
                static SafetyHookInline shLoad = {};

                struct Memcard
                {
                    // MemcardManager::Load(file, silent)
                    static void __fastcall Load(void* manager, void*, void* file, bool silent)
                    {
                        shLoad.unsafe_thiscall(manager, file, silent || bBooting);
                    }
                };
                shLoad = safetyhook::create_inline(load.get_first(), Memcard::Load);

                // The boot flow is done once it switches to its exit state
                static auto BootFlowStateChangeHook = safetyhook::create_mid(bootFlowStateChange.get_first(0x31), [](SafetyHookContext& regs)
                {
                    if (*reinterpret_cast<int32_t*>(regs.esi + 4) == -1)
                        bBooting = false;
                });
            }

            // Brake lights: while the car moves forward they only came on above 80% brake, so with a pad's trigger they
            // mostly showed when ABS pulsed the brake fully. They come on with any brake, as when reversing.
            {
                static constexpr double d0 = 0.0;
                pattern = hook::pattern("DD 05 ? ? ? ? D9 C9 DF F1 DD D8 77 15 8B 86");
                injector::WriteMemory(pattern.get_first(2), &d0, true);
            }

            // Sparks
            {
                auto drawBatchCall = hook::pattern("E8 ? ? ? ? 8B 4E 08 83 C4 04 51 E8 ? ? ? ? 8B 56 08 5E");
                auto setTextureCall = hook::pattern("6A 20 6A 00 53 6A 00 57 FF D1 8B 46 14 8B 17");
                pattern = hook::pattern("A1 ? ? ? ? 53 8B 5C 24 08 56 57 8B 38 74 19");
                XenonEffects::DrawBatchFunc = reinterpret_cast<decltype(XenonEffects::DrawBatchFunc)>(pattern.get_first(-7));
                auto setTexture = hook::pattern("56 57 8B 7C 24 0C 8B 17 8B 52 10 8B F1 8B 46 18 8B 08 52");
                if (setTexture.empty())
                {
                    setTexture.get_first();
                    std::abort();
                }
                if (setTexture.size() != 2)
                    std::abort();
                XenonEffects::SetTextureFunc = reinterpret_cast<decltype(XenonEffects::SetTextureFunc)>(setTexture.get(0).get<void>(0));
                injector::MakeCALL(drawBatchCall.get_first(0xC), XenonEffects::DrawBatch, true);
                injector::MakeCALL(setTextureCall.get_first(0x25), XenonEffects::SetTexture, true);
            }

            // Motion blur off without turning off the other World FX: its branch is skipped. Switched in the options
            // menu at once.
            pattern = hook::pattern("74 73 F3 0F 10 84 24 ? ? ? ? F3 0F 11 44 24");
            static ProtectedGameRef<uint8_t> MotionBlurBranch;
            MotionBlurBranch.SetAddress(pattern.get_first(0));
            static auto UpdateMotionBlur = []()
            {
                uint8_t opcode = *pDisableMotionBlur ? 0xEB : 0x74;    // jmp or jz
                if (MotionBlurBranch != opcode)
                    MotionBlurBranch = opcode;
            };
            UpdateMotionBlur();
            WFP::onGameProcessEvent() += []() { UpdateMotionBlur(); };

            WFP::onGameProcessEvent() += []() { FOV::Update(); };

            // Shadow map size for every quality level
            if (nShadowsRes != 0)
            {
                auto shadowPattern = hook::pattern("B8 ? ? ? ? EB 0C B8 ? ? ? ? EB 05 B8 ? ? ? ? 57 6A 00");
                injector::WriteMemory(shadowPattern.get_first(1), nShadowsRes, true);
                injector::WriteMemory(shadowPattern.get_first(8), nShadowsRes, true);
                injector::WriteMemory(shadowPattern.get_first(15), nShadowsRes, true);
            }

            // Window procedure, and the input blocker for online races
            {
                auto wndProc = hook::pattern("C7 44 24 2C 30 00 00 00 C7 44 24 30 40 00 00 00 C7 44 24 34 ? ? ? ? 89 6C 24 38").get_first<WNDPROC>(0x14);
                OnlineInputBlocker::GameWndProc = *wndProc;
                injector::WriteMemory(wndProc, &OnlineInputBlocker::WndProc, true);

                OnlineActiveGameManager.SetAddress(*hook::pattern("8B 0D ? ? ? ? 89 3D ? ? ? ? 3B CF 5F 5E").get_first<uintptr_t*>(2));

                auto gameDeviceVTable = *hook::pattern("64 A3 00 00 00 00 8B F1 89 74 24 10 8D 7E 2C C7 06").get_first<uintptr_t>(0x11);
                OnlineInputBlocker::ptrGameDevicePoll = *reinterpret_cast<uintptr_t*>(gameDeviceVTable + 0x10);
                injector::WriteMemory(gameDeviceVTable + 0x10, &OnlineInputBlocker::GameDevicePoll, true);

                auto wheelVTable = *hook::pattern("8B 44 24 20 0F 57 C0 89 46 18 C7 06").get_first<uintptr_t>(0xC);
                OnlineInputBlocker::ptrSteeringWheelDevicePoll = *reinterpret_cast<uintptr_t*>(wheelVTable + 0x10);
                injector::WriteMemory(wheelVTable + 0x10, &OnlineInputBlocker::SteeringWheelDevicePoll, true);

                // Switched at once: the game gets the focus change it skipped (or missed) while the window is in the
                // background
                OnlineInputBlocker::pPauseOnFocusLoss = &FixSettings().Int("MAIN.PauseOnFocusLoss");
                FixSettings().OnChange("MAIN.PauseOnFocusLoss", []()
                {
                    auto window = hWnd.get();
                    if (!window || GetForegroundWindow() == window)
                        return;
                    bool active = !FixSettings().Bool("MAIN.PauseOnFocusLoss");
                    PostMessage(window, WM_ACTIVATE, active ? WA_ACTIVE : WA_INACTIVE, 0);
                    PostMessage(window, WM_ACTIVATEAPP, active ? TRUE : FALSE, 0);
                });
            }

            // Frame rate limit and simulation rate: -1 = monitor refresh rate, -2 = twice the refresh rate.
            // The game needs a fixed rate, so it can't be unlimited.
            if (nFPSLimit == -2)
                nFPSLimit = GetDesktopRefreshRate() * 2;
            else if (nFPSLimit <= 0)
                nFPSLimit = GetDesktopRefreshRate();
            SimRate = std::clamp(nFPSLimit, 30, 1000);
            FrameRate::fStepTime = 1.0f / static_cast<float>(SimRate);
            FrameRate::dStepTime = 1.0 / static_cast<double>(SimRate);
            FrameRate::fFullSpeedTime = FrameRate::fStepTime * 10.0f;
            {
                // Sim::QueueEvents(dt): the steps that are due, by the step time as a float and as a double
                pattern = hook::pattern("33 DB 39 1D ? ? ? ? 74 36 0F 57 C0 0F 2F 44 24 3C 73 2C D9 05");
                SimActive.SetAddress(*pattern.get_first<int32_t*>(4));
                TimeBank.SetAddress(*pattern.get_first<float*>(0x16));
                static auto QueueEventsHook = safetyhook::create_mid(pattern.get_first(-0x26), [](SafetyHookContext& regs)
                {
                    FrameRate::PrepareStep(*reinterpret_cast<float*>(regs.esp + 4));
                });
                Redirect(pattern.get_first(0x28), &FrameRate::fStepTime);
                Redirect(pattern.get_first(0x59), &FrameRate::dStepTime);

                // The simulation's step time: set up in its constructor, and every step it's the time scale (slow
                // motion and so on, eased by the time scale controller with the step time) times the step time
                Redirect(hook::get_pattern("A1 ? ? ? ? F3 0F 10 05 ? ? ? ? 89 46 60 8B 0D", 9), &FrameRate::fStepTime);
                Redirect(hook::get_pattern("D9 46 1C 8B 10 83 EC 08 D9 5C 24 04 8B C8 D9 05", 16), &FrameRate::fStepTime);
                Redirect(hook::get_pattern("D9 46 1C 83 7E 24 03 DC 0D ? ? ? ? 5F", 9), &FrameRate::dStepTime);

                // The step event adds up the unscaled simulation time of the frame, which the world and the
                // effects are updated with
                Redirect(hook::get_pattern("D9 46 14 DC 05 ? ? ? ? D9 5E 14 7D", 5), &FrameRate::dStepTime);

                // The main loop's full speed mode, which the game turns on for a moment around race starts (until
                // the intro NIS runs), advances ten steps per frame
                Redirect(hook::get_pattern("D8 44 24 08 D9 5C 24 20 EB 47 83 3D", 0x47), &FrameRate::fFullSpeedTime);

                // Boost grip of the drag race burnout, scaled with the step time of every simulate call
                auto boostAdd = hook::pattern("D9 83 80 03 00 00 DC 05 ? ? ? ? D9 5C 24 14");
                auto boostSub = hook::pattern("D9 83 80 03 00 00 DC 25 ? ? ? ? D9 5C 24 14");
                auto simulate = hook::pattern("55 8B EC 83 E4 F8 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 81 EC F8 02 00 00");
                OriginalBoostAdder.SetAddress(*boostAdd.get_first<double*>(8));
            BoostGrip::dOriginalAdder = BoostGrip::dAdder = OriginalBoostAdder;
                OriginalBoostSubtractor.SetAddress(*boostSub.get_first<double*>(8));
            BoostGrip::dOriginalSubtractor = BoostGrip::dSubtractor = OriginalBoostSubtractor;
                Redirect(boostAdd.get_first(8), &BoostGrip::dAdder);
                Redirect(boostSub.get_first(8), &BoostGrip::dSubtractor);

                // thiscall OnTaskSimulate(float dT), at its entry
                static auto SimulateHook = safetyhook::create_mid(simulate.get_first(0), [](SafetyHookContext& regs)
                {
                    double scale = std::clamp(static_cast<double>(*reinterpret_cast<float*>(regs.esp + 4)), 0.0, 0.1) * 60.0;
                    BoostGrip::dAdder = BoostGrip::dOriginalAdder * scale;
                    BoostGrip::dSubtractor = BoostGrip::dOriginalSubtractor * scale;
                });
            }

            WFP::onEndScene() += []()
            {
                FrameRate::LimitFrameRate();
            };
        };
    }
} Misc;
