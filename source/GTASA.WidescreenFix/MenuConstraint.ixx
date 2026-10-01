module;

#include <stdafx.h>
#include "common.h"

export module MenuConstraint;

import Skeleton;
import Draw;
import Menu;
import Frontend;
import MenuMap;

namespace MenuConstraintHooks
{
    // Process includes tile streaming, input and the dialogs displayed by them.
    constexpr bool InputDrawsMenu = true;
    constexpr const char* TransitionSignature = nullptr;
    bool IsCurrentFrontend()
    {
        CIniReader reader("");
        return reader.ReadInteger("MAIN", "ScalingMode", 1) != 0;
    }
    // The 1.0 retail executable rewrites the following call during DRM setup.
    constexpr const char* DrawSignature = "56 8B F1 8A 46 32 84 C0 0F 85 ? ? ? ? 68 00 00 7F 43";
    constexpr const char* InputSignature = "56 8B F1 8A 46 5C 84 C0 74 2A 33 C0 8A 46 5B 50 E8";
    constexpr const char* MouseSignature = "80 BE 5D 01 00 00 26 C6 44 24 ? 00 75 ? 8A 86 EA 1A 00 00";
    void* MouseManager(SafetyHookContext& regs) { return reinterpret_cast<void*>(regs.esi); }
    constexpr const char* VideoSignature = "8B 44 24 04 50 E8 ? ? ? ? 83 C4 04 E8 ? ? ? ? A1 ? ? ? ? 8B 48 60 8B 51 0C";
    constexpr const char* CentreSignature = "83 EC 10 DB 05 ? ? ? ? D8 0D ? ? ? ? DB 05 ? ? ? ? D8 0D ? ? ? ? D9 5C 24 0C D8 1D";
    constexpr const char* FontSignature = "E8 ? ? ? ? 8B CE E8 ? ? ? ? A0 ? ? ? ? 84 C0 74 11 A1 ? ? ? ? A3 ? ? ? ? C6 05";
    constexpr const char* PreviewSignature = nullptr;
    constexpr void (*PreviewCallback)() = nullptr;
    constexpr std::array<size_t, 3> MouseFields{ offsetof(CMenuManager, m_nMousePosX), offsetof(CMenuManager, m_nMousePosWinX), 0x1AF8 };
    static_assert(MouseFields == std::array<size_t, 3>{ 0xBC, 0xE0, 0x1AF8 });
    uintptr_t FontAddress(uintptr_t address) { return injector::GetBranchDestination(address).as_int(); }
    bool UsesNativeCanvas(void* menu)
    {
        auto manager = static_cast<CMenuManager*>(menu);
        return manager->m_bMenuActive && !(bFullscreenMap && manager->GetCurrentScreen() == SCREEN_MAP);
    }
}

// No hooks are installed for the default Auto setting.
namespace MenuConstraintHooks
{
    struct Vertex
    {
        float x, y, z, rhw;
        uint32_t color;
        float u, v;
    };
    static_assert(sizeof(Vertex) == 28);

    SafetyHookInline Draw, Input, Transition, ChangeVideoMode, CentreMouse, Preview;
    SafetyHookInline Primitive, Indexed, Line, Triangle;
    SafetyHookMid Mouse;
    void (__cdecl* FlushFonts)() = nullptr;

    void RestoreMouseRange(SafetyHookContext& regs)
    {
        if (!MenuCanvas::Depth || MenuCanvas::Suspensions) return;
        auto menu = static_cast<uint8_t*>(MouseManager(regs));
        auto& x = *reinterpret_cast<int*>(menu + MouseFields[0]);
        const int raw = *reinterpret_cast<int*>(menu + MouseFields[1]);
        // Native input has just clamped x to the narrower menu. Recover the
        // unclamped position before hover, click and drag handling use it.
        x = MenuCanvas::GetMouseX(raw);
    }

