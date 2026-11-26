#pragma once
#include <d3d9.h>
#include <atomic>
#include <cstdlib>
#include <mutex>
#include <string>
#include <unordered_map>
#include "Shared/AsiSettings/asi_settings.hpp"
#include "Shared/TonyHawk/PadIconArt.hpp"

// Gamepad support shared by THUG and THUG2 (DirectInput through the bundled Xidi): the last device used, the pad
// assigned with default bindings when none is chosen, per-action default keys, vibration on/off, camera speed and
// invert for the pad, and controller icons in the text prompts while the pad is in use.
namespace TonyPad
{
    struct Bindings
    {
        const char* name;
        // DirectInput
        const char* enumerate;          // device enumeration callback
        const char* selected;           // operand: player 1's chosen device GUID
        size_t selectedOffset;
        size_t playerStride;            // bytes from one player's controls to the next
        const char* pad;                // pad poll (player) -> state
        const char* padBuffer;          // operand: previous pad states, 272 bytes per player
        size_t padBufferOffset;
        const char* mouse;              // mouse poll () -> state
        const char* mouseBuffer;        // operand: previous mouse state, searched from the mouse poll
        size_t mouseBufferOffset;
        std::array<uint32_t, 24> defaultPad; // launcher-style bindings for the Xidi pad
        // Keyboard defaults
        const char* keyboardMap;        // operand: player 1's 24 keys
        size_t keyboardMapOffset;
        const char* readControls;       // the settings read; defaults are filled after it
        size_t readControlsOffset;
        std::array<int32_t, 24> defaultKeys;
        // Vibration
        const char* vibrate;            // (player, big motor, small motor)
        // Camera look: frame time call and right stick Y load
        const char* lookTime;
        const char* lookStick;
        // Prompts
        const char* meta;               // meta button -> glyph character
        const char* widthButton; size_t widthButtonOffset;
        const char* widthMeta; size_t widthMetaOffset;
        const char* drawButton; size_t drawButtonOffset;
        const char* drawMeta; size_t drawMetaOffset;
        const char* glyph;              // the draw loop selects the button font; operand at +2
        const char* texture; size_t textureOffset; // SetTexture call of the text batch (esi = font)
        size_t fontTexture;             // font -> IDirect3DTexture9*
        size_t elementId, elementParent, elementText;
        size_t rendererText;            // the text a draw call renders (a copy of the element's text)
    };

    struct Device
    {
        DWORD size;
        GUID instance, product;
        DWORD type;
    };

    inline Bindings Native{};
    inline std::atomic<bool> UsingPad{ false };
    inline std::atomic<int> ActivePlayer{ 0 };
    inline std::atomic<bool> VibrationEnabled{ true }, InvertCameraY{ false };
    inline std::atomic<float> CameraSpeed{ 1.0f };
    inline std::atomic<int> IconFamily{ 0 };

    inline GUID* Selected = nullptr;
    inline uint8_t* PreviousPad = nullptr;
    inline int32_t* PreviousMouse = nullptr;
    inline int32_t* KeyboardMap = nullptr;
    inline SafetyHookInline EnumerateHook, PadHook, MouseHook, VibrateHook, MetaHook;
    inline SafetyHookMid DefaultKeysHook, StickHook, StickRestoreHook, GlyphHook, TextureHook;
    inline bool StickInverted = false;
    inline float (__cdecl* FrameTime)() = nullptr;

    inline bool Focused()
    {
        DWORD process = 0;
        const auto window = GetForegroundWindow();
        return window && GetWindowThreadProcessId(window, &process) && process == GetCurrentProcessId();
    }

    // Last device used

    struct StickActivity
    {
        std::array<int32_t, 8> reference{};
        bool initialized = false;

        void Remember(const int32_t* axes)
        {
            std::copy_n(axes, reference.size(), reference.begin());
            initialized = true;
        }

        bool Moved(const int32_t* axes) const
        {
            for (size_t i = 0; i < reference.size(); ++i)
            {
                const int64_t value = axes[i];
                if (std::abs(value) > 200 && std::abs(value - reference[i]) > 200)
                    return true;
            }
            return false;
        }
    };

