module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/NFS/FieldOfView.hpp"
#include <d3d9.h>

export module Misc;

import ComVars;

// Shadows: the cascades of the shadow map sit side by side in one texture, so its width is the
// resolution times the number of cascades. The resolution is limited by the device and by memory,
// and the cascades stay square.
namespace Shadows
{
    uint32_t iniShadowsRes = 2048;
    uint32_t ShadowsRes = 2048;     // one cascade
    uint32_t ShadowsResX = 6144;    // the whole texture
    float fShadowsRes = 2048.0f;

    void UpdateShadowsRes()
    {
        uint32_t levels = std::max(ShadowLevel, 1u);
        uint32_t maxWidth = 16384, maxHeight = 16384;
        if (IDirect3DDevice9* device = Direct3DDevice)
        {
            D3DCAPS9 caps = {};
            if (SUCCEEDED(device->GetDeviceCaps(&caps)))
            {
                maxWidth = caps.MaxTextureWidth;
                maxHeight = caps.MaxTextureHeight;
            }
        }

        // 32-bit depth, at most 256 MB for the whole texture
        uint32_t memoryLimit = static_cast<uint32_t>(std::sqrt(double(256u * 1024u * 1024u / 4u) / double(levels)));
        ShadowsRes = std::clamp(iniShadowsRes, 256u, std::min({ maxWidth / levels, maxHeight, memoryLimit }));
        ShadowsResX = ShadowsRes * levels;
        fShadowsRes = static_cast<float>(ShadowsRes);
    }

    uintptr_t TextureCaveExit = 0;
    void __declspec(naked) TextureCave()
    {
        _asm
        {
            mov ShadowLevel, eax
            pushad
            call UpdateShadowsRes
            popad
            mov eax, ShadowsResX
            mov ecx, ShadowsRes
            push ecx
            push eax
            jmp TextureCaveExit
        }
    }
}

// Screen width functions, replaced for the GPS map background and the transition overlay
namespace FullWidth
{
    float fPositive = 99999.0f;
    float fNegative = -99999.0f;

    void __declspec(naked) LoadPositive()
    {
        _asm
        {
            fld dword ptr ds:[fPositive]
            ret
        }
    }

    uintptr_t GPSMapExit = 0;
    void __declspec(naked) GPSMapCave()
    {
        _asm
        {
            fld dword ptr ds:[fNegative]
            fst dword ptr [esp + 4]
            jmp GPSMapExit
        }
    }
}

// Frame rate and simulation rate. The game simulates in fixed steps of 1/60 s: Sim::QueueEvents turns the frame's
// time into a number of steps, SimSystem::UpdateState makes every step 1/60 s long, and each step adds 1/60 s to the
// frame's real time, which the camera, the in-game cutscenes (ICE) and the front end are updated with. The main loop
// itself doesn't wait, only V-Sync limits it. The step is set to a multiple of the frame rate limit, at least 60 per
// second, in all of these places, and frames are paced to the limit precisely. Real time is still kept: slower frames
// get more steps.
namespace FrameRate
{
    float fStepTime = 1.0f / 60.0f;
    double dStepTime = 1.0 / 60.0;
    float fStepRate = 60.0f;
    float fFrameTime = 1.0f / 60.0f;    // the frame rate limit

    // The time the steps of this frame simulate. The chase camera's springs (DC::Damp) assume 1/60 s per update and
    // are updated once per frame that has steps, so they're given this instead.
    float fCameraStep = 1.0f / 60.0f;
    double dCameraStep = 1.0 / 60.0;

    // Sim::QueueEvents runs at most this many steps in a frame and drops the rest (8 at 60 per second, 2/15 s)
    int32_t MaxSteps = 8;

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

        LONGLONG period = static_cast<LONGLONG>(double(frequency.QuadPart) * fFrameTime);
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
    int32_t* SimActive = nullptr;
    float* TimeBank = nullptr;
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
            // Less than half a step: no step this frame, the time waits in the carry. With no time the game queues
            // nothing and leaves its time bank alone.
            Carry = due - previous;
            dt = 0.0f;
            return;
        }

        // A little more than the steps, so that the game's truncation gives exactly this many
        float target = (static_cast<float>(steps) + 0.001f) * fStepTime;
        Carry = due - target;
        dt = target - previous;

        fCameraStep = static_cast<float>(std::min(steps, MaxSteps)) * fStepTime;
        dCameraStep = static_cast<double>(std::min(steps, MaxSteps)) * dStepTime;
    }

    // Points the instructions in [begin, end) that read the constant at `from` to `to`, if there are exactly `expected`
    // of them (a 4-byte absolute address in this code is always such an operand)
    bool RedirectConstant(uintptr_t begin, uintptr_t end, uintptr_t from, const void* to, size_t expected)
    {
        std::vector<uintptr_t> sites;
        for (auto p = begin; p + 4 <= end; ++p)
        {
            if (*reinterpret_cast<uint32_t*>(p) == from)
                sites.push_back(p);
        }
        if (sites.size() != expected)
            return false;
        for (auto site : sites)
            injector::WriteMemory(site, reinterpret_cast<uintptr_t>(to), true);
        return true;
    }
}

// Animated textures (signs, screens) take their frame from a 60 Hz tick count: the world's in the race, the real
// time's otherwise (sub_53CD60 steam). AdvanceRealTime counts at least one tick per frame, so above 60 FPS those
// animations ran faster. They get a count of real 60 Hz ticks instead.
namespace TextureAnimation
{
    int32_t RealTicks = 0;

    void Update()
    {
        static LARGE_INTEGER frequency = []() { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
        static LARGE_INTEGER start = []() { LARGE_INTEGER s; QueryPerformanceCounter(&s); return s; }();
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        RealTicks = static_cast<int32_t>((now.QuadPart - start.QuadPart) * 60 / frequency.QuadPart) + 1;
    }
}

// Scenery options of the world detail menu, applied while the game runs
namespace Scenery
{
    const int* pImproveLOD = nullptr;
    const int* pDisablePreculler = nullptr;
    const int* pBruteforceCulling = nullptr;

    // Scenery LOD mask of the highest world detail: 0x0E, 0x0F shows the last LOD step too (more scenery far away)
    ProtectedGameRef<uint8_t> LodMask([]() -> uint8_t*
    {
        auto pattern = hook::pattern("C6 44 24 ? ? 75 ? C6 44 24 ? ? 83 BF");
        if (!pattern.empty() && *pattern.get_first<uint8_t>(4) == 0x0E)
            return pattern.get_first<uint8_t>(4);
        return nullptr;
    });

