module;

#include <stdafx.h>
#include "common.h"

export module MenuConstraint;

import Skeleton;
import Draw;
import Frontend;

namespace MenuConstraintHooks
{
    constexpr bool InputDrawsMenu = false;
    constexpr const char* TransitionSignature = nullptr;
    bool IsCurrentFrontend() { return true; }
    // The menu frame limiter replaces the first call with a mid-hook jump.
    constexpr const char* DrawSignature = "53 89 CB FF 35 ? ? ? ? ? ? ? ? ? 83 BB 48 05 00 00 00 59 75 22";
    constexpr const char* InputSignature = "53 56 57 55 83 EC 70 83 3D ? ? ? ? 00 89 CD 74 0E";
    constexpr const char* MouseSignature = "8B 85 48 05 00 00 83 F8 1E 74 ? 83 F8 36 74 ? 83 F8 37";
    void* MouseManager(SafetyHookContext& regs) { return reinterpret_cast<void*>(regs.ebp); }
    constexpr const char* VideoSignature = "6A 00 E8 ? ? ? ? 59 B9 ? ? ? ? E8 ? ? ? ? 8B 44 24 04 50 E8 ? ? ? ? 50 E8";
    constexpr const char* FontSignature = "A1 ? ? ? ? 50 E8 ? ? ? ? A1 ? ? ? ? 59 40 50 E8 ? ? ? ? A1 ? ? ? ? 59 83 C0 02";
    constexpr const char* CentreSignature = "D9 EE DB 05 ? ? ? ? 83 EC 18 89 4C 24 0C D8 0D ? ? ? ? DD D9 DB 05 ? ? ? ? D8 0D ? ? ? ?";
    constexpr const char* PreviewSignature = "53 56 57 83 EC 38 80 3D ? ? ? ? 00 75 11 C6 05 ? ? ? ? 01 C7 05 ? ? ? ? 00 00 00 00 BE";
    void __cdecl RenderPreview();
    constexpr auto PreviewCallback = RenderPreview;
    constexpr std::array<size_t, 3> MouseFields{ 0x118, 0x120, 0x53C };
    uintptr_t FontAddress(uintptr_t address) { return address; }
    bool UsesNativeCanvas(void*) { return true; }
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

void __cdecl MenuConstraintHooks::RenderPreview()
{
    // The model uses the physical 3D viewport, but its horizontal placement
    // follows the centered menu canvas. Restore only the renderer dimensions.
    const float x = playerSkinPos->x;
    MenuCanvas::Suspend physicalViewport;
    playerSkinPos->x = x;
    Preview.unsafe_ccall();
}