    inline uint8_t* __cdecl ReadPad(int player)
    {
        auto* current = PadHook.ccall<uint8_t*>(player);
        if (!current || player < 0 || player >= 2)
            return current;
        static std::array<StickActivity, 2> stickActivity;
        auto& activity = stickActivity[player];
        const auto* axes = reinterpret_cast<const int32_t*>(current);
        if (!Focused())
        {
            activity.Remember(axes);
            return current;
        }
        const auto* previous = PreviousPad + 272 * player;
        const auto* oldAxes = reinterpret_cast<const int32_t*>(previous);
        if (!activity.initialized)
            activity.Remember(oldAxes);

        // Accumulate motion until it can identify this pad. While it is active, keep the reference at its last sample.
        bool pressed = activity.Moved(axes);
        for (int i = 48; i < 176 && !pressed; ++i)
            pressed = (current[i] & 0x80) && !(previous[i] & 0x80);
        for (int i = 8; i < 12 && !pressed; ++i)
            pressed = axes[i] != -1 && axes[i] != oldAxes[i];
        if (pressed)
        {
            ActivePlayer.store(player, std::memory_order_relaxed);
            UsingPad.store(true, std::memory_order_relaxed);
        }
        if (pressed || (UsingPad.load(std::memory_order_relaxed) && ActivePlayer.load(std::memory_order_relaxed) == player))
            activity.Remember(axes);
        return current;
    }

    // Called after the game read the keyboard
    inline void KeysRead(const uint8_t* current, const uint8_t* previous)
    {
        if (!Focused())
            return;
        for (int i = 0; i < 256; ++i)
        {
            if ((current[i] & 0x80) && !(previous[i] & 0x80))
            {
                UsingPad.store(false, std::memory_order_relaxed);
                return;
            }
        }
    }

    inline int32_t Decay(int32_t value)
    {
        return value > 2 ? value - 2 : value < -2 ? value + 2 : 0;
    }

    inline int32_t* __cdecl ReadMouse()
    {
        auto* current = MouseHook.ccall<int32_t*>();
        if (!current || !Focused())
            return current;
        // The game eases the previous delta towards zero when no new movement came in
        const bool moved = (current[0] || current[1]) &&
            (current[0] != Decay(PreviousMouse[0]) || current[1] != Decay(PreviousMouse[1]));
        bool pressed = current[2] != 0;
        auto* buttons = reinterpret_cast<const uint8_t*>(current) + 12;
        auto* previous = reinterpret_cast<const uint8_t*>(PreviousMouse) + 12;
        for (int i = 0; i < 8 && !pressed; ++i)
            pressed = (buttons[i] & 0x80) && !(previous[i] & 0x80);
        if (moved || pressed)
            UsingPad.store(false, std::memory_order_relaxed);
        return current;
    }

