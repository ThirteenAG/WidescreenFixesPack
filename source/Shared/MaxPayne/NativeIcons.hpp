#pragma once
#include <array>
#include <map>
#include <string>
#include <vector>
#include <functional>
#include <cctype>
#include <cmath>
#include <cstring>
#include <utility>
#include <wincodec.h>
#include "GamepadButtons.hpp"
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

namespace MaxPayneIcons
{
    constexpr unsigned First = 0x80;
    constexpr unsigned Count = 27;
    inline HMODULE Host = nullptr;
    inline std::function<int()> Family;
    inline std::function<bool()> PadInUse;
    inline int BuildingFamily = 0;
    inline bool BuiltWithIcons = false;
    struct FontLayout
    {
        ptrdiff_t alphaOffset = 0x21;
        ptrdiff_t alphaFlag = 0x42;
        size_t fontSize = 0x2450;
        const wchar_t* crt = L"msvcrt.dll";
    };
    inline FontLayout Layout;

    struct Atlas
    {
        UINT width = 0, height = 0;
        std::vector<uint8_t> pixels;
    };
    inline std::array<Atlas, 3> Atlases;
    inline int AtlasState = -1;

    inline const char* Glyph(unsigned icon)
    {
        static const auto strings = []()
        {
            std::array<std::array<char, 2>, Count> value{};
            for (unsigned i = 0; i < Count; ++i)
                value[i][0] = char(First + i);
            return value;
        }();
        return icon < Count ? strings[icon].data() : nullptr;
    }

