module;

#include <stdafx.h>
#include "common.h"
#include "TextOverlay.hpp"
#include "PromptSpans.hpp"

export module ButtonIcons;

import NativeMenuABI;


import Skeleton;
import Draw;
import Renderer;
import Controller;

export namespace MHIcons
{
    SafetyHookInline characterHook, commandHook, lookupHook, footerHook, controlsHook, wrapHook;
    std::map<std::wstring, std::wstring> prompts;
    std::map<wchar_t, std::wstring> originals;
    std::map<std::wstring, std::wstring> hints;
    thread_local int controlsDrawing = 0;
    const wchar_t* Label(uint32_t button);
    GameRef<PrintState>* print = nullptr;
    GameRef<int>* font = nullptr;
    GameRef<float>* heights = nullptr;
    GameRef<int>* measuring = nullptr;
    void* dictionary = nullptr;
    void* raster = nullptr;
    DWORD lastAttempt = 0;
    void* (__cdecl* load)(const char*) = nullptr;
    void* (__cdecl* find)(void*, const char*) = nullptr;
    int (__cdecl* begin)(void*) = nullptr;
    int (__cdecl* flush)() = nullptr;

    bool Ready()
    {
        if (raster)
            return true;
        if (!load || !find || !GetCurrentRaster())
            return false;
        const DWORD now = GetTickCount();
        if (lastAttempt && now - lastAttempt < 1000)
            return false;
        lastAttempt = now;
        const auto path = GetThisModulePath() / L"Manhunt.FusionFix" / L"buttons.txd";
        if (!std::filesystem::is_regular_file(path))
            return false;
        if (!dictionary)
            dictionary = load(path.string().c_str());
        if (dictionary)
            if (auto texture = find(dictionary, "buttons"))
                raster = *static_cast<void**>(texture);
        return raster != nullptr;
    }

    int Cell(uint32_t button)
    {
        switch (button)
        {
            case MHPad::A: return 0;
            case MHPad::B: return 1;
            case MHPad::X: return 2;
            case MHPad::Y: return 3;
            case MHPad::LB: return 4;
            case MHPad::RB: return 5;
            case MHPad::LT: return 6;
            case MHPad::RT: return 7;
            case MHPad::LS: return 8;
            case MHPad::RS: return 9;
            case MHPad::Back: return 10;
            case MHPad::Start: return 11;
            case MHPad::Up: return 12;
            case MHPad::Down: return 13;
            default: return -1;
        }
    }

    uint64_t Hash(std::wstring_view text)
    {
        uint64_t value = 14695981039346656037ULL;
        for (const auto character : text)
            for (const uint8_t byte : { uint8_t(character), uint8_t(character >> 8) })
                value = (value ^ byte) * 1099511628211ULL;
        return value;
    }

    const wchar_t* Transform(const wchar_t* text)
    {
        if (!text)
            return text;
        const size_t length = wcsnlen(text, 4096);
        if (length == 4096)
            return text;
        const std::wstring_view source(text, length);
        const uint64_t hash = Hash(source);
        auto first = std::lower_bound(std::begin(MHPrompts::spans), std::end(MHPrompts::spans), hash,
            [](const auto& span, uint64_t value) { return span.hash < value; });
        if (first == std::end(MHPrompts::spans) || first->hash != hash)
            return text;
        if (auto found = prompts.find(std::wstring(source)); found != prompts.end())
            return found->second.c_str();
        if (prompts.size() >= 512)
            return text;
        std::wstring transformed;
        size_t position = 0;
        for (auto span = first; span != std::end(MHPrompts::spans) && span->hash == hash; ++span)
        {
            if (span->offset < position || size_t(span->offset) + span->length > source.size())
                return text;
            const wchar_t marker = wchar_t(0xE200 + (span - std::begin(MHPrompts::spans)));
            transformed.append(source.substr(position, span->offset - position));
            transformed += marker;
            originals.emplace(marker, source.substr(span->offset, span->length));
            position = span->offset + span->length;
        }
        transformed.append(source.substr(position));
        return prompts.emplace(source, std::move(transformed)).first->second.c_str();
    }