    // Preculler mode (1): the precomputed visibility of PrecullerBooBooScript.hoo; 0 turns it off
    ProtectedGameRef<int32_t> PrecullerMode([]() -> int32_t*
    {
        auto pattern = find_pattern("8B 0D ? ? ? ? 85 C9 B3 ? 0F 84", "8B 0D ? ? ? ? 85 C9 B3 ? 74 ? 83 F9");
        if (!pattern.empty())
            return *pattern.get_first<int32_t*>(2);
        return nullptr;
    });

    // The jz that picks ScenerySectionHeader::TreeCull for the main views; without it CullBruteForce is used for all
    ProtectedGameRef<uint16_t> TreeCullBranch([]() -> uint16_t*
    {
        auto pattern = hook::pattern("84 C0 ? 8B CE 74 07 E8");
        if (!pattern.empty())
            return pattern.get_first<uint16_t>(5);
        return nullptr;
    });

    void Apply()
    {
        if (LodMask.get_ptr())
        {
            uint8_t mask = *pImproveLOD ? 0x0F : 0x0E;
            if (LodMask != mask)
                LodMask = mask;
        }
        if (PrecullerMode.get_ptr())
        {
            int32_t mode = *pDisablePreculler ? 0 : 1;
            if (PrecullerMode != mode)
                PrecullerMode = mode;
        }
        if (TreeCullBranch.get_ptr())
        {
            uint16_t branch = *pBruteforceCulling ? 0x9090 : 0x0774;
            if (TreeCullBranch != branch)
                TreeCullBranch = branch;
        }
    }
}

// Extra field of view for the driving cameras, in percent, from the options menu
namespace FieldOfView
{
    const float* pFovFactor = nullptr;
}

// Movies (all 16:9) are drawn as a quad over the whole screen on screens wider than 4:3, which stretched them on wider
// ones, and over 75% of the height on others. The movie always takes the whole-screen quad, sized for the screen:
// original aspect (0) with black bars, fill (1) keeping its aspect and cropped to cover the screen, or stretch (2).
// Switched in the options menu at once.
namespace Movie
{
    const int* pMode = nullptr;
    bool bDrawing = false;      // the movie's quad is drawn next

    // The movie's check for a widescreen screen
    bool __cdecl IsWidescreen()
    {
        bDrawing = true;
        return true;
    }

    // Scales the quad's vertices ({x, y, z, w, u, v}, four of them) for the screen
    void Scale(float* vertices)
    {
        if (!Width || !Height || !*Width || !*Height)
            return;
        float aspect = (static_cast<float>(*Width) / static_cast<float>(*Height)) / (16.0f / 9.0f);
        float x = 1.0f, y = 1.0f;
        int mode = pMode ? *pMode : 1;
        if (mode == 0)
        {
            if (aspect > 1.0f)
                x = 1.0f / aspect;      // pillarboxes
            else
                y = aspect;             // letterboxes
        }
        else if (mode == 1)
        {
            if (aspect > 1.0f)
                y = aspect;             // cropped at the top and bottom
            else
                x = 1.0f / aspect;      // cropped at the sides
        }
        for (int i = 0; i < 4; ++i)
        {
            vertices[i * 6 + 0] *= x;
            vertices[i * 6 + 1] *= y;
        }
    }
}

// In-engine cutscenes (the ICE scenes) have black bars above and below: the letterbox channel of a scene sets the
// player camera's letterbox height (the bars' share of the screen height, up to 24%, eased in and out by the scene),
// and the renderer draws the bars from it. Letterbox (1) keeps them; without it the height is 0 and the player view is
// zoomed so that it shows what was between the bars, nothing that they hid. Every view of the scene's camera is zoomed
// alike (the player view and the views drawn with it: depth texture, shadow matte, glow, ...), or their passes no longer
// line up with the player view and parts of the picture stay black. Pillarbox (2) keeps the scenes at 16:9 on
// wider screens: the sides are cleared before the frame is shown while a scene's camera runs. Animated borders off
// shows the bars at their full height at once. All switched in the options menu at once.
namespace Cutscene
{
    const int* pBorders = nullptr;
    const int* pAnimated = nullptr;
    uintptr_t* ICEManager = nullptr;    // TheICEManager; its camera mover (+0x90) exists while a scene's camera runs
    float fLetterBox = 0.0f;            // the bars' height the game would show
    bool bCameraView = false;           // the view whose projection is set up shows the scene's camera

    bool IsPlaying()
    {
        return ICEManager && *ICEManager && *reinterpret_cast<uintptr_t*>(*ICEManager + 0x90);
    }

    // The camera of the scene's camera mover
    uintptr_t Camera()
    {
        return IsPlaying() ? *reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(*ICEManager + 0x90) + 4) : 0;
    }

    void DrawPillarbox(IDirect3DDevice9* device)
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
        LONG bar = (width - static_cast<LONG>(std::lround(height * 16.0 / 9.0))) / 2;
        if (bar <= 0)
            return;

        D3DVIEWPORT9 viewport = {};
        device->GetViewport(&viewport);
        DWORD scissor = FALSE;
        device->GetRenderState(D3DRS_SCISSORTESTENABLE, &scissor);
        D3DVIEWPORT9 full = { 0, 0, desc.Width, desc.Height, 0.0f, 1.0f };
        device->SetViewport(&full);
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DRECT rects[] = { { 0, 0, bar, height }, { width - bar, 0, width, height } };
        device->Clear(2, rects, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, scissor);
        device->SetViewport(&viewport);
    }
}