    inline const char* Button(int button)
    {
        static constexpr unsigned Icons[] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9, 12, 13, 14, 15 };
        return button >= 0 && button < int(std::size(Icons)) ? Glyph(Icons[button]) : nullptr;
    }

    inline int CurrentFamily()
    {
        int value = Family ? Family() : MaxPayneGamepadButtons::Auto;
        return std::clamp(value == MaxPayneGamepadButtons::Auto ? MaxPayneGamepadButtons::DetectFamily() : value, 1, 3);
    }

    inline bool LoadAtlases()
    {
        if (AtlasState >= 0)
            return AtlasState != 0;
        AtlasState = 0;
        HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        IWICImagingFactory* factory = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
        {
            const wchar_t* names[] = { L"MP_PAD_XBOX", L"MP_PAD_PLAYSTATION", L"MP_PAD_NINTENDO" };
            bool all = true;
            for (size_t i = 0; i < Atlases.size(); ++i)
            {
                auto resource = FindResourceW(Host, names[i], MAKEINTRESOURCEW(10));
                auto size = resource ? SizeofResource(Host, resource) : 0;
                auto bytes = resource ? static_cast<BYTE*>(LockResource(LoadResource(Host, resource))) : nullptr;
                IWICStream* stream = nullptr;
                IWICBitmapDecoder* decoder = nullptr;
                IWICBitmapFrameDecode* frame = nullptr;
                IWICFormatConverter* converter = nullptr;
                auto& atlas = Atlases[i];
                bool ok = bytes && size && SUCCEEDED(factory->CreateStream(&stream)) &&
                          SUCCEEDED(stream->InitializeFromMemory(bytes, size)) &&
                          SUCCEEDED(factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnLoad,
                                                                     &decoder)) &&
                          SUCCEEDED(decoder->GetFrame(0, &frame)) &&
                          SUCCEEDED(frame->GetSize(&atlas.width, &atlas.height)) && atlas.width == 512 &&
                          atlas.height == 256 && SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
                          SUCCEEDED(converter->Initialize(frame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone,
                                                          nullptr, 0, WICBitmapPaletteTypeCustom));
                if (ok)
                {
                    atlas.pixels.resize(size_t(atlas.width) * atlas.height * 4);
                    ok = SUCCEEDED(converter->CopyPixels(nullptr, atlas.width * 4, UINT(atlas.pixels.size()),
                                                         atlas.pixels.data()));
                }
                all &= ok;
                if (converter)
                    converter->Release();
                if (frame)
                    frame->Release();
                if (decoder)
                    decoder->Release();
                if (stream)
                    stream->Release();
            }
            AtlasState = all ? 1 : 0;
            factory->Release();
        }
        if (SUCCEEDED(initialized))
            CoUninitialize();
        return AtlasState != 0;
    }

    struct BitmapSwap
    {
        ProtectedGameRef<uint8_t*> data;
        ProtectedGameRef<int> width;
        uint8_t* original = nullptr;
        int originalWidth = 0;
        std::vector<uint8_t> pixels;
        bool installed = false;

        void Install(uint8_t* image, std::vector<uint8_t> value, int newWidth)
        {
            data.SetAddress(reinterpret_cast<uint8_t**>(image + 5));
            width.SetAddress(reinterpret_cast<int*>(image + 0x19));
            original = data.get();
            originalWidth = width.get();
            pixels = std::move(value);
            data = pixels.data();
            width = newWidth;
            installed = true;
        }

        ~BitmapSwap()
        {
            if (installed)
            {
                data = original;
                width = originalWidth;
            }
        }
    };

    inline bool Repack(uint8_t* font, BitmapSwap& color, BitmapSwap& alpha)
    {
        auto source = *reinterpret_cast<const uint8_t**>(font + 5);
        int width = *reinterpret_cast<int*>(font + 0x19);
        int height = *reinterpret_cast<int*>(font + 0x1D);
        bool hasAlpha = font[Layout.alphaFlag] != 0;
        auto mask = hasAlpha ? *reinterpret_cast<const uint8_t**>(font + Layout.alphaOffset + 5) : nullptr;
        if (!source || !mask || width <= 0 || width > 32768 || height < 5 || height > 256 ||
            *reinterpret_cast<int*>(font + Layout.alphaOffset + 0x19) != width ||
            *reinterpret_cast<int*>(font + Layout.alphaOffset + 0x1D) != height || !LoadAtlases())
            return false;
        std::vector<std::pair<int, int>> cells;
        for (int x = 0; x < width && cells.size() < 256;)
        {
            auto nonblack = [&](int at)
            {
                return source[at * 4] || source[at * 4 + 1] || source[at * 4 + 2];
            };
            while (x < width && !nonblack(x))
                ++x;
            int begin = x;
            while (x < width && nonblack(x))
                ++x;
            if (begin < width)
                cells.emplace_back(begin, std::min(width - begin, x - begin + 1));
        }
        if (cells.size() != 256)
            return false;
        int size = height - 2;
        int resultWidth = 0;
        for (size_t i = 0; i < cells.size(); ++i)
            resultWidth += i >= First && i < First + Count ? size + 2 : cells[i].second;
        std::vector<uint8_t> colors(size_t(resultWidth) * height * 4, 0);
        std::vector<uint8_t> masks(colors.size(), 0);
        auto& atlas = Atlases[BuildingFamily - 1];
        int dest = 0;
        for (unsigned i = 0; i < cells.size(); ++i)
        {
            bool icon = i >= First && i < First + Count;
            int cellWidth = icon ? size + 2 : cells[i].second;
            if (!icon)
            {
                for (int y = 0; y < height; ++y)
                {
                    std::memcpy(colors.data() + (size_t(y) * resultWidth + dest) * 4,
                                source + (size_t(y) * width + cells[i].first) * 4, size_t(cellWidth) * 4);
                    std::memcpy(masks.data() + (size_t(y) * resultWidth + dest) * 4,
                                mask + (size_t(y) * width + cells[i].first) * 4, size_t(cellWidth) * 4);
                }
            }
            else
            {
                for (int x = 0; x < cellWidth - 1; ++x)
                {
                    for (int channel = 0; channel < 3; ++channel)
                        colors[(dest + x) * 4 + channel] = masks[(dest + x) * 4 + channel] = 255;
                }
                unsigned cell = i - First;
                int cellX = int(cell % 8) * 64, cellY = int(cell / 8) * 64;
                for (int y = 0; y < size; ++y)
                {
                    for (int x = 0; x < size; ++x)
                    {
                        float sx = (x + 0.5f) * 64.0f / size - 0.5f;
                        float sy = (y + 0.5f) * 64.0f / size - 0.5f;
                        int x0 = std::clamp(int(std::floor(sx)), 0, 63), x1 = std::min(x0 + 1, 63);
                        int y0 = std::clamp(int(std::floor(sy)), 0, 63), y1 = std::min(y0 + 1, 63);
                        float fx = std::clamp(sx - x0, 0.0f, 1.0f), fy = std::clamp(sy - y0, 0.0f, 1.0f);
                        auto target = (size_t(y + 2) * resultWidth + dest + x) * 4;
                        float rgba[4]{};
                        for (int channel = 0; channel < 4; ++channel)
                        {
                            auto pixel = [&](int px, int py)
                            {
                                auto offset = (size_t(cellY + py) * atlas.width + cellX + px) * 4;
                                float value = atlas.pixels[offset + channel];
                                return channel == 3 ? value : value * atlas.pixels[offset + 3] / 255.0f;
                            };
                            float top = pixel(x0, y0) * (1.0f - fx) + pixel(x1, y0) * fx;
                            float bottom = pixel(x0, y1) * (1.0f - fx) + pixel(x1, y1) * fx;
                            rgba[channel] = top * (1.0f - fy) + bottom * fy;
                        }
                        for (int channel = 0; channel < 3; ++channel)
                        {
                            colors[target + channel] =
                                rgba[3] > 0
                                    ? uint8_t(std::clamp(std::lround(rgba[channel] * 255.0f / rgba[3]), 0L, 255L))
                                    : 0;
                            masks[target + channel] = uint8_t(std::clamp(std::lround(rgba[3]), 0L, 255L));
                        }
                        colors[target + 3] = masks[target + 3] = 255;
                    }
                }
            }
            dest += cellWidth;
        }
        color.Install(font, std::move(colors), resultWidth);
        alpha.Install(font + Layout.alphaOffset, std::move(masks), resultWidth);
        return true;
    }

    struct Font
    {
        std::string color, alpha;
        std::array<void*, 3> variants{};
        std::array<bool, 3> failed{};
    };
    inline std::map<void*, Font> Fonts;
    inline std::map<void*, void*> Bases;
    inline SafetyHookInline Constructor, ConstructorOne, Destructor, Validate;
    inline const char*(__fastcall* FullPath)(void*, void*, const char*) = nullptr;
    inline void** File = nullptr;
    inline void*(__cdecl* Allocate)(size_t) = nullptr;
    inline void(__cdecl* Free)(void*) = nullptr;
    inline int* NewCount = nullptr;
    inline int* DeleteCount = nullptr;

    inline std::string Path(const char* value)
    {
        if (!value || !*value)
            return {};
        auto path = *File ? FullPath(*File, nullptr, value) : value;
        return path ? path : "";
    }

    inline void* __fastcall Construct(void* self, void* edx, const char* color, const char* alpha)
    {
        std::string colorName = color ? color : "";
        std::string alphaName = alpha ? alpha : "";
        Font font{ colorName.empty() ? "" : Path(colorName.c_str()), alphaName.empty() ? "" : Path(alphaName.c_str()) };
        auto result = Constructor.unsafe_fastcall<void*>(self, edx, color ? colorName.c_str() : nullptr,
                                                         alpha ? alphaName.c_str() : nullptr);
        Fonts[self] = std::move(font);
        return result;
    }

    inline void* __fastcall ConstructOne(void* self, void* edx, const char* color)
    {
        std::string colorName = color ? color : "";
        Font font{ colorName.empty() ? "" : Path(colorName.c_str()), "" };
        auto result = ConstructorOne.unsafe_fastcall<void*>(self, edx, color ? colorName.c_str() : nullptr);
        Fonts[self] = std::move(font);
        return result;
    }

    inline void __fastcall ValidateFont(uint8_t* self, void* edx)
    {
        BitmapSwap color, alpha;
        if (BuildingFamily)
            BuiltWithIcons = Repack(self, color, alpha);
        Validate.unsafe_fastcall(self, edx);
    }

    inline void CountAllocation(int* address)
    {
        ProtectedGameRef<int> count;
        count.SetAddress(address);
        count = count.get() + 1;
    }

    inline void __fastcall Destroy(void* self, void* edx)
    {
        auto found = Fonts.find(self);
        if (found != Fonts.end())
        {
            for (auto variant : found->second.variants)
            {
                if (!variant)
                    continue;
                Bases.erase(variant);
                Destructor.unsafe_fastcall(variant, nullptr);
                Free(variant);
                CountAllocation(DeleteCount);
            }
            Fonts.erase(found);
        }
        Destructor.unsafe_fastcall(self, edx);
    }

    inline void* Variant(void* source, int family)
    {
        if (auto base = Bases.find(source); base != Bases.end())
            source = base->second;
        auto found = Fonts.find(source);
        if (found == Fonts.end() || found->second.alpha.empty() || !LoadAtlases())
            return nullptr;
        auto& font = found->second;
        auto& variant = font.variants[family - 1];
        if (font.failed[family - 1])
            return nullptr;
        if (!variant)
        {
            auto memory = Allocate(Layout.fontSize);
            if (!memory)
                return nullptr;
            BuildingFamily = family;
            BuiltWithIcons = false;
            variant = Constructor.unsafe_fastcall<void*>(memory, nullptr, font.color.c_str(), font.alpha.c_str());
            BuildingFamily = 0;
            CountAllocation(NewCount);
            if (!BuiltWithIcons)
            {
                Destructor.unsafe_fastcall(variant, nullptr);
                Free(variant);
                CountAllocation(DeleteCount);
                variant = nullptr;
                font.failed[family - 1] = true;
                OutputDebugStringA("Max Payne: controller icon font could not be built; using button names.\n");
                return nullptr;
            }
            Bases[variant] = source;
        }
        return variant;
    }

    struct Prompt
    {
        std::string keyboard, pad;
    };
    inline std::map<std::string, Prompt> Prompts;

    inline void RegisterPrompt(const std::string& actual, Prompt prompt)
    {
        if (prompt.keyboard == prompt.pad)
            return;
        Prompts[prompt.keyboard] = prompt;
        Prompts[prompt.pad] = prompt;
        Prompts[actual] = std::move(prompt);
    }

    inline Prompt Alternatives(const std::string& text)
    {
        auto found = Prompts.find(text);
        return found == Prompts.end() ? Prompt{ text, text } : found->second;
    }

    inline void Replace(std::string& text, const std::string& token, const std::string& value)
    {
        if (token.empty())
            return;
        for (size_t at = text.find(token); at != std::string::npos; at = text.find(token, at + value.size()))
            text.replace(at, token.size(), value);
    }

    inline Prompt Hardcoded(const std::string& text)
    {
        Prompt prompt{ text, text };
        struct Key
        {
            const char* name;
            unsigned icon;
        };
        static constexpr Key keys[] = {
            { "F1", 13 }, { "F2", 12 }, { "F5", 5 }, { "F9", 5 },
            { "Esc", 11 }, { "ESC", 11 }, { "Escape", 11 }, { "ESCAPE", 11 },
        };
        auto letter = [](unsigned char c)
        {
            return std::isalnum(c) || c == '_';
        };
        for (auto& key : keys)
        {
            auto glyph = Glyph(key.icon);
            if (!glyph)
                continue;
            size_t length = std::strlen(key.name);
            for (size_t at = prompt.pad.find(key.name); at != std::string::npos;)
            {
                if ((!at || !letter(uint8_t(prompt.pad[at - 1]))) &&
                    (at + length == prompt.pad.size() || !letter(uint8_t(prompt.pad[at + length]))))
                {
                    prompt.pad.replace(at, length, glyph);
                    ++at;
                }
                else
                    at += length;
                at = prompt.pad.find(key.name, at);
            }
        }
        Replace(prompt.pad, "Press Delete", std::string("Press ") + Glyph(2));
        Replace(prompt.pad, "press Delete", std::string("press ") + Glyph(2));
        Replace(prompt.pad, "mousewheel", Glyph(18));
        Replace(prompt.pad, "MouseWheel", Glyph(18));
        Replace(prompt.pad, "pressing 1-6", std::string("pressing ") + Glyph(18));
        Replace(prompt.pad, "pressing 6.", std::string("pressing ") + Glyph(18) + ".");
        return prompt;
    }

    inline std::string Names(std::string text, int family)
    {
        static constexpr int buttons[] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9, 12, 13, 14, 15 };
        for (size_t i = 0; i < text.size();)
        {
            unsigned icon = uint8_t(text[i]) - First;
            if (icon >= Count)
            {
                ++i;
                continue;
            }
            const char* name = icon < std::size(buttons) ? MaxPayneGamepadButtons::Name(buttons[icon], family)
                               : icon < 19               ? "D-pad"
                               : icon < 24               ? "Left Stick"
                                                         : "Right Stick";
            text.replace(i, 1, name);
            i += std::strlen(name);
        }
        return text;
    }

    struct Text
    {
        Prompt prompt;
        void* base = nullptr;
        void* appliedFont = nullptr;
        std::string shown;
    };
    inline std::map<void*, Text> Texts;
    inline std::map<void*, std::string> ControlTexts;
    inline SafetyHookInline Print, Execute, TextDestructor;
    inline void*(__fastcall* GetFont)(void*, void*) = nullptr;
    inline void(__fastcall* SetFont)(void*, void*, void*) = nullptr;
    inline const char*(__fastcall* GetString)(void*, void*) = nullptr;

    inline void Refresh(void* self, Text& text)
    {
        auto current = GetFont(self, nullptr);
        if (current != text.appliedFont)
        {
            auto base = Bases.find(current);
            text.base = base == Bases.end() ? current : base->second;
        }
        bool pad = PadInUse && PadInUse();
        int family = CurrentFamily();
        auto font = pad ? Variant(text.base, family) : text.base;
        auto shown = pad ? text.prompt.pad : text.prompt.keyboard;
        if (!font)
        {
            font = text.base;
            shown = Names(std::move(shown), family);
        }
        if (current != font)
            SetFont(self, nullptr, font);
        text.appliedFont = font;
        auto previous = GetString(self, nullptr);
        if (shown != text.shown || !previous || std::strcmp(previous, shown.c_str()) != 0)
            Print.unsafe_fastcall(self, nullptr, shown.c_str());
        text.shown = std::move(shown);
    }

    inline void __fastcall PrintText(void* self, void* edx, const char* value)
    {
        std::string source = value ? value : "";
        auto found = Prompts.find(source);
        Prompt prompt = found == Prompts.end() ? Hardcoded(source) : found->second;
        if (auto control = ControlTexts.find(self); control != ControlTexts.end())
        {
            if (auto previous = Texts.find(self); previous != Texts.end() && source == previous->second.shown)
                source = previous->second.prompt.keyboard;
            prompt = { source, control->second };
        }
        if (prompt.keyboard == prompt.pad)
        {
            Texts.erase(self);
            auto current = GetFont(self, nullptr);
            if (auto base = Bases.find(current); base != Bases.end())
                SetFont(self, nullptr, base->second);
            Print.unsafe_fastcall(self, edx, value);
            return;
        }
        auto& text = Texts[self];
        text.prompt = std::move(prompt);
        Refresh(self, text);
    }

    inline bool __fastcall ExecuteText(void* self, void* edx)
    {
        if (auto text = Texts.find(self); text != Texts.end())
            Refresh(self, text->second);
        return Execute.unsafe_fastcall<bool>(self, edx);
    }

    inline void __fastcall DestroyText(void* self, void* edx)
    {
        Texts.erase(self);
        ControlTexts.erase(self);
        TextDestructor.unsafe_fastcall(self, edx);
    }

    inline std::string Source(void* text)
    {
        auto found = Texts.find(text);
        if (found != Texts.end())
            return found->second.prompt.keyboard;
        auto value = GetString(text, nullptr);
        return value ? value : "";
    }

    inline SafetyHookInline ControlsUpdate;
    inline std::function<std::string(int)> ControlBinding;
    inline std::function<std::string(uint8_t*)> ControlBindingObject;
    inline ptrdiff_t ControlId = 12, ControlText = 34;

    inline void __fastcall UpdateControl(uint8_t* self, void* edx)
    {
        auto text = *reinterpret_cast<void**>(self + ControlText);
        auto pad = ControlBindingObject ? ControlBindingObject(self) : ControlBinding(*reinterpret_cast<int*>(self + ControlId));
        if (text && !pad.empty())
        {
            ControlTexts[text] = std::move(pad);
            auto previous = Texts.find(text);
            auto value = GetString(text, nullptr);
            auto source = previous == Texts.end() ? std::string(value ? value : "") : previous->second.prompt.keyboard;
            PrintText(text, nullptr, source.c_str());
        }
        ControlsUpdate.unsafe_fastcall(self, edx);
    }

    inline void HookControls(void** table, int update, ptrdiff_t id, ptrdiff_t text,
                             std::function<std::string(int)> binding)
    {
        ControlId = id;
        ControlText = text;
        ControlBinding = std::move(binding);
        ControlsUpdate = safetyhook::create_inline(table[update], UpdateControl);
    }

    inline void HookControlsByObject(void** table, int update, ptrdiff_t text,
                                     std::function<std::string(uint8_t*)> binding)
    {
        ControlText = text;
        ControlBindingObject = std::move(binding);
        ControlsUpdate = safetyhook::create_inline(table[update], UpdateControl);
    }

    inline void Init(HMODULE host, HMODULE e2mfc, HMODULE rlmfc, std::function<int()> family,
                     std::function<bool()> padInUse, FontLayout layout = {})
    {
        Host = host;
        Layout = layout;
        Family = std::move(family);
        PadInUse = std::move(padInUse);
        auto constructor = GetProcAddress(e2mfc, "??0P_Font@@QAE@PBD0@Z");
        auto one = GetProcAddress(e2mfc, "??0P_Font@@QAE@PBD@Z");
        auto destructor = GetProcAddress(e2mfc, "??1P_Font@@QAE@XZ");
        auto validate = GetProcAddress(e2mfc, "?validate@P_Font@@AAEXXZ");
        auto print = GetProcAddress(e2mfc, "?print@P_Text@@QAEXPBD@Z");
        auto execute = GetProcAddress(e2mfc, "?executeAlways@P_Text@@MAE_NXZ");
        auto destroyText = GetProcAddress(e2mfc, "??1P_Text@@UAE@XZ");
        GetFont = reinterpret_cast<decltype(GetFont)>(GetProcAddress(e2mfc, "?getFont@P_Text@@QBEPAVP_Font@@XZ"));
        SetFont = reinterpret_cast<decltype(SetFont)>(GetProcAddress(e2mfc, "?setFont@P_Text@@QAEXPAVP_Font@@@Z"));
        GetString = reinterpret_cast<decltype(GetString)>(GetProcAddress(e2mfc, "?getString@P_Text@@QBEPBDXZ"));
        FullPath = reinterpret_cast<decltype(FullPath)>(GetProcAddress(rlmfc, "?getFullPathName@R_File@@QAEPBDPBD@Z"));
        File = reinterpret_cast<void**>(GetProcAddress(rlmfc, "?File@@3PAVR_File@@A"));
        auto crt = GetModuleHandleW(Layout.crt);
        Allocate = reinterpret_cast<decltype(Allocate)>(GetProcAddress(crt, "malloc"));
        Free = reinterpret_cast<decltype(Free)>(GetProcAddress(crt, "free"));
        NewCount = reinterpret_cast<int*>(GetProcAddress(rlmfc, "?sm_iNewCount@R_Memory@@2HA"));
        DeleteCount = reinterpret_cast<int*>(GetProcAddress(rlmfc, "?sm_iDeleteCount@R_Memory@@2HA"));
        Constructor = safetyhook::create_inline(constructor, Construct);
        ConstructorOne = safetyhook::create_inline(one, ConstructOne);
        Destructor = safetyhook::create_inline(destructor, Destroy);
        Validate = safetyhook::create_inline(validate, ValidateFont);
        Print = safetyhook::create_inline(print, PrintText);
        Execute = safetyhook::create_inline(execute, ExecuteText);
        TextDestructor = safetyhook::create_inline(destroyText, DestroyText);
    }
} // namespace MaxPayneIcons