    const wchar_t* __fastcall Lookup(void* self, void*, const char* key)
    {
        const auto text = lookupHook.unsafe_thiscall<const wchar_t*>(self, key);
        if (controlsDrawing && text && key && MHPad::lastPad && MHPad::modern && *MHPad::modern && hints.size() < 64)
        {
            const uint32_t button = std::strcmp(key, "CRMOK") == 0 ? MHPad::A : std::strcmp(key, "CRMCANC") == 0 ? MHPad::B : 0;
            if (button)
            {
                std::wstring label(text);
                label += L" ";
                const auto icon = Label(button);
                label += icon ? icon : MHPad::Label(button);
                return hints.emplace(label, label).first->second.c_str();
            }
        }
        return Transform(text);
    }

    int __cdecl Controls()
    {
        struct Scope
        {
            Scope() { ++controlsDrawing; }
            ~Scope() { --controlsDrawing; }
        } scope;
        return controlsHook.unsafe_ccall<int>();
    }

    const wchar_t* LabelCell(int cell)
    {
        if (cell < 0 || cell >= 27 || !Ready())
            return nullptr;
        static const auto labels = []()
        {
            std::array<std::array<wchar_t, 2>, 27> values{};
            for (size_t i = 0; i < values.size(); ++i)
                values[i][0] = wchar_t(0xE100 + i);
            return values;
        }();
        return labels[cell].data();
    }

    const wchar_t* Label(uint32_t button)
    {
        return LabelCell(Cell(button));
    }

    const wchar_t* MovementLabel()
    {
        return LabelCell(19);
    }

    using OverlayRows = std::array<wchar_t, 1600>;
    struct OverlayText
    {
        std::wstring source;
        OverlayRows rows;
        bool pad;
    };
    std::map<void*, OverlayText> overlays;

    bool PadText()
    {
        return MHPad::lastPad && MHPad::modern && *MHPad::modern && Ready();
    }

    int __fastcall Wrap(void* self, void*, const wchar_t* text)
    {
        overlays.erase(self);
        const int result = wrapHook.unsafe_thiscall<int>(self, text);
        if (result && text && overlays.size() < 32)
        {
            const size_t length = wcsnlen(text, 4096);
            const std::wstring_view source(text, length);
            if (length < 4096 && std::any_of(source.begin(), source.end(), [](wchar_t c)
                { return c >= 0xE200 && c < 0xE200 + std::size(MHPrompts::spans); }))
            {
                const auto rows = reinterpret_cast<const OverlayRows*>(static_cast<char*>(self) + 140);
                overlays.emplace(self, OverlayText{ std::wstring(source), *rows, PadText() });
            }
        }
        return result;
    }

    void Overlay(void* self)
    {
        if (auto found = overlays.find(self); found != overlays.end())
        {
            auto data = static_cast<char*>(self);
            const auto rows = reinterpret_cast<const OverlayRows*>(data + 140);
            if (!*reinterpret_cast<const int*>(data + 6544) || *rows != found->second.rows)
                overlays.erase(found);
            else if (const bool pad = PadText(); pad != found->second.pad)
            {
                // Ordinary tutorial overlays show all rows at once. Rewrap without restarting their timer or sound.
                if (!*reinterpret_cast<const int*>(data + 6548) && !*reinterpret_cast<const int*>(data + 6552))
                {
                    const auto savedPrint = print->get();
                    const int savedFont = font->get();
                    if (wrapHook.unsafe_thiscall<int>(self, found->second.source.c_str()))
                    {
                        injector::WriteMemory<OverlayRows>(reinterpret_cast<OverlayRows*>(data + 3340), *rows, true);
                        found->second.rows = *rows;
                        found->second.pad = pad;
                    }
                    injector::WriteMemory<PrintState>(print->get_ptr(), savedPrint, true);
                    injector::WriteMemory<int>(font->get_ptr(), savedFont, true);
                }
            }
        }
    }