    class MouseScope
    {
        void* menu;
        int offset;
    public:
        explicit MouseScope(void* manager) : menu(manager), offset(MenuCanvas::Depth ? int(MenuCanvas::Offset) : 0)
        {
            for (auto field : MouseFields)
                *reinterpret_cast<int*>(static_cast<uint8_t*>(menu) + field) -= offset;
        }
        ~MouseScope()
        {
            for (auto field : MouseFields)
                *reinterpret_cast<int*>(static_cast<uint8_t*>(menu) + field) += offset;
        }
    };

    void __fastcall DrawMenu(void* menu, void*)
    {
        FlushFonts();
        MenuCanvas::Scope canvas(true, UsesNativeCanvas(menu));
        MouseScope mouse(menu);
        Draw.unsafe_thiscall(menu);
        // Error/help text can be queued after the menu's own font flush.
        FlushFonts();
    }

    void __fastcall ProcessInput(void* menu, void*)
    {
        const bool nativeCanvas = UsesNativeCanvas(menu);
        const bool drawing = InputDrawsMenu && nativeCanvas;
        if (drawing) FlushFonts();
        MenuCanvas::Scope canvas(drawing, nativeCanvas);
        MouseScope mouse(menu);
        Input.unsafe_thiscall(menu);
        if (drawing) FlushFonts();
    }

    int __fastcall SwitchScreen(void* menu, void*, int screen)
    {
        FlushFonts();
        const bool translateMouse = MenuCanvas::Depth == 0;
        MenuCanvas::Scope canvas(true, UsesNativeCanvas(menu));
        std::optional<MouseScope> mouse;
        if (translateMouse) mouse.emplace(menu);
        // VC also captures transitions after load/save operations, outside
        // UserInput. Both captures must use the same canvas as the live menu.
        const int result = Transition.unsafe_thiscall<int>(menu, screen);
        FlushFonts();
        return result;
    }

    void __cdecl SetVideoMode(int mode)
    {
        MenuCanvas::Suspend physicalViewport;
        ChangeVideoMode.unsafe_ccall(mode);
    }

    void __cdecl CentreMousePointer()
    {
        MenuCanvas::Suspend physicalViewport;
        CentreMouse.unsafe_ccall();
    }

    std::vector<Vertex> Translate(Vertex* vertices, int count)
    {
        if (!MenuCanvas::Drawing || MenuCanvas::Suspensions || MenuCanvas::Offset == 0.0f || !vertices || count <= 0)
            return {};
        std::vector<Vertex> translated(vertices, vertices + count);
        // Native black clears still cover the physical background;
        // menu textures and map tiles keep their shape.
        const bool mask = count == 4 && std::ranges::all_of(translated, [](const Vertex& vertex) { return (vertex.color & 0x00FFFFFF) == 0; });
        const auto [top, bottom] = std::ranges::minmax_element(translated, {}, &Vertex::y);
        const bool fullHeight = top->y <= 0.5f && bottom->y >= RsGlobal->height - 0.5f;
        for (auto& vertex : translated)
        {
            if (mask && fullHeight)
            {
                if (vertex.x <= 0.5f) vertex.x -= MenuCanvas::Offset;
                else if (vertex.x >= RsGlobal->width - 0.5f) vertex.x += MenuCanvas::Offset;
            }
            vertex.x += MenuCanvas::Offset;
        }
        return translated;
    }

    int __cdecl RenderPrimitive(int type, Vertex* vertices, int count)
    {
        auto translated = Translate(vertices, count);
        return Primitive.unsafe_ccall<int>(type, translated.empty() ? vertices : translated.data(), count);
    }
    int __cdecl RenderIndexed(int type, Vertex* vertices, int count, uint16_t* indices, int indexCount)
    {
        auto translated = Translate(vertices, count);
        return Indexed.unsafe_ccall<int>(type, translated.empty() ? vertices : translated.data(), count, indices, indexCount);
    }
    int __cdecl RenderLine(Vertex* vertices, int count, int a, int b)
    {
        auto translated = Translate(vertices, count);
        return Line.unsafe_ccall<int>(translated.empty() ? vertices : translated.data(), count, a, b);
    }
    int __cdecl RenderTriangle(Vertex* vertices, int count, int a, int b, int c)
    {
        auto translated = Translate(vertices, count);
        return Triangle.unsafe_ccall<int>(translated.empty() ? vertices : translated.data(), count, a, b, c);
    }