// The options menu lists the resolutions of the display
void InitResolutionList()
{
    struct ScreenRes
    {
        uint32_t ResX;
        uint32_t ResY;
        uint32_t LangHash;
    };

    static std::vector<std::string> list;
    static std::vector<ScreenRes> list2;
    static std::vector<uint32_t> list3;
    GetResolutionsList(list);
    if (list.empty())
        return;

    for (auto& s : list)
    {
        ScreenRes r = {};
        sscanf_s(s.c_str(), "%dx%d", &r.ResX, &r.ResY);
        list2.emplace_back(r);
    }
    list3.resize(list2.size() + 1);
    for (size_t i = 0; i < list2.size(); ++i)
        list3[i] = reinterpret_cast<uint32_t>(&list2[i]);
    list3[list2.size()] = 0;

    auto pattern = GetPattern("E8 ? ? ? ? 33 FF 39 ? ? ? ? ? 76"); //0x587291
    auto names = GetPattern("E8 ? ? ? ? 89 44 BC ? 83 C7 01 83 C4 04"); //0x5872AB
    if (pattern.empty() || names.empty())
        return;

    injector::MakeNOP(pattern.get_first(0), 5, true);
    injector::WriteMemory(*pattern.get_first<void*>(9), list3.size() - 1, true);//0x12AA194

    for (auto p : { "8B 14 8D ? ? ? ? ? ? 50", "8B 14 8D ? ? ? ? 8B 42 04 50", "8B 04 8D ? ? ? ? 8B 50", "8B 04 BD ? ? ? ? 8B 48", "8B 14 8D ? ? ? ? 39 3A" })
    {
        auto ref = GetPattern(p);
        if (!ref.empty())
            injector::WriteMemory(ref.get_first(3), &list3[0], true);
    }

    pattern = GetPattern("8B 0C 85 ? ? ? ? 89 4F 6C 8B 46 10"); //0x74006F
    if (!pattern.empty())
    {
        injector::WriteMemory(pattern.get_first(3), &list2[0], true);
        injector::WriteMemory(pattern.get_first(19), &list2[0].ResY, true);//0x74007F
    }

    // The names of the resolutions. The options screen (case 20 of its state switch) collects them in a stack array of
    // 16 entries, and more display modes overran it into its other locals and up to the stack cookie. They go into an
    // array of the fix's instead: the store and its loop increment are replaced, and the screen's list of labels
    // (lea ecx, [esp+array]) points to it.
    static std::vector<const char*> Names(list.size() + 1, nullptr);
    struct StoreName
    {
        void operator()(injector::reg_pack& regs)
        {
            if (regs.edi < Names.size())
                Names[regs.edi] = regs.edi < list.size() ? list[regs.edi].data() : reinterpret_cast<const char*>(regs.eax);
            regs.edi += 1;
        }
    };
    auto store = names.get_first<uint8_t>(5);   // mov [esp+edi*4+array], eax; add edi, 1
    uint8_t arrayOffset = store[3];
    injector::MakeInline<StoreName>(store, store + 7);

    auto labels = hook::range_pattern(reinterpret_cast<uintptr_t>(store), reinterpret_cast<uintptr_t>(store) + 0xA0, "8D 4C 24 ?");
    for (size_t i = 0; i < labels.size(); ++i)
    {
        // Three pushes later the array is 8 bytes farther from esp
        if (*labels.get(i).get<uint8_t>(3) != uint8_t(arrayOffset + 8))
            continue;
        static std::vector<SafetyHookMid> hooks;
        hooks.push_back(safetyhook::create_mid(labels.get(i).get<void>(4), [](SafetyHookContext& regs)
        {
            regs.ecx = reinterpret_cast<uintptr_t>(Names.data());
        }));
    }
}

// Interactive music (PathFinder): its timer thread pauses and resumes the music streams while the main
// thread is still creating them. A stream that wasn't set up yet holds the allocator's fill value 0xAAAAAAAA,
// and the game crashed with it when a profile was loaded, mostly on the 1.1.2.1 executables. Such streams are
// skipped until they're set up.
void FixMusicStreamRace()
{
    // mov eax, [esi+ecx]; cmp dword ptr [eax+220h], 0; jz next
    auto pattern = hook::pattern("83 3C 0E 00 74 ? 8B 04 0E 83 B8 20 02 00 00 00 74 1E 85 DB 8B 80 20 02 00 00");
    if (pattern.empty())
        return;

    auto site = reinterpret_cast<uintptr_t>(pattern.get_first(6));
    auto next = reinterpret_cast<uintptr_t>(pattern.get_first(0x12)) + 0x1E;
    auto resume = reinterpret_cast<uintptr_t>(pattern.get_first(0x12));

    auto cave = static_cast<uint8_t*>(VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!cave)
        return;

    std::vector<uint8_t> code;
    auto jump = [&](std::initializer_list<uint8_t> opcode, uintptr_t target)
    {
        code.insert(code.end(), opcode);
        int32_t rel = int32_t(target - (reinterpret_cast<uintptr_t>(cave) + code.size() + 4));
        code.insert(code.end(), reinterpret_cast<uint8_t*>(&rel), reinterpret_cast<uint8_t*>(&rel) + 4);
    };
    code.insert(code.end(), { 0x8B, 0x04, 0x0E });                          // mov eax, [esi+ecx]
    code.insert(code.end(), { 0x3D, 0xAA, 0xAA, 0xAA, 0xAA });              // cmp eax, 0AAAAAAAAh
    jump({ 0x0F, 0x84 }, next);                                             // je next
    code.insert(code.end(), { 0x8B, 0x90, 0x20, 0x02, 0x00, 0x00 });        // mov edx, [eax+220h]
    code.insert(code.end(), { 0x85, 0xD2 });                                // test edx, edx
    jump({ 0x0F, 0x84 }, next);                                             // jz next
    code.insert(code.end(), { 0x81, 0xFA, 0xAA, 0xAA, 0xAA, 0xAA });        // cmp edx, 0AAAAAAAAh
    jump({ 0x0F, 0x84 }, next);                                             // je next
    jump({ 0xE9 }, resume);                                                 // jmp back
    memcpy(cave, code.data(), code.size());
    FlushInstructionCache(GetCurrentProcess(), cave, code.size());

    injector::MakeJMP(site, cave, true);
    injector::MakeNOP(site + 5, 7, true);
}

void Init1()
{
    FixMusicStreamRace();

    // Settings were reset after a crash
    auto pattern = GetPattern("C7 44 24 ? ? ? ? ? FF 15 ? ? ? ? 8D 54 24 0C 52");
    injector::WriteMemory(pattern.get_first(4), 0, true);

    static float fBloomIntensity = FixSettings().Float("GRAPHICS.BloomIntensity");

    pattern = GetPattern("D9 05 ? ? ? ? 50 83 EC 08 D9 5C 24 04"); //0x766FC5
    if (!pattern.empty())
        injector::WriteMemory(pattern.get_first(2), &fBloomIntensity, true);
}