    // The pad works without the launcher: the first Xidi pad gets player 1 with the default bindings while player 1
    // has no device chosen; a chosen device and its bindings stay as they are
    inline BOOL __stdcall Enumerate(const Device* device, void* context)
    {
        const auto& product = device->product;
        const bool xidi = (product.Data1 & 0xFFFFu) == 0xFFFFu && product.Data2 == 0 && product.Data3 == 0 &&
                          product.Data4[0] == 'X' && product.Data4[1] == 'I' && product.Data4[2] == 'D' &&
                          product.Data4[3] == 'I' && product.Data4[4] == 'V' && product.Data4[5] == 'C';
        constexpr GUID unassigned{ 0xFFFFFFFFu, 0xFFFFu, 0xFFFFu, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } };
        bool taken = false;
        for (size_t i = 0; i < 2; ++i)
            taken |= IsEqualGUID(*reinterpret_cast<const GUID*>(reinterpret_cast<uint8_t*>(Selected) + i * Native.playerStride), device->instance) != 0;
        if (xidi && !taken && (IsEqualGUID(*Selected, GUID_NULL) || IsEqualGUID(*Selected, unassigned)))
        {
            *Selected = device->instance;
            std::copy(Native.defaultPad.begin(), Native.defaultPad.end(), reinterpret_cast<uint32_t*>(Selected + 1));
        }
        return EnumerateHook.stdcall<BOOL>(device, context);
    }

    // Vibration

    inline void __cdecl Vibrate(int player, uint32_t strong, uint32_t weak)
    {
        if (!VibrationEnabled.load(std::memory_order_relaxed))
            strong = weak = 0;
        VibrateHook.ccall<void>(player, strong, weak);
    }

    // Stops a running effect once vibration is turned off; called on the game thread
    inline void Frame()
    {
        static bool applied = true;
        if (!VibrateHook)
            return;
        const bool enabled = VibrationEnabled.load(std::memory_order_relaxed);
        if (applied && !enabled)
        {
            for (int i = 0; i < 2; ++i)
                VibrateHook.ccall<void>(i, 0u, 0u);
        }
        applied = enabled;
    }

    // Camera look with the right stick: the stick sets the look angle the camera eases towards

    inline float __cdecl LookTime()
    {
        const float time = FrameTime();
        const float speed = CameraSpeed.load(std::memory_order_relaxed);
        if (!UsingPad.load(std::memory_order_relaxed) || !std::isfinite(speed))
            return time;
        return time * std::clamp(speed, 0.1f, 5.0f);
    }

    // Controller icons

    namespace Prompts
    {
        inline int (__cdecl* Number)(const char*) = nullptr;
        inline const char* MetaTable = nullptr;
        inline uint8_t** ButtonFont = nullptr;
        inline thread_local const char* CurrentString = nullptr;
        inline thread_local int Pending = -1, Active = -1;
        inline std::mutex TextMutex;
        inline std::unordered_map<std::string, bool> TextContexts;
        inline IDirect3DDevice9* AtlasDevice = nullptr;
        inline IDirect3DTexture9* Atlas[3][TonyHawkPadIcons::Count]{};

        constexpr uint32_t Checksum(const char* name)
        {
            uint32_t value = 0xFFFFFFFF;
            for (; *name; ++name)
            {
                value ^= static_cast<uint8_t>(*name);
                for (int bit = 0; bit < 8; ++bit)
                    value = (value >> 1) ^ ((value & 1) ? 0xEDB88320 : 0);
            }
            return value;
        }

        inline bool IsMenuHelper(uintptr_t element)
        {
            for (int depth = 0; element && depth < 32; ++depth)
            {
                const auto id = *reinterpret_cast<const uint32_t*>(element + Native.elementId);
                if (id == Checksum("helper_text_anchor") || id == Checksum("dialog_box_anchor"))
                    return true;
                element = *reinterpret_cast<const uintptr_t*>(element + Native.elementParent);
            }
            return false;
        }

        // Called before each text element's draw. The text is drawn later from a copy, so texts with arrows are
        // remembered by content: in menu helper lines the arrows mean the D-pad, elsewhere the left stick
        inline void SetTextContext(uintptr_t element)
        {
            if (!element || !UsingPad.load(std::memory_order_relaxed))
                return;
            const auto* text = *reinterpret_cast<const char* const*>(element + Native.elementText);
            bool directional = false;
            if (text)
            {
                for (const char* p = text; *p && !directional; ++p)
                    directional = p[0] == '\\' && (p[1] == 'b' || p[1] == 'B') && p[2] >= '4' && p[2] <= '7';
            }
            if (directional)
            {
                const bool helper = IsMenuHelper(element);
                std::lock_guard lock(TextMutex);
                if (TextContexts.size() >= 512)
                    TextContexts.clear();
                TextContexts[text] = helper;
            }
        }

        inline bool MenuHelper()
        {
            if (!CurrentString)
                return false;
            std::lock_guard lock(TextMutex);
            const auto it = TextContexts.find(CurrentString);
            return it != TextContexts.end() && it->second;
        }

        inline char __cdecl PreserveMeta(const char* text)
        {
            const int action = Number(text);
            return action >= 0 && action < 32 ? static_cast<char>(0x80 + action) : '0';
        }

        // Button glyph -> binding entry (keyboard and pad share the action order)
        inline int Action(int glyph)
        {
            switch (glyph)
            {
                case 0: return 11; case 1: return 10; case 2: return 9; case 3: return 8;
                case 4: return MenuHelper() ? 21 : 1;
                case 5: return MenuHelper() ? 23 : 3;
                case 6: return MenuHelper() ? 22 : 2;
                case 7: return MenuHelper() ? 20 : 0;
                case 8: return 12; case 9: return 13;
                case 14: return 16; case 15: return 17; case 16: return 14; case 17: return 15;
                case 18: return 19; case 19: return 18;
                case 22: return 8; case 23: return 9;
                // The PC's fixed menu keys (E, R, -, =, 1, 2) stand for these pad buttons
                case 24: return 10; case 25: return 11; case 26: return 14; case 27: return 15;
                case 28: return 16; case 29: return 17;
                default: return -1;
            }
        }

        inline const uint32_t* PadBindings()
        {
            const int player = std::clamp(ActivePlayer.load(std::memory_order_relaxed), 0, 1);
            return reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(Selected) + 16 + Native.playerStride * player);
        }

        inline int Icon(int action)
        {
            if (action < 0 || action >= 24)
                return -1;
            const uint32_t code = PadBindings()[action];
            // The D-pad as buttons 13-16 (up, down, left, right)
            if (code >= 23 && code <= 26)
                return 12 + static_cast<int>(code - 23);
            return TonyHawkPadIcons::Icon(code, action <= 7 || action >= 20, action % 2 == 0);
        }

        inline int Stick(int glyph)
        {
            const auto* bindings = PadBindings();
            const int first = glyph == 20 ? 0 : 4;
            const uint32_t a = bindings[first], b = bindings[first + 2];
            if ((a == 1 || a == 2) && (b == 1 || b == 2) && a != b) return 36;
            if ((a == 3 || a == 8) && (b == 3 || b == 8) && a != b) return 37;
            if ((a == 4 || a == 5) && (b == 4 || b == 5) && a != b) return 38;
            return -1;
        }

        inline int PadIcon(int glyph)
        {
            if (!UsingPad.load(std::memory_order_relaxed))
                return -1;
            if (glyph >= 10 && glyph <= 13)
                return TonyHawkPadIcons::Diagonal(PadBindings(), glyph - 10);
            if (glyph == 20 || glyph == 21)
                return Stick(glyph);
            return Icon(Action(glyph));
        }

        // The text code shows key names for most buttons and draws the arrow glyphs (4-7, 10-13) from the button
        // font; an icon is drawn in place of such a glyph
        inline int Resolve(const char* text, bool draw)
        {
            const auto marker = static_cast<uint8_t>(*text);
            const bool meta = marker >= 0x80 && marker < 0xa0;
            const char mapped = meta ? MetaTable[marker - 0x80] : *text;
            const int glyph = Number(&mapped);
            const int icon = PadIcon(glyph);
            if (draw)
                Pending = icon;
            if (icon < 0)
                return glyph;
            return (glyph >= 4 && glyph <= 7) || (glyph >= 10 && glyph <= 13) ? glyph : 4;
        }

        inline int __cdecl Width(const char* text)
        {
            CurrentString = nullptr;
            return Resolve(text, false);
        }

        inline int __cdecl DrawWithContext(const char* text, uintptr_t renderer)
        {
            CurrentString = *reinterpret_cast<const char* const*>(renderer + Native.rendererText);
            return Resolve(text, true);
        }

        // The draw loop's renderer is in edi
        __declspec(naked) inline void Draw()
        {
            __asm
            {
                push edi
                push dword ptr [esp + 8]
                call DrawWithContext
                add esp, 8
                ret
            }
        }

        inline IDirect3DTexture9* GetAtlas(uint8_t* font, int icon)
        {
            auto* native = *reinterpret_cast<IDirect3DTexture9**>(font + Native.fontTexture);
            if (!native)
                return nullptr;
            IDirect3DDevice9* device = nullptr;
            if (FAILED(native->GetDevice(&device)))
                return nullptr;
            if (device != AtlasDevice)
            {
                for (auto& family : Atlas)
                {
                    for (auto*& texture : family)
                    {
                        if (texture) { texture->Release(); texture = nullptr; }
                    }
                }
                if (AtlasDevice) AtlasDevice->Release();
                AtlasDevice = device;
            }
            else
                device->Release();
            const int family = std::clamp(IconFamily.load(std::memory_order_relaxed), 0, 3);
            const int index = family == 0 ? 0 : family - 1;
            auto*& result = Atlas[index][icon];
            if (result)
                return result;
            D3DSURFACE_DESC desc{};
            if (FAILED(native->GetLevelDesc(0, &desc)) || !desc.Width || !desc.Height)
                return nullptr;
            IDirect3DTexture9* texture = nullptr;
            if (FAILED(AtlasDevice->CreateTexture(desc.Width, desc.Height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture, nullptr)))
                return nullptr;
            D3DLOCKED_RECT lock{};
            if (FAILED(texture->LockRect(0, &lock, nullptr, 0)))
            {
                texture->Release();
                return nullptr;
            }
            for (UINT y = 0; y < desc.Height; ++y)
                std::memset(static_cast<uint8_t*>(lock.pBits) + y * lock.Pitch, 0, desc.Width * 4);
            // Every button glyph rectangle gets the icon: the batch draws only the one glyph
            const auto* glyphs = *reinterpret_cast<const uint8_t* const*>(font + 8);
            const auto* map = font + 268;
            for (int i = 0; i < 31; ++i)
            {
                const auto* uv = reinterpret_cast<const float*>(glyphs + 24 * map[i]);
                TonyHawkPadIcons::Rect r{ static_cast<int>(uv[0] * desc.Width), static_cast<int>(uv[1] * desc.Height),
                    static_cast<int>((uv[2] - uv[0]) * desc.Width + 0.5f), static_cast<int>((uv[3] - uv[1]) * desc.Height + 0.5f) };
                if (r.x >= 0 && r.y >= 0 && r.x + r.w <= static_cast<int>(desc.Width) && r.y + r.h <= static_cast<int>(desc.Height))
                    TonyHawkPadIcons::Paint(static_cast<uint32_t*>(lock.pBits), lock.Pitch / 4, r, icon, index + 1);
            }
            texture->UnlockRect(0);
            return result = texture;
        }

        inline void Install()
        {
            auto meta = hook::pattern(Native.meta);
            auto widthButton = hook::pattern(Native.widthButton);
            auto widthMeta = hook::pattern(Native.widthMeta);
            auto drawButton = hook::pattern(Native.drawButton);
            auto drawMeta = hook::pattern(Native.drawMeta);
            auto glyph = hook::pattern(Native.glyph);
            auto texture = hook::pattern(Native.texture);
            Number = reinterpret_cast<decltype(Number)>(injector::GetBranchDestination(meta.get_first(5)).as_int());
            MetaTable = *meta.get_first<const char*>(12);
            ButtonFont = *glyph.get_first<uint8_t**>(2);
            MetaHook = safetyhook::create_inline(meta.get_first(), PreserveMeta);
            injector::MakeCALL(widthButton.get_first(Native.widthButtonOffset), Width, true);
            injector::MakeCALL(widthMeta.get_first(Native.widthMetaOffset), Width, true);
            injector::MakeCALL(drawButton.get_first(Native.drawButtonOffset), Draw, true);
            injector::MakeCALL(drawMeta.get_first(Native.drawMetaOffset), Draw, true);
            GlyphHook = safetyhook::create_mid(glyph.get_first(), [](SafetyHookContext&) { Active = Pending; });
            TextureHook = safetyhook::create_mid(texture.get_first(Native.textureOffset), [](SafetyHookContext& regs)
            {
                if (Active < 0 || regs.esi != reinterpret_cast<uintptr_t>(*ButtonFont))
                    return;
                if (auto* atlas = GetAtlas(*ButtonFont, Active))
                    reinterpret_cast<uintptr_t*>(regs.esp)[1] = reinterpret_cast<uintptr_t>(atlas);
            });
        }
    }

    inline void Install(const Bindings& bindings, asi_settings::Plugin& settings)
    {
        Native = bindings;
        const auto refresh = [&settings]()
        {
            static std::mutex mutex;
            std::lock_guard lock(mutex);
            VibrationEnabled.store(settings.Get("MAIN.Vibration") != "0", std::memory_order_relaxed);
            InvertCameraY.store(settings.Get("MAIN.InvertGamepadY") == "1", std::memory_order_relaxed);
            const auto speed = settings.Get("MAIN.GamepadCameraSpeed");
            CameraSpeed.store(std::strtof(speed.c_str(), nullptr), std::memory_order_relaxed);
            IconFamily.store(std::atoi(settings.Get("MAIN.GamepadIcons").c_str()), std::memory_order_relaxed);
        };
        settings.OnChange("MAIN.Vibration", refresh);
        settings.OnChange("MAIN.InvertGamepadY", refresh);
        settings.OnChange("MAIN.GamepadCameraSpeed", refresh);
        settings.OnChange("MAIN.GamepadIcons", refresh);
        refresh();

        auto enumerate = hook::pattern(Native.enumerate);
        auto selected = hook::pattern(Native.selected);
        auto pad = hook::pattern(Native.pad);
        auto padBuffer = hook::pattern(Native.padBuffer);
        auto mouse = hook::pattern(Native.mouse);
        auto mouseBuffer = hook::range_pattern(reinterpret_cast<uintptr_t>(mouse.get_first()),
            reinterpret_cast<uintptr_t>(mouse.get_first(0x100)), Native.mouseBuffer);
        Selected = *selected.get_first<GUID*>(Native.selectedOffset);
        PreviousPad = *padBuffer.get_first<uint8_t*>(Native.padBufferOffset);
        PreviousMouse = *mouseBuffer.get_first<int32_t*>(Native.mouseBufferOffset);
        EnumerateHook = safetyhook::create_inline(enumerate.get_first(), Enumerate);
        PadHook = safetyhook::create_inline(pad.get_first(), ReadPad);
        MouseHook = safetyhook::create_inline(mouse.get_first(), ReadMouse);

        // Without the launcher's saved controls the keyboard has no bindings, and saved controls can leave actions
        // without a key: those get the launcher's default key while it is free
        auto keyboardMap = hook::pattern(Native.keyboardMap);
        auto readControls = hook::pattern(Native.readControls);
        KeyboardMap = *keyboardMap.get_first<int32_t*>(Native.keyboardMapOffset);
        DefaultKeysHook = safetyhook::create_mid(readControls.get_first(Native.readControlsOffset), [](SafetyHookContext&)
        {
            const auto bound = [](int32_t key) { return key > 0 && key < 256; };
            for (size_t i = 0; i < Native.defaultKeys.size(); ++i)
            {
                const int32_t key = Native.defaultKeys[i];
                if (key > 0 && !bound(KeyboardMap[i]) && std::find(KeyboardMap, KeyboardMap + 24, key) == KeyboardMap + 24)
                    KeyboardMap[i] = key;
            }
        });

        auto vibrate = hook::pattern(Native.vibrate);
        VibrateHook = safetyhook::create_inline(vibrate.get_first(), Vibrate);

        auto lookTime = hook::pattern(Native.lookTime);
        auto lookStick = hook::pattern(Native.lookStick);
        FrameTime = reinterpret_cast<decltype(FrameTime)>(injector::GetBranchDestination(lookTime.get_first()).as_int());
        injector::MakeCALL(lookTime.get_first(), LookTime, true);
        // The stick value is negated for the load only (edi = the pad's look input)
        StickHook = safetyhook::create_mid(lookStick.get_first(), [](SafetyHookContext& regs)
        {
            StickInverted = UsingPad.load(std::memory_order_relaxed) && InvertCameraY.load(std::memory_order_relaxed);
            if (StickInverted)
                *reinterpret_cast<float*>(regs.edi + 0x214) = -*reinterpret_cast<float*>(regs.edi + 0x214);
        });
        StickRestoreHook = safetyhook::create_mid(lookStick.get_first(6), [](SafetyHookContext& regs)
        {
            if (StickInverted)
                *reinterpret_cast<float*>(regs.edi + 0x214) = -*reinterpret_cast<float*>(regs.edi + 0x214);
            StickInverted = false;
        });

        Prompts::Install();
    }
}