    void Draw(int cell, float width, float height)
    {
        auto get = RwEngine::Field<int(__cdecl*)(int, void*)>(RwEngine::RenderStateGet);
        auto set = RwEngine::Field<int(__cdecl*)(int, void*)>(RwEngine::RenderStateSet);
        void* saved = nullptr;
        if (!get(1, &saved))
            return;
        flush();
        set(1, raster);
        auto target = GetCurrentRaster();
        const float w = float(RwRasterGetWidth(target)), h = float(RwRasterGetHeight(target));
        const int selected = MHPad::family ? *MHPad::family : 0;
        const int row = selected == 2 ? 1 : selected == 3 ? 2 : 0;
        const float u = float(cell % 8) / 8.0f, v = float(row * 4 + cell / 8) / 16.0f;
        const float x = print->get().x * w, y = print->get().y * h;
        RwIm2DVertex vertices[] =
        {
            { x, y, 0.0f, 1.0f, 0xFFFFFFFF, u, v },
            { x + width * w, y, 0.0f, 1.0f, 0xFFFFFFFF, u + 0.125f, v },
            { x + width * w, y + height * h, 0.0f, 1.0f, 0xFFFFFFFF, u + 0.125f, v + 0.0625f },
            { x, y + height * h, 0.0f, 1.0f, 0xFFFFFFFF, u, v + 0.0625f }
        };
        for (size_t i = 0; i < std::size(vertices); ++i)
            vertices[i].emissiveColor = 0x00FFFFFF | (uint32_t(std::clamp(print->get().colors[i][3], 0, 255)) << 24);
        uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };
        RwEngine::Field<int(__cdecl*)(int, RwIm2DVertex*, int, uint16_t*, int)>(RwEngine::Im2DRenderIndexedPrimitive)(
            3, vertices, 4, indices, 6);
        set(1, saved);
        begin(saved);
    }

    int __cdecl Character(const wchar_t* text, int measure)
    {
        int cell = *text - 0xE100;
        if (*text >= 0xE200 && *text < 0xE200 + std::size(MHPrompts::spans))
        {
            const auto action = MHPrompts::spans[*text - 0xE200].action;
            cell = action == 14 ? 19 : action == 38 ? 24 : action == 39 ? 16 : Cell(MHPad::ActionButton(action));
            if (!MHPad::lastPad || !MHPad::modern || !*MHPad::modern || cell < 0 || !Ready())
            {
                if (auto found = originals.find(*text); found != originals.end())
                    for (const auto& character : found->second)
                        characterHook.unsafe_ccall<int>(&character, measure);
                return 1;
            }
        }
        if (cell < 0 || cell >= 27 || !Ready() || font->get() < 0 || font->get() > 3)
            return characterHook.unsafe_ccall<int>(text, measure);
        const float width = heights->get_ptr()[font->get()] * print->get().scaleX * 0.75f;
        const float height = heights->get_ptr()[font->get()] * print->get().scaleY;
        if (measure == 1)
            injector::WriteMemory<float>(&print->get().width, print->get().width + width, true);
        else
        {
            if (!measuring->get())
                Draw(cell, width, height);
            injector::WriteMemory<float>(&print->get().x, print->get().x + width, true);
        }
        return 1;
    }

    int __cdecl Command(const wchar_t* text, int* position, int length, int measure)
    {
        if (MHPad::lastPad && MHPad::modern && *MHPad::modern)
        {
            const std::wstring_view remaining(text + *position, std::max(0, length - *position));
            for (const auto& [tag, button] : std::array<std::pair<std::wstring_view, uint32_t>, 4>{
                { { L"~cross~", MHPad::A }, { L"~triangle~", MHPad::B }, { L"~square~", MHPad::X }, { L"~circle~", MHPad::B } } })
            {
                if (remaining.starts_with(tag))
                    if (const auto label = Label(button))
                    {
                        Character(label, measure);
                        *position += int(tag.size());
                        return 1;
                    }
            }
        }
        return commandHook.unsafe_ccall<int>(text, position, length, measure);
    }

    int __cdecl Footer(const wchar_t* first, const wchar_t* second, const wchar_t* third, const wchar_t* fourth)
    {
        std::array<const wchar_t*, 4> arguments{ first, second, third, fourth };
        std::array<std::wstring, 4> labels;
        if (MHPad::lastPad && MHPad::modern && *MHPad::modern)
        {
            for (size_t i = 0; i < arguments.size(); ++i)
            {
                if (!arguments[i])
                    continue;
                for (const auto& [key, button] : std::array<std::pair<const char*, uint32_t>, 3>{
                    { { "IBACK", MHPad::B }, { "ICANCEL", MHPad::B }, { "IOK", MHPad::A } } })
                {
                    if (std::wcscmp(arguments[i], MHMenu::ABI::Text(key)) == 0)
                    {
                        labels[i] = arguments[i];
                        labels[i] += L" ";
                        const auto icon = Label(button);
                        labels[i] += icon ? icon : MHPad::Label(button);
                        arguments[i] = labels[i].c_str();
                        break;
                    }
                }
            }
        }
        return footerHook.unsafe_ccall<int>(arguments[0], arguments[1], arguments[2], arguments[3]);
    }

    void Install()
    {
        auto character = hook::pattern("53 83 EC 04 8B 44 24 0C 8B 1D ? ? ? ? 0F B7 08 3B 0C 9D");
        auto state = hook::pattern("8B 44 24 08 8B 54 24 0C 8B 4C 24 10 A3 ? ? ? ? 8B 44 24 14 89 15");
        auto height = hook::pattern("D9 04 9D ? ? ? ? D8 0D ? ? ? ? 83 3D ? ? ? ? 00 DD D9");
        auto loader = hook::pattern("FF 74 24 04 E8 ? ? ? ? 59 C3 90 90 90 90 90 8B 54 24 04 31 C0 85 D2");
        auto finder = hook::pattern("8B 44 24 04 53 83 C0 08 55 56 57 8B 18 89 44 24 14 3B D8");
        auto batchBegin = hook::pattern("57 E8 ? ? ? ? 59 56 55 E8 ? ? ? ? 59 59 E8 ? ? ? ? EB");
        auto batchEnd = hook::pattern("53 8B 1D ? ? ? ? 85 DB 74 2A 8D 0C 9D 00 00 00 00 89 DA");
        auto command = hook::pattern("53 56 57 55 83 EC 28 31 ED 8D 5C 24 20 C7 03 00 00 00 00 8B 44 24 40");
        auto lookup = hook::pattern("FF 74 24 04 E8 ? ? ? ? C2 04 00 90 90 90 90 8B 44 24 08 0F B7 10");
        auto footer = hook::pattern("8B 0D ? ? ? ? 53 56 55 83 EC 18 8B 74 24 28 8B 6C 24 30 8B 5C 24 34 89 0D");
        auto controls = hook::pattern("D9 05 ? ? ? ? D8 35 ? ? ? ? 53 56 55 83 EC 14 B9 ? ? ? ? BD ? ? ? ? 8B 1D");
        auto wrap = hook::pattern("53 56 57 55 8B 5C 24 14 81 EC 40 01 00 00 85 DB 89 CD 74 06 66 83 3B 00 75 16");
        if (!MHPad::nameHook)
            return;
        print = BindGameRef(printState, *state.get_first<PrintState*>(13));
        font = BindGameRef(printFont, *character.get_first<int*>(10));
        heights = BindGameRef(printHeights, *height.get_first<float*>(3));
        measuring = BindGameRef(printMeasuring, *height.get_first<int*>(15));
        load = reinterpret_cast<decltype(load)>(loader.get_first());
        find = reinterpret_cast<decltype(find)>(finder.get_first());
        begin = reinterpret_cast<decltype(begin)>(injector::GetBranchDestination(batchBegin.get_first(1)).as_int());
        flush = reinterpret_cast<decltype(flush)>(batchEnd.get_first());
        characterHook = safetyhook::create_inline(character.get_first(), Character);
        commandHook = safetyhook::create_inline(command.get_first(), Command);
        lookupHook = safetyhook::create_inline(lookup.get_first(), Lookup);
        footerHook = safetyhook::create_inline(footer.get_first(), Footer);
        controlsHook = safetyhook::create_inline(controls.get_first(), Controls);
        wrapHook = safetyhook::create_inline(wrap.get_first(), Wrap);
        MHTextOverlay::beforeRender = Overlay;
        MHPad::iconLabel = Label;
        MHPad::movementLabel = MovementLabel;
    }
}