void Init2()
{
    static bool bSkipIntro = FixSettings().Bool("MAIN.SkipIntro");

    auto pattern = GetPattern("C6 ? 98 00 00 00 01 A1 ? ? ? ? 85 C0 74 05 E8"); //0x575604
    static auto dword_12BB200 = *pattern.get_first<uint32_t*>(25);
    auto rpattern = hook::range_pattern((uintptr_t)pattern.get_first(-100), (uintptr_t)pattern.get_first(0), "A1 ? ? ? ? 33 C4 89 84 24"); //0x5755C4
    static auto dword_1272378 = *rpattern.get_first<uint32_t*>(1);

    struct MoviesHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *dword_1272378;
            *dword_12BB200 = 0;
            static bool bOnce = false;
            if (!bOnce)
            {
                InitResolutionList();
                if (bSkipIntro)
                    *dword_12BB200 = 1;
            }
            bOnce = true;
        }
    }; injector::MakeInline<MoviesHook>(rpattern.get_first(0));

    // Every display mode of at least 640x480 is offered, instead of a few fixed ones
    pattern = hook::pattern("83 C0 08 83 F8 78 72 E8 E9 ? ? ? ? 8B 56 3C 8B 46 10");
    if (!pattern.empty())
    {
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
    }

    if (bSkipIntro)
    {
        // The title screen continues by itself once it's shown, as if Start was pressed after its minimum time
        auto padStart = hook::pattern("56 8B F1 8B 46 04 83 E8 12 0F 84 ? ? ? ? 83 E8 02 74");
        auto screenTick = hook::pattern("56 8B F1 83 7E 04 14 74 ? 8B 4E 58 8B 01 8B 50 08 FF D2");
        auto realTimer = padStart.empty() ? padStart : hook::range_pattern(reinterpret_cast<uintptr_t>(padStart.get_first()), reinterpret_cast<uintptr_t>(padStart.get_first(0x100)), "A1 ? ? ? ? 2B 46 4C");
        if (!padStart.empty() && !screenTick.empty() && !realTimer.empty())
        {
            static auto HandlePadStart = reinterpret_cast<void(__thiscall*)(uint8_t*)>(padStart.get_first());
            static auto RealTimer = *realTimer.get_first<int32_t*>(1);
            static SafetyHookInline shScreenTick = {};

            struct BootFlow
            {
                static void __fastcall ScreenTick(uint8_t* self, void*)
                {
                    constexpr int32_t SplashScreenState = 20;
                    static bool bDone = false;
                    if (!bDone && *reinterpret_cast<int32_t*>(self + 4) == SplashScreenState)
                    {
                        bDone = true;
                        *reinterpret_cast<int32_t*>(self + 0x4C) = *RealTimer - 4000 * 5;     // timer ticks, 4000 per second
                        HandlePadStart(self);
                        return;
                    }
                    shScreenTick.unsafe_thiscall(self);
                }
            };
            shScreenTick = safetyhook::create_inline(screenTick.get_first(), BootFlow::ScreenTick);
        }
    }

    // Profiles are loaded like on the Xbox 360: only errors are shown, not "Load complete" with a button to press
    pattern = hook::pattern("8B 44 24 04 56 8B F1 89 46 20 C6 05 ? ? ? ? 00 8B 46 20 85 C0 0F 84");
    if (!pattern.empty())
    {
        static SafetyHookInline shLoad = {};
        struct Memcard
        {
            // MemcardManager::Load(file, silent)
            static void __fastcall Load(void* manager, void*, void* file, bool)
            {
                shLoad.unsafe_thiscall(manager, file, true);
            }
        };
        shLoad = safetyhook::create_inline(pattern.get_first(), Memcard::Load);
    }
}