    uintptr_t FindUnique(const char* signature)
    {
        auto pattern = hook::pattern(signature);
        return pattern.size() == 1 ? reinterpret_cast<uintptr_t>(pattern.get_first()) : 0;
    }

    bool Install()
    {
        const auto draw = FindUnique(DrawSignature);
        const auto input = FindUnique(InputSignature);
        const auto mouse = FindUnique(MouseSignature);
        const auto transition = TransitionSignature ? FindUnique(TransitionSignature) : 0;
        const auto video = FindUnique(VideoSignature);
        const auto font = FindUnique(FontSignature);
        const auto primitive = FindUnique("A1 ? ? ? ? FF 60 30 90 90 90 90 90 90 90 90");
        const auto centre = CentreSignature ? FindUnique(CentreSignature) : 0;
        const auto preview = PreviewSignature ? FindUnique(PreviewSignature) : 0;
        if (!draw || !input || !mouse || !video || !font || !primitive)
            return false;
        if (CentreSignature && !centre) return false;
        if (TransitionSignature && !transition) return false;
        if (PreviewSignature && !preview) return false;
        auto engine = **reinterpret_cast<uintptr_t***>(primitive + 1);
        if (!engine) return false;
        FlushFonts = reinterpret_cast<decltype(FlushFonts)>(FontAddress(font));
        if (!FlushFonts) return false;

        // Prepare every hook before enabling any of them. Unsupported/replaced
        // entry points leave the original menu and its input together.
        const auto flags = SafetyHookInline::StartDisabled;
        Draw = safetyhook::create_inline(draw, DrawMenu, flags);
        Input = safetyhook::create_inline(input, ProcessInput, flags);
        Mouse = safetyhook::create_mid(mouse, RestoreMouseRange, SafetyHookMid::StartDisabled);
        if (transition) Transition = safetyhook::create_inline(transition, SwitchScreen, flags);
        ChangeVideoMode = safetyhook::create_inline(video, SetVideoMode, flags);
        if (centre) CentreMouse = safetyhook::create_inline(centre, CentreMousePointer, flags);
        if (preview) Preview = safetyhook::create_inline(preview, PreviewCallback, flags);
        Line = safetyhook::create_inline(engine[0x28 / 4], RenderLine, flags);
        Triangle = safetyhook::create_inline(engine[0x2C / 4], RenderTriangle, flags);
        Primitive = safetyhook::create_inline(engine[0x30 / 4], RenderPrimitive, flags);
        Indexed = safetyhook::create_inline(engine[0x34 / 4], RenderIndexed, flags);
        std::vector hooks{ &Draw, &Input, &ChangeVideoMode, &Line, &Triangle, &Primitive, &Indexed };
        if (centre) hooks.push_back(&CentreMouse);
        if (transition) hooks.push_back(&Transition);
        if (preview) hooks.push_back(&Preview);
        if (!Mouse)
        {
            for (auto installed : hooks) installed->reset();
            return false;
        }
        for (auto hook : hooks)
        {
            if (!*hook || !hook->enable())
            {
                for (auto installed : hooks) installed->reset();
                Mouse.reset();
                return false;
            }
        }
        if (!Mouse.enable())
        {
            for (auto installed : hooks) installed->reset();
            Mouse.reset();
            return false;
        }
        return true;
    }
}

class MenuConstraint
{
public:
    MenuConstraint()
    {
        WFP::onGameInitEvent() += []()
        {
            if (!MenuConstraintHooks::IsCurrentFrontend()) return;
            CIniReader reader("");
            MenuCanvas::Constraint = ParseWidescreenHudOffset(reader.ReadString("MAIN", "MenuAspectRatioConstraint", "Auto"));
            if (!MenuCanvas::Constraint || !std::isfinite(*MenuCanvas::Constraint))
                return;
            MenuCanvas::Enabled = MenuConstraintHooks::Install();
            if (MenuCanvas::Enabled)
                onResChange().executeAll(RsGlobal->width, RsGlobal->height);
        };
    }
} MenuConstraint;