void Init4()
{
    static int32_t nScaling = std::clamp(FixSettings().Int("GRAPHICS.Scaling"), 0, 1);
    int nFPSLimit = FixSettings().Int("MAIN.FPSLimit");
    bool bDisableMotionBlur = FixSettings().Bool("GRAPHICS.DisableMotionBlur");

    // HUD Refresh
    auto pattern = GetPattern("38 86 E3 00 00 00 5F");
    injector::MakeNOP(pattern.get_first(7), 2, true);

    // HUD Width
    static float fHudScale = 1.0f;
    static float fHudXPos = -1.0f;

    uintptr_t loc_764436 = reinterpret_cast<uintptr_t>(hook::pattern("C7 41 04 06 00 00 00 C7 01 04 00 00 00 C7 81 BC 00 00 00 04 00 00 00").get_first(0)) + 0x20;
    static float* fHudWidth = *reinterpret_cast<float**>(loc_764436 + 4);
    DWORD oldprotect;
    injector::UnprotectMemory(fHudWidth, sizeof(float), oldprotect);

    pattern = GetPattern("C7 40 ? ? ? ? ? 88 50 20");
    struct ResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.eax + 0x14) = 1;
            if (!Width || !Height)
            {
                Width = (uint32_t*)(regs.eax + 0x18);
                Height = (uint32_t*)(regs.eax + 0x1C);
            }
            float fAspectRatio = static_cast<float>(*Width) / static_cast<float>(*Height);
            fHudScale = 1280.0f / (720.0f * fAspectRatio);
            *fHudWidth = 1280.0f / fHudScale;
            fHudXPos = (fAspectRatio <= 1.34f ? -0.75f : -1.0f) * fHudScale;
        }
    }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(7));

    // 16:9 HUD settings for anything wider than 4:3
    pattern = GetPattern("D9 05 ? ? ? ? D9 C9 DF F1 DD D8 ? ?  B8 ? ? ? ? C3 33 C0 C3");
    struct HUDWidescreenModeHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float fAspectRatio = static_cast<float>(*Width) / static_cast<float>(*Height);
            regs.eax = (fAspectRatio <= 1.333333373f) ? 0 : 1;
        }
    };
    injector::MakeInline<HUDWidescreenModeHook>(pattern.count(1).get(0).get<uint32_t>(12), pattern.count(1).get(0).get<uint32_t>(19)); // 7454F1

    //HUD X Pos
    pattern = GetPattern("F3 0F 11 44 24 0C E8 ? ? ? ? 0F 57 C0 F3 0F 10 0D ? ? ? ? F3 0F 10 15");
    injector::WriteMemory(pattern.get_first(26), &fHudXPos, true);

    // HUD render width
    static float fRenderWidth_99999 = 99999.0f;
    pattern = hook::pattern("E8 ? ? ? ? 84 C0 74 0A F3 0F");
    if (pattern.size() == 13)
    {
        injector::WriteMemory(pattern.get(3).get<uint32_t>(13), &fRenderWidth_99999, true);
        injector::WriteMemory(pattern.get(11).get<uint32_t>(13), &fRenderWidth_99999, true);
    }

    pattern = GetPattern("F3 0F 11 86 8C 05 00 00 F3 0F 11 86 90 05 00 00");
    if (pattern.size() == 8)
    {
        struct HUDRenderHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esi + 0x58C) = -99999.0f;
            }
        };
        injector::MakeInline<HUDRenderHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(8)); // 781C7D
        injector::MakeInline<HUDRenderHook>(pattern.get(6).get<uint32_t>(0), pattern.get(6).get<uint32_t>(8)); // 7B1DD5
    }

    // GPS map background: the full width of the screen
    pattern = GetPattern("E8 ? ? ? ? D9 5C 24 0C E8 ? ? ? ? D9 5C 24 10 6A 00");
    if (!pattern.empty())
        injector::MakeCALL(pattern.get_first(0), FullWidth::LoadPositive, true); // 830301

    pattern = GetPattern("D9 EE D9 54 24 04 D9 1C 24 E8");
    if (!pattern.empty())
    {
        // fldz; fst [esp+4] becomes fld -99999; fst [esp+4]
        FullWidth::GPSMapExit = reinterpret_cast<uintptr_t>(pattern.get_first(6));
        injector::MakeJMP(pattern.get_first(0), FullWidth::GPSMapCave, true); // 83037F
        injector::MakeNOP(pattern.get_first(5), 1, true);
    }

    // Transition overlay: the full width of the screen
    pattern = GetPattern("E8 ? ? ? ? D9 5C 24 14 E8 ? ? ? ? D9 5C 24 1C");
    if (pattern.empty())
        pattern = GetPattern("E8 ? ? ? ? ? ? ? ? E8 ? ? ? ? ? ? ? ? 0F B6 44 24");
    if (!pattern.empty())
        injector::MakeCALL(pattern.get_first(0), FullWidth::LoadPositive, true); // 561A00

    pattern = hook::pattern("D9 05 ? ? ? ? D9 54 24 04 D9 1C 24 E8 ? ? ? ? 8B 0D");
    if (!pattern.empty())
        injector::WriteMemory(pattern.get_first(2), &FullWidth::fNegative, true);

    // FOV, HUD and movies are worked out for 16:9 and corrected for the screen
    static float fVertFOV = 1.777777777f;
    uint32_t* dword_76B7FF = find_pattern("74 ? 6A ? B9 ? ? ? ? E8 ? ? ? ? F3 0F 2A C0", "74 ? 6A ? B9 ? ? ? ? E8 ? ? ? ? F3 0F 2A C0").get_first<uint32_t>();
    injector::WriteMemory<uint8_t>(dword_76B7FF, 0xEB, true); // jmp
    uint32_t* dword_74A81D = find_pattern("D9 C9 DF F1 DD D8 76 ? F2 0F 10 0D", "D9 C9 DF F1 DD D8 76 ? F3 0F 10 0D").count(1).get(0).get<uint32_t>(6);
    injector::MakeNOP(dword_74A81D, 2, true);
    auto dword_76B84C = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 11 04 24 D9 04 24 83 C4 08");
    if (!dword_76B84C.empty())
        injector::WriteMemory(dword_76B84C.count(1).get(0).get<uint32_t>(4), &fVertFOV, true);
    else
    {
        dword_76B84C = hook::pattern("C3 F3 0F 10 05 ? ? ? ? ? ? ? ? ? ? ? ? 59 C3");
        injector::WriteMemory(dword_76B84C.count(3).get(2).get<uint32_t>(5), &fVertFOV, true);
    }

    // Mathematically correct horizontal FOV
    if (nScaling)
    {
        auto pattern = hook::pattern("F2 0F 59 0D ? ? ? ? 66 0F 5A C1 F3 0F 11 44 24");
        if (!pattern.empty())
        {
            static const double Corrected_Hor = 1.3;
            injector::WriteMemory(pattern.get_first(4), &Corrected_Hor, true);
        }
        else
        {
            static const float Corrected_Hor = 1.3f;
            pattern = hook::pattern("F3 0F 59 0D ? ? ? ? F3 0F 59 05 ? ? ? ? F3 0F 11 4C 24");
            if (!pattern.empty())
                injector::WriteMemory(pattern.get_first(4), &Corrected_Hor, true);
        }
    }

    // FOV
    pattern = GetPattern("F3 0F 10 44 24 38 F3 0F 10 4C 24 28"); //0x74A959
    if (!pattern.empty())
    {
        struct FOVHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x38) *= fHudScale;
                regs.xmm0.f32[0] = *(float*)(regs.esp + 0x38);
            }
        }; injector::MakeInline<FOVHook>(pattern.get_first(0), pattern.get_first(6));
    }
    else
    {
        pattern = GetPattern("F3 0F 10 44 24 ? F3 0F 5E 44 24 ? F3 0F 11 44 24");
        struct FOVHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x28) *= fHudScale;
                regs.xmm0.f32[0] = *(float*)(regs.esp + 0x28);
            }
        }; injector::MakeInline<FOVHook>(pattern.get_first(0), pattern.get_first(6));
    }

    // Field of View option. DrivingCameraMover::Update, the camera mover of the driving views (chase, hood, bumper and
    // look back), writes the camera's field of view (bAngle at +0x1C4, 65536 = 360 degrees) from its smoothed value at
    // the end of each update, in two places. The extra field of view goes on top of it there, so it applies at once and
    // the front end's, garage's and cutscenes' camera movers keep theirs.
    FieldOfView::pFovFactor = &FixSettings().Float("MAIN.FOVFactor");
    pattern = hook::pattern("75 ? 8B 86 2C 01 00 00 8B 56 28 F3 0F 2C 08 66 89 8A C4 01 00 00");
    static std::vector<SafetyHookMid> DrivingFOVHooks;
    NFSFieldOfView::InstallDrivingHooks(pattern, 15, 2, DrivingFOVHooks, +[](SafetyHookContext& regs)
    {
        regs.ecx = NFSFieldOfView::ScaleDrivingAngle(regs.ecx, *FieldOfView::pFovFactor, float(0x7000));
    });

    // Movies: the movie's widescreen check (before its whole-screen quad) and the quad function's draw call
    pattern = GetPattern("E8 ? ? ? ? 84 C0 74 0C 8B CE E8 ? ? ? ? E9");
    if (!pattern.empty())
    {
        auto quad = injector::GetBranchDestination(pattern.get_first(0xB)).as_int();
        auto draw = hook::range_pattern(quad, quad + 0x180, "E8 ? ? ? ? 83 C4 78 C3");
        if (!draw.empty())
        {
            Movie::pMode = &FixSettings().Int("MAIN.FMVWidescreenMode");
            injector::MakeCALL(pattern.get_first(0), Movie::IsWidescreen, true);
            static auto MovieQuadHook = safetyhook::create_mid(draw.get_first(), [](SafetyHookContext& regs)
            {
                // DrawPrimitiveUP's arguments: name, device, triangle fan, 2, vertices, stride
                if (Movie::bDrawing)
                {
                    Movie::bDrawing = false;
                    Movie::Scale(*reinterpret_cast<float**>(regs.esp + 0x10));

                    // The bars of the original aspect are black, not what was drawn before
                    if (*Movie::pMode == 0)
                    {
                        if (IDirect3DDevice9* device = Direct3DDevice)
                            device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
                    }
                }
            });
        }
    }

    // Cutscene borders: the letterbox channel's frame update writes the camera's letterbox height (fstp [ecx+1A8h],
    // the channel in edi), the view's projection setup takes the view (its camera at +0x40) and sets the projection's
    // horizontal and vertical scales (+0x40/+0x54 and the copies at +0xC0/+0xD4) before multiplying the matrices
    {
        auto iceManager = GetPattern("A1 ? ? ? ? 56 8B B0 90 00 00 00 85 F6 57 8B F9 74");
        auto letterbox = GetPattern("D9 99 A8 01 00 00 5F 5E C2 08 00");
        auto projection = GetPattern("8B 44 24 ? F3 0F 10 40 04 D9 00 F3 0F 10 48 08 F3 0F 10 50 0C 53");
        auto matrices = GetPattern("8D 9E 80 00 00 00 53 F3 0F 11 46 0C F3 0F 11 46 1C F3 0F 11 46 2C E8");
        if (!iceManager.empty() && !letterbox.empty() && !projection.empty() && !matrices.empty())
        {
            Cutscene::pBorders = &FixSettings().Int("MAIN.CutsceneBorders");
            Cutscene::pAnimated = &FixSettings().Int("MAIN.AnimatedCutsceneBorders");
            Cutscene::ICEManager = *iceManager.get_first<uintptr_t*>(1);

            static auto LetterBoxHook = safetyhook::create_mid(letterbox.get_first(6), [](SafetyHookContext& regs)
            {
                float& height = *reinterpret_cast<float*>(regs.ecx + 0x1A8);
                if (!*Cutscene::pAnimated && height > 0.0f)
                    height = *reinterpret_cast<float*>(regs.edi + 0x0C);   // the channel's full height
                Cutscene::fLetterBox = height;
                if (!(*Cutscene::pBorders & 1))
                    height = 0.0f;
            });

            // mov eax, [esp+n] reads the argument after the view
            static uint8_t ArgumentOffset = *projection.get_first<uint8_t>(3);
            static auto ProjectionHook = safetyhook::create_mid(projection.get_first(4), [](SafetyHookContext& regs)
            {
                auto view = *reinterpret_cast<uintptr_t*>(regs.esp + ArgumentOffset - 4);
                auto camera = Cutscene::Camera();
                Cutscene::bCameraView = view && camera && *reinterpret_cast<uintptr_t*>(view + 0x40) == camera;
            });

            static auto MatricesHook = safetyhook::create_mid(matrices.get_first(0x16), [](SafetyHookContext& regs)
            {
                if (!Cutscene::IsPlaying())
                {
                    Cutscene::fLetterBox = 0.0f;
                    return;
                }
                if (!Cutscene::bCameraView || (*Cutscene::pBorders & 1) || Cutscene::fLetterBox <= 0.0f || Cutscene::fLetterBox >= 0.5f)
                    return;
                float zoom = 1.0f / (1.0f - Cutscene::fLetterBox);
                for (auto offset : { 0x40, 0x54, 0xC0, 0xD4 })
                    *reinterpret_cast<float*>(regs.esi + offset) *= zoom;
            });

            WFP::onEndScene() += []()
            {
                if ((*Cutscene::pBorders & 2) && Cutscene::IsPlaying())
                {
                    if (IDirect3DDevice9* device = Direct3DDevice)
                        Cutscene::DrawPillarbox(device);
                }
            };
        }
    }

    // Cascade shadow maps: resolution and scale
    Shadows::iniShadowsRes = std::clamp(FixSettings().Int("GRAPHICS.ShadowResolution"), 256, 16384);
    static float CSMScale = FixSettings().Float("GRAPHICS.CSMScale");
    static float CSMScaleNear = FixSettings().Float("GRAPHICS.CSMScaleNear") * CSMScale;
    static float CSMScaleMid = FixSettings().Float("GRAPHICS.CSMScaleMid") * CSMScale;
    static float CSMScaleFar = FixSettings().Float("GRAPHICS.CSMScaleFar") * CSMScale;
    Shadows::UpdateShadowsRes();

    // Shadow texture creation: push height, width
    pattern = hook::pattern("51 6A 01 68 00 04 00 00 C1 E0 0A");
    if (!pattern.empty())
    {
        Shadows::TextureCaveExit = reinterpret_cast<uintptr_t>(pattern.get_first(3 + 9));
        injector::MakeJMP(pattern.get_first(3), Shadows::TextureCave, true);
    }

    // Shadow rendering viewports
    pattern = hook::pattern("8B 46 08 0F 57 C0 C1 E0 0A");
    if (!pattern.empty())
    {
        struct ShadowsResHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uint32_t*)(regs.esp + 4) = Shadows::ShadowsRes * regs.eax;
                *(uint32_t*)(regs.esp + 0xC) = Shadows::ShadowsRes;
                *(uint32_t*)(regs.esp + 0x10) = Shadows::ShadowsRes;
            }
        }; injector::MakeInline<ShadowsResHook2>(pattern.get_first(6), pattern.get_first(26));
    }

    // Cascade scales: the game's writes to CSM::fCSMScaleLevel[] are removed
    pattern = find_pattern("F3 0F 11 0D ? ? ? ? 0F 5A C8", "F3 0F 11 0D ? ? ? ? 56");
    auto pattern2 = find_pattern("F3 0F 11 0D ? ? ? ? F3 0F 11 05 ? ? ? ? E8 ? ? ? ? 8B C8", "F3 0F 11 0D ? ? ? ? F3 0F 11 05 ? ? ? ? E8 ? ? ? ? 8B C8");
    if (!pattern.empty() && !pattern2.empty())
    {
        static float* CSM_fCSMScaleLevel = *pattern.get_first<float*>(4);
        injector::MakeNOP(pattern.get_first(), 8);
        injector::MakeNOP(pattern2.get_first(0), 8);
        injector::MakeNOP(pattern2.get_first(8), 8);

        CSM_fCSMScaleLevel[0] = CSMScaleNear;
        CSM_fCSMScaleLevel[1] = CSMScaleMid;
        CSM_fCSMScaleLevel[2] = CSMScaleFar;
    }

    // Shadows of meshes outside the view
    pattern = hook::pattern("77 ? F3 0F 5A C0 0F 5A C9");
    if (!pattern.empty())
        injector::MakeJMP(pattern.get_first(), pattern.get_first(15), true);
    else
    {
        pattern = hook::pattern("77 ? 0F 2F CA 77 ? B0 01");
        if (!pattern.empty())
            injector::MakeJMP(pattern.get_first(), pattern.get_first(7), true);
    }

    // Shadow resolution during CSM::Update(), some distant shadows use it
    pattern = hook::pattern("68 00 04 00 00 68 00 04 00 00");
    if (pattern.size() == 1)
    {
        static auto CSMUpdateHook = [](uint32_t, uint32_t) -> uint32_t { return Shadows::ShadowsRes; };
        injector::MakeCALL(pattern.get_first(10), static_cast<uint32_t(__cdecl*)(uint32_t, uint32_t)>(CSMUpdateHook), true);
    }

    // Frame rate limit: -1 = monitor refresh rate, -2 = twice the refresh rate. The game needs a fixed rate, so it can't
    // be unlimited. The simulation runs at a whole multiple of it, at least 60 steps per second like the original (a
    // limit of 30 or 40 runs 2 steps per frame), so physics never step coarser than on the consoles.
    if (nFPSLimit == -2)
        nFPSLimit = GetDesktopRefreshRate() * 2;
    else if (nFPSLimit <= 0)
        nFPSLimit = GetDesktopRefreshRate();
    nFPSLimit = std::clamp(nFPSLimit, 30, 1000);
    SimRate = nFPSLimit * ((60 + nFPSLimit - 1) / nFPSLimit);
    FrameRate::fFrameTime = 1.0f / static_cast<float>(nFPSLimit);
    FrameRate::fStepRate = static_cast<float>(SimRate);
    FrameRate::fStepTime = 1.0f / FrameRate::fStepRate;
    FrameRate::dStepTime = 1.0 / static_cast<double>(SimRate);
    FrameRate::fCameraStep = FrameRate::fStepTime;
    FrameRate::dCameraStep = FrameRate::dStepTime;
    FrameRate::MaxSteps = std::clamp((8 * SimRate + 59) / 60, 8, 127);
    {
        // The 1/60 constants are shared with unrelated code (online, debug modes, physics substeps), so only the
        // instructions that make up the simulation step are pointed at the fix's own values.

        // Sim::QueueEvents: the step, and its rate (retail) or divisor (Steam)
        auto steam = hook::pattern("83 3D ? ? ? ? 00 74 39 0F 57 C0 F3 0F 10 4C 24 30 0F 2F C1 73 2B F3 0F 10 15 ? ? ? ? 0F 5A D2 0F 5A C9 F2 0F 58 D1 66 0F 5A CA F3 0F 10 15");
        auto retail = hook::pattern("83 3D ? ? ? ? 00 74 2F 0F 57 C9 F3 0F 10 54 24 30 0F 2F CA 73 21 F3 0F 10 05 ? ? ? ? F3 0F 58 C2 F3 0F 10 15");
        auto steamDivisor = hook::pattern("F2 0F 10 15 ? ? ? ? F3 0F 5A D9 F2 0F 5E DA F2 0F 2C C3 83 F8 01");
        auto retailRate = hook::pattern("0F 28 D8 F3 0F 59 1D ? ? ? ? F3 0F 2C C3 83 F8 01");
        // SimSystem::UpdateState: mTimeStep = mSpeed * 1/60 (double on Steam, float on retail), and the 1/60 it gives
        // the time manager that blends the speed (slow motion)
        auto steamUpdateState = hook::pattern("0F 5A C0 F2 0F 59 05 ? ? ? ? 66 0F 5A C0 5F F3 0F 11 46 14");
        auto retailUpdateState = hook::pattern("F3 0F 10 46 1C F3 0F 59 05 ? ? ? ? 83 7E 24 03 5F F3 0F 11 46 14");
        auto timeManager = hook::pattern("8B C8 D9 05 ? ? ? ? 8B 42 04 D9 1C 24 FF D0 D9 5E 1C");
        // SimSystem::SimSystem: the first mTimeStep
        auto constructor = hook::pattern("66 0F D6 46 58 F3 0F 10 05 ? ? ? ? 8B CD");
        // ESimTick: Frame.ElapsedReal += 1/60 per step; it's the time ESimFrameEnd gives the camera, the cutscenes
        // (ICE), the crowd and, in the front end, the rendering
        auto steamTick = hook::pattern("F3 0F 10 46 14 0F 5A C0 F2 0F 58 05 ? ? ? ? D9 5E 10");
        auto retailTick = hook::pattern("F3 0F 10 46 14 F3 0F 58 05 ? ? ? ? 83 C4 04 D9 5E 10");
        // Sim::QueueEvents: at most 8 steps per frame (cmp eax, 8 / add eax, -8 / mov eax, 8)
        auto steamCap = hook::pattern("83 F8 08 F3 0F 11 0D ? ? ? ? F3 0F 11 44 24 04 7E");
        auto retailCap = hook::pattern("83 F8 08 F3 0F 11 05 ? ? ? ? F3 0F 11 4C 24 04 7E");

        bool bSteam = !steam.empty() && !steamDivisor.empty() && !steamUpdateState.empty() && !steamTick.empty() && !steamCap.empty();
        bool bRetail = !retail.empty() && !retailRate.empty() && !retailUpdateState.empty() && !retailTick.empty() && !retailCap.empty();
        if ((bSteam || bRetail) && !timeManager.empty() && !constructor.empty())
        {
            auto& anchor = bSteam ? steam : retail;
            FrameRate::SimActive = *anchor.get_first<int32_t*>(2);
            FrameRate::TimeBank = *anchor.get_first<float*>(0x1B);
            static auto QueueEventsHook = safetyhook::create_mid(anchor.get_first(-0x23), [](SafetyHookContext& regs)
            {
                FrameRate::PrepareStep(*reinterpret_cast<float*>(regs.esp + 4));
            });

            if (bSteam)
            {
                injector::WriteMemory(steam.get_first(0x31), &FrameRate::fStepTime, true);
                injector::WriteMemory(steamDivisor.get_first(4), &FrameRate::dStepTime, true);
                injector::WriteMemory(steamUpdateState.get_first(7), &FrameRate::dStepTime, true);
                injector::WriteMemory(steamTick.get_first(12), &FrameRate::dStepTime, true);
            }
            else
            {
                injector::WriteMemory(retail.get_first(0x27), &FrameRate::fStepTime, true);
                injector::WriteMemory(retailRate.get_first(7), &FrameRate::fStepRate, true);
                injector::WriteMemory(retailUpdateState.get_first(9), &FrameRate::fStepTime, true);
                injector::WriteMemory(retailTick.get_first(9), &FrameRate::fStepTime, true);
            }
            injector::WriteMemory(timeManager.get_first(4), &FrameRate::fStepTime, true);
            injector::WriteMemory(constructor.get_first(9), &FrameRate::fStepTime, true);

            // The step cap keeps the same time as at 60 steps per second
            auto cap = (bSteam ? steamCap : retailCap).get_first<uint8_t>();
            size_t addOffset = 0x13, movOffset = bSteam ? 0x34 : 0x28;
            if (cap[addOffset] == 0x83 && cap[addOffset + 1] == 0xC0 && cap[addOffset + 2] == 0xF8 && cap[movOffset] == 0xB8 && *reinterpret_cast<int32_t*>(cap + movOffset + 1) == 8)
            {
                injector::WriteMemory<int8_t>(cap + 2, int8_t(FrameRate::MaxSteps), true);
                injector::WriteMemory<int8_t>(cap + addOffset + 2, int8_t(-FrameRate::MaxSteps), true);
                injector::WriteMemory<int32_t>(cap + movOffset + 1, FrameRate::MaxSteps, true);
            }
            else
                FrameRate::MaxSteps = 8;

            // The chase camera's springs (DC::Damp, DC::AngleDamp and the copies inlined into DrivingCameraMover): every
            // read of 1/60 in the camera code between the first and the last of them. IDA shows 11 reads of the double
            // and 1 of a float 1/60 on Steam and 9 of the float on retail, and no other code in between reads them.
            auto steamFirst = hook::pattern("F3 0F 10 44 24 18 F2 0F 10 1D ? ? ? ? 8B 44 24 14 0F 5A C0 F2 0F 5E");
            auto steamLast = hook::pattern("0F 8B ? ? ? ? F2 0F 10 25 ? ? ? ? 0F 5A C1 F2 0F 10 0D");
            auto steamFloat = hook::pattern("E8 ? ? ? ? D9 05 ? ? ? ? 51 D9 1C 24 8D 4C 24 14");
            auto retailFirst = hook::pattern("F3 0F 5E 4C 24 14 F3 0F 10 1D ? ? ? ? F3 0F 10 15");
            auto retailLast = hook::pattern("F3 0F 10 0D ? ? ? ? F3 0F 10 2D ? ? ? ? F3 0F 10 A6 5C 01 00 00");
            if (bSteam && steamFirst.size() == 1 && !steamLast.empty() && steamFloat.size() == 1)
            {
                auto begin = reinterpret_cast<uintptr_t>(steamFirst.get_first());
                auto end = reinterpret_cast<uintptr_t>(steamLast.get(steamLast.size() - 1).get<void>(14));
                auto doubleConstant = *steamFirst.get_first<uintptr_t>(10);
                auto floatConstant = *steamFloat.get_first<uintptr_t>(7);
                FrameRate::RedirectConstant(begin, end, doubleConstant, &FrameRate::dCameraStep, 11);
                FrameRate::RedirectConstant(begin, end, floatConstant, &FrameRate::fCameraStep, 1);
            }
            else if (bRetail && retailFirst.size() == 1 && retailLast.size() == 1)
            {
                auto begin = reinterpret_cast<uintptr_t>(retailFirst.get_first());
                auto end = reinterpret_cast<uintptr_t>(retailLast.get_first(16));
                FrameRate::RedirectConstant(begin, end, *retailFirst.get_first<uintptr_t>(10), &FrameRate::fCameraStep, 9);
            }
        }

        // Animated textures: the real-time tick count (cvtsi2ss xmm1, [count]) after the world's
        auto textures = hook::pattern("83 EC 14 83 3D ? ? ? ? 03");
        if (textures.size() == 1)
        {
            auto load = hook::range_pattern(reinterpret_cast<uintptr_t>(textures.get_first()), reinterpret_cast<uintptr_t>(textures.get_first(0x40)), "F3 0F 2A 0D");
            if (!load.empty())
            {
                TextureAnimation::Update();
                injector::WriteMemory(load.get_first(4), &TextureAnimation::RealTicks, true);
                WFP::onGameProcessEvent() += []()
                {
                    TextureAnimation::Update();
                };
            }
        }
    }

    // Frames are paced to the limit before they're shown
    WFP::onEndScene() += []()
    {
        FrameRate::LimitFrameRate();
    };

    // Scenery options
    Scenery::pImproveLOD = &FixSettings().Int("GRAPHICS.ImproveSceneryLOD");
    Scenery::pDisablePreculler = &FixSettings().Int("GRAPHICS.DisablePreculler");
    Scenery::pBruteforceCulling = &FixSettings().Int("GRAPHICS.BruteforceCulling");
    Scenery::Apply();
    WFP::onGameProcessEvent() += []()
    {
        Scenery::Apply();
    };

    if (bDisableMotionBlur)
    {
        auto pattern = hook::pattern("0F 85 ? ? ? ? 6A ? B9 ? ? ? ? E8 ? ? ? ? 85 C0");
        if (!pattern.empty())
            injector::WriteMemory<uint16_t>(pattern.get_first(0), 0xE990, true); // jnz -> jmp
    }

    // Online services are gone; the game doesn't try to log in
    {
        auto pattern = hook::pattern("E8 ? ? ? ? 84 C0 74 ? 39 5C 24 ? 75 ? 8B 0D");
        if (!pattern.empty())
        {
            static auto OnlineLoginEnabled = []() -> bool { return false; };
            static auto shOnlineLogin = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), static_cast<bool(*)()>(OnlineLoginEnabled));
        }
    }
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            auto ModuleStart = (uintptr_t)GetModuleHandle(NULL);
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init1, hook::range_pattern(ModuleStart, RangeEnd, "C7 44 24 ? ? ? ? ? FF 15 ? ? ? ? 8D 54 24 0C 52"));
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init2, hook::range_pattern(ModuleStart, RangeEnd, "E8 ? ? ? ? 8B 4E 04 83 C4 20 89 46 08 8B 15 ? ? ? ? 51 52 68 0B 20 00 00"));
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init4, hook::range_pattern(ModuleStart, RangeEnd, "8B 0D ? ? ? ? 6A 01 51 8B C8"));
        };
    }
} Misc;
