module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/ButtonIconAtlas.hpp"
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

export module ButtonIcons;

import Pad;
import Frontend;
import Main;
import Skeleton;

// Button pictures in the game's prompts. The game writes a control's keys into a prompt where its text has
// ~k~~ACTION~ (CMessages::InsertPlayerControlKeysInString): there the pad's button (while the pad is used) or the
// key's picture (MAIN.KeyboardIcons) is written as one character of the Unicode private use area. The font measures
// it by the picture's width (the width table it reads is extended by these characters) and the text renderer draws
// the picture in its place. A shown help message is written again when the device or the icon style changes, so its
// pictures follow the last device used.
// With a controller mod loaded that draws its own button pictures, none of it is installed.

using namespace RWGTA::ButtonIcons;

namespace
{
    constexpr wchar_t PadFirst = 0xE000;
    constexpr wchar_t KeyFirst = 0xE100;
    constexpr int KeyCount = int(std::size(KeyPictures));
    constexpr wchar_t IconLast = KeyFirst + KeyCount - 1;

    bool IsIcon(wchar_t c)
    {
        return c >= PadFirst && c <= IconLast && (c < PadFirst + int(PadIcon::Count) || c >= KeyFirst);
    }

    float Aspect(wchar_t c)
    {
        return c >= KeyFirst ? KeyPictures[c - KeyFirst].width / 64.0f : 1.0f;
    }

    // The game's controls, by their index in CControllerConfigManager
    enum Action
    {
        PED_FIREWEAPON, PED_CYCLE_WEAPON_RIGHT, PED_CYCLE_WEAPON_LEFT, GO_FORWARD, GO_BACK, GO_LEFT, GO_RIGHT,
        PED_SNIPER_ZOOM_IN, PED_SNIPER_ZOOM_OUT, VEHICLE_ENTER_EXIT, CAMERA_CHANGE_VIEW_ALL_SITUATIONS, PED_JUMPING,
        PED_SPRINT, PED_LOOKBEHIND, PED_DUCK, PED_ANSWER_PHONE, VEHICLE_ACCELERATE, VEHICLE_BRAKE,
        VEHICLE_CHANGE_RADIO_STATION, VEHICLE_HORN, TOGGLE_SUBMISSIONS, VEHICLE_HANDBRAKE, PED_1RST_PERSON_LOOK_LEFT,
        PED_1RST_PERSON_LOOK_RIGHT, VEHICLE_LOOKLEFT, VEHICLE_LOOKRIGHT, VEHICLE_LOOKBEHIND, VEHICLE_TURRETLEFT,
        VEHICLE_TURRETRIGHT, VEHICLE_TURRETUP, VEHICLE_TURRETDOWN, PED_CYCLE_TARGET_LEFT, PED_CYCLE_TARGET_RIGHT,
        PED_CENTER_CAMERA_BEHIND_PLAYER, PED_LOCK_TARGET, NETWORK_TALK, PED_1RST_PERSON_LOOK_UP,
        PED_1RST_PERSON_LOOK_DOWN, ActionCount = 44
    };

    using P = PadIcon;
    constexpr P None = P::Count;

    struct PadPrompt
    {
        P first, second = None;     // second: pressed together (look behind with both triggers)
    };

    // The buttons of each control in the console layout (the game's first configuration) and in the modern one
    PadPrompt ClassicButton(int action)
    {
        switch (action)
        {
            case PED_FIREWEAPON: return { P::B };
            case PED_CYCLE_WEAPON_RIGHT: return { P::RT };
            case PED_CYCLE_WEAPON_LEFT: return { P::LT };
            case GO_FORWARD: case GO_BACK: return { P::LeftStickUpDown };
            case GO_LEFT: case GO_RIGHT: return { P::LeftStickLeftRight };
            case PED_SNIPER_ZOOM_IN: return { P::X };
            case PED_SNIPER_ZOOM_OUT: return { P::A };
            case VEHICLE_ENTER_EXIT: return { P::Y };
            case CAMERA_CHANGE_VIEW_ALL_SITUATIONS: return { P::Back };
            case PED_JUMPING: return { P::X };
            case PED_SPRINT: return { P::A };
            case PED_LOOKBEHIND: return { P::RS };
            case PED_DUCK: return { P::LS };
            case PED_ANSWER_PHONE: return { P::LB };
            case VEHICLE_ACCELERATE: return { P::A };
            case VEHICLE_BRAKE: return { P::X };
            case VEHICLE_CHANGE_RADIO_STATION: return { P::LB };
            case VEHICLE_HORN: return { P::LS };
            case TOGGLE_SUBMISSIONS: return { P::RS };
            case VEHICLE_HANDBRAKE: return { P::RB };
            case PED_1RST_PERSON_LOOK_LEFT: case PED_1RST_PERSON_LOOK_RIGHT: return { P::LeftStickLeftRight };
            case PED_1RST_PERSON_LOOK_UP: case PED_1RST_PERSON_LOOK_DOWN: return { P::LeftStickUpDown };
            case VEHICLE_LOOKLEFT: return { P::LT };
            case VEHICLE_LOOKRIGHT: return { P::RT };
            case VEHICLE_LOOKBEHIND: return { P::LT, P::RT };
            case VEHICLE_TURRETLEFT: case VEHICLE_TURRETRIGHT: return { P::RightStickLeftRight };
            case VEHICLE_TURRETUP: case VEHICLE_TURRETDOWN: return { P::RightStickUpDown };
            case PED_CYCLE_TARGET_LEFT: return { P::LT };
            case PED_CYCLE_TARGET_RIGHT: return { P::RT };
            case PED_CENTER_CAMERA_BEHIND_PLAYER: return { P::LB };
            case PED_LOCK_TARGET: return { P::RB };
            default: return { None };
        }
    }

    PadPrompt ModernButton(int action, Pad::Seat seat)
    {
        bool air = seat == Pad::Seat::Aircraft;
        switch (action)
        {
            case PED_FIREWEAPON: return { seat == Pad::Seat::Foot ? P::RT : P::A };
            case PED_CYCLE_WEAPON_RIGHT: return { P::LB };
            case PED_CYCLE_WEAPON_LEFT: return { P::DPadLeft };
            case GO_FORWARD: case GO_BACK: return { P::LeftStickUpDown };
            case GO_LEFT: case GO_RIGHT: return { P::LeftStickLeftRight };
            case PED_SNIPER_ZOOM_IN: return { P::LeftStickUp };
            case PED_SNIPER_ZOOM_OUT: return { P::LeftStickDown };
            case VEHICLE_ENTER_EXIT: return { P::Y };
            case CAMERA_CHANGE_VIEW_ALL_SITUATIONS: return { P::Back };
            case PED_JUMPING: return { P::X };
            case PED_SPRINT: return { P::A };
            case PED_LOOKBEHIND: return { P::RS };
            case PED_DUCK: return { P::LS };
            case PED_ANSWER_PHONE: return { P::RB };
            case VEHICLE_ACCELERATE: return { P::RT };
            case VEHICLE_BRAKE: return { P::LT };
            case VEHICLE_CHANGE_RADIO_STATION: return { P::DPadRight };
            case VEHICLE_HORN: return { P::LS };
            case TOGGLE_SUBMISSIONS: return { P::DPadUp };
            case VEHICLE_HANDBRAKE: return { air ? P::B : P::RB };
            case PED_1RST_PERSON_LOOK_LEFT: case PED_1RST_PERSON_LOOK_RIGHT: return { P::RightStickLeftRight };
            case PED_1RST_PERSON_LOOK_UP: case PED_1RST_PERSON_LOOK_DOWN: return { P::RightStickUpDown };
            case VEHICLE_LOOKLEFT: return { air ? P::LB : P::X };
            case VEHICLE_LOOKRIGHT: return { air ? P::RB : P::B };
            case VEHICLE_LOOKBEHIND: return { air ? None : P::RS };
            case VEHICLE_TURRETLEFT: case VEHICLE_TURRETRIGHT: return { P::RightStickLeftRight };
            case VEHICLE_TURRETUP: case VEHICLE_TURRETDOWN: return { P::RightStickUpDown };
            case PED_CYCLE_TARGET_LEFT: case PED_CYCLE_TARGET_RIGHT: return { P::RightStickLeftRight };
            case PED_CENTER_CAMERA_BEHIND_PLAYER: return { P::RB };
            case PED_LOCK_TARGET: return { P::LT };
            default: return { None };
        }
    }

    // CControllerConfigManager::m_aSettings: per control and device type (keyboard, second keyboard key, mouse,
    // joystick) the key and its order in the control's list
    struct ControlBinding
    {
        int32_t key;
        int32_t order;
    };
    constexpr ptrdiff_t SettingsOffset = 0xFF8;
    constexpr int32_t KeyNone = 1056;
    enum DeviceType { KEYBOARD, OPTIONAL_EXTRA, MOUSE, JOYSTICK };

    int FindKeyPicture(int code)
    {
        for (int i = 0; i < KeyCount; ++i)
            if (KeyPictures[i].code == code)
                return i;
        return -1;
    }

    // The control's first key or mouse button that has a picture
    int KeyPictureOf(uint8_t* manager, int action)
    {
        auto bindings = reinterpret_cast<ControlBinding*>(manager + SettingsOffset + action * 4 * sizeof(ControlBinding));
        int best = -1, bestOrder = INT_MAX;
        for (int type : { KEYBOARD, OPTIONAL_EXTRA, MOUSE })
        {
            auto& binding = bindings[type];
            bool bound = type == MOUSE ? binding.key != 0 : binding.key != KeyNone;
            if (!bound || binding.order <= 0 || binding.order >= bestOrder)
                continue;
            int code = binding.key;
            if (type != MOUSE && code >= 'a' && code <= 'z')
                code -= 'a' - 'A';
            int picture = FindKeyPicture(type == MOUSE ? 2000 + code : code);
            if (picture >= 0)
            {
                best = picture;
                bestOrder = binding.order;
            }
        }
        return best;
    }

    // The game's CControllerConfigManager::GetWideStringOfCommandKeys at its call in InsertPlayerControlKeysInString
    injector::hook_back<void(__fastcall*)(void*, void*, uint16_t, wchar_t*, uint16_t)> hbGetWideStringOfCommandKeys;
    void __fastcall GetWideStringOfCommandKeys(void* manager, void* edx, uint16_t action, wchar_t* text, uint16_t length)
    {
        if (length >= 4 && action < ActionCount)
        {
            if (Pad::IsPadLastDevice())
            {
                auto prompt = Pad::IsModernLayout() ? ModernButton(action, Pad::PlayerSeat()) : ClassicButton(action);
                if (prompt.first != None)
                {
                    int n = 0;
                    text[n++] = wchar_t(PadFirst + int(prompt.first));
                    if (prompt.second != None)
                    {
                        text[n++] = L'+';
                        text[n++] = wchar_t(PadFirst + int(prompt.second));
                    }
                    text[n] = 0;
                    return;
                }
            }
            else
            {
                static auto& keyIcons = FixSettings().Int("MAIN.KeyboardIcons");
                if (keyIcons)
                {
                    int picture = KeyPictureOf(static_cast<uint8_t*>(manager), action);
                    if (picture >= 0)
                    {
                        text[0] = wchar_t(KeyFirst + picture);
                        text[1] = 0;
                        return;
                    }
                }
            }
        }
        hbGetWideStringOfCommandKeys.fun(manager, edx, action, text, length);
    }

    // The atlases, as RenderWare textures made on first use
    struct RwFunctions
    {
        RwRaster* (__cdecl* RasterCreate)(int, int, int, int) = nullptr;
        uint8_t* (__cdecl* RasterLock)(RwRaster*, int, int) = nullptr;
        RwRaster* (__cdecl* RasterUnlock)(RwRaster*) = nullptr;
        RwTexture* (__cdecl* TextureCreate)(RwRaster*) = nullptr;
        int (__cdecl* RenderStateSet)(int, void*) = nullptr;
    } Rw;

    struct Atlas
    {
        const wchar_t* resource;
        int width, height;
        RwTexture* texture = nullptr;
        bool failed = false;
    };
    Atlas PadAtlases[] = { { L"BUTTONICONS_XBOX", 512, 256 }, { L"BUTTONICONS_PLAYSTATION", 512, 256 }, { L"BUTTONICONS_NINTENDO", 512, 256 } };
    Atlas KeyAtlas = { L"BUTTONICONS_KEYS", 1024, 1024 };

    // A PNG of the plugin's resources, as BGRA rows
    bool DecodePng(const wchar_t* name, int width, int height, std::vector<uint8_t>& pixels)
    {
        HMODULE self = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&DecodePng), &self);
        auto resource = FindResourceW(self, name, L"PNG");
        if (!resource)
            return false;
        auto data = LoadResource(self, resource);
        auto size = SizeofResource(self, resource);
        if (!data || !size)
            return false;

        bool comInitialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
        bool ok = false;
        IWICImagingFactory* factory = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
        {
            IWICStream* stream = nullptr;
            IWICBitmapDecoder* decoder = nullptr;
            IWICBitmapFrameDecode* frame = nullptr;
            IWICFormatConverter* converter = nullptr;
            UINT w = 0, h = 0;
            if (SUCCEEDED(factory->CreateStream(&stream)) &&
                SUCCEEDED(stream->InitializeFromMemory(static_cast<BYTE*>(LockResource(data)), size)) &&
                SUCCEEDED(factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand, &decoder)) &&
                SUCCEEDED(decoder->GetFrame(0, &frame)) &&
                SUCCEEDED(frame->GetSize(&w, &h)) && int(w) == width && int(h) == height &&
                SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
                SUCCEEDED(converter->Initialize(frame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
            {
                pixels.resize(size_t(width) * height * 4);
                ok = SUCCEEDED(converter->CopyPixels(nullptr, width * 4, UINT(pixels.size()), pixels.data()));
            }
            for (IUnknown* object : std::initializer_list<IUnknown*>{ converter, frame, decoder, stream, factory })
                if (object)
                    object->Release();
        }
        if (comInitialized)
            CoUninitialize();
        return ok;
    }

    RwTexture* TextureOf(Atlas& atlas)
    {
        if (atlas.texture || atlas.failed)
            return atlas.texture;
        atlas.failed = true;
        if (!Rw.RasterCreate)
            return nullptr;
        std::vector<uint8_t> pixels;
        if (!DecodePng(atlas.resource, atlas.width, atlas.height, pixels))
            return nullptr;
        constexpr int rwRASTERTYPETEXTURE = 0x04, rwRASTERFORMAT8888 = 0x0500, rwRASTERLOCKWRITE = 0x01;
        auto raster = Rw.RasterCreate(atlas.width, atlas.height, 0, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888);
        if (!raster)
            return nullptr;
        if (auto bits = Rw.RasterLock(raster, 0, rwRASTERLOCKWRITE))
        {
            std::memcpy(bits, pixels.data(), pixels.size());
            Rw.RasterUnlock(raster);
        }
        atlas.texture = Rw.TextureCreate(raster);
        if (atlas.texture)
        {
            // Bilinear filtering, clamped
            auto& flags = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(atlas.texture) + 80);
            flags = (flags & 0xFFFF0000u) | 2 | (3 << 8) | (3 << 12);
            atlas.failed = false;
        }
        return atlas.texture;
    }

    struct Picture
    {
        RwTexture* texture;
        float u0, v0, u1, v1;
    };

    std::optional<Picture> PictureOf(wchar_t c)
    {
        if (c >= KeyFirst)
        {
            auto& key = KeyPictures[c - KeyFirst];
            auto texture = TextureOf(KeyAtlas);
            if (!texture)
                return std::nullopt;
            float u = (key.cell % 8) * 128.0f / 1024.0f, v = (key.cell / 8) * 64.0f / 1024.0f;
            return Picture{ texture, u, v, u + key.width / 1024.0f, v + 64.0f / 1024.0f };
        }
        auto& atlas = PadAtlases[int(Pad::Style()) - 1];
        auto texture = TextureOf(atlas);
        if (!texture)
            return std::nullopt;
        int cell = c - PadFirst;
        float u = (cell % 8) * 64.0f / 512.0f, v = (cell / 8) * 64.0f / 256.0f;
        return Picture{ texture, u, v, u + 64.0f / 512.0f, v + 64.0f / 256.0f };
    }

    // The picture's size in the font: as tall as a line's characters (16 units of the height scale), its width in
    // the font's width units for the help box's proportions (1.1 high to 0.52 wide)
    constexpr float PictureHeight = 16.0f;
    constexpr float PictureGap = 1.5f;
    constexpr float HeightToWidth = 1.1f / 0.52f;

    int16_t WidthUnits(wchar_t c)
    {
        return int16_t(std::lround(PictureHeight * Aspect(c) * HeightToWidth + PictureGap * 2.0f));
    }

    // The font's width table (per style 210 characters) copied into one that also covers the pictures' characters,
    // so that every function measuring text measures them
    constexpr int FontStyles = 3;
    constexpr int WidthsPerStyle = 210;
    std::vector<int16_t> Widths;

    bool ExtendWidthTable(const int16_t* table, uint8_t* codeBegin, uint8_t* codeEnd)
    {
        constexpr size_t count = 0x10000 + FontStyles * WidthsPerStyle;
        Widths.assign(count, 0);
        std::copy_n(table, FontStyles * WidthsPerStyle, Widths.begin());
        for (int style = 0; style < FontStyles; ++style)
        {
            for (wchar_t c = PadFirst; c <= IconLast; ++c)
            {
                if (IsIcon(c))
                    Widths[style * WidthsPerStyle + (c - L' ')] = WidthUnits(c);
            }
        }

        // movsx reg, word ptr [table + index * 2] and the fixed-width column [table + 209 * 2 + index * 4]
        auto fixed = reinterpret_cast<const uint8_t*>(table) + (WidthsPerStyle - 1) * sizeof(int16_t);
        int count1 = 0, count2 = 0;
        for (auto p = codeBegin; p + 8 <= codeEnd; ++p)
        {
            if (p[0] != 0x0F || p[1] != 0xBF || (p[2] & 0xC7) != 0x04)
                continue;
            auto operand = reinterpret_cast<const void**>(p + 4);
            if (*operand == table && (p[3] >> 6) == 1)
            {
                injector::WriteMemory(operand, Widths.data(), true);
                ++count1;
            }
            else if (*operand == fixed && (p[3] >> 6) == 2)
            {
                injector::WriteMemory(operand, reinterpret_cast<uint8_t*>(Widths.data()) + (WidthsPerStyle - 1) * sizeof(int16_t), true);
                ++count2;
            }
        }
        return count1 >= 6 && count2 >= 6;
    }

    // The code section of the game
    std::pair<uint8_t*, uint8_t*> CodeRange()
    {
        auto module = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(module + reinterpret_cast<IMAGE_DOS_HEADER*>(module)->e_lfanew);
        auto section = IMAGE_FIRST_SECTION(nt);
        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section)
        {
            if (section->Characteristics & IMAGE_SCN_CNT_CODE)
                return { module + section->VirtualAddress, module + section->VirtualAddress + section->Misc.VirtualSize };
        }
        return { nullptr, nullptr };
    }

    struct CSprite2d
    {
        RwTexture* m_pTexture;
    };
    void* Sprite2dDraw = nullptr;   // CSprite2d::Draw(const CRect&, const CRGBA&, u0, v0, u1, v1, u3, v3, u2, v2)

    // The pictures of a text block, drawn once its characters are drawn
    struct QueuedPicture
    {
        float x, y, width, cellHeight;
        wchar_t c;
        uint8_t alpha;
    };
    std::vector<QueuedPicture> Queue;
    bool PendingAdvance = false;
    float PendingX = 0.0f;
    float PendingWidth = 0.0f;

    void DrawPicture(const QueuedPicture& q)
    {
        auto picture = PictureOf(q.c);
        if (!picture)
            return;
        float aspect = Aspect(q.c);
        float gap = q.width * PictureGap / WidthUnits(q.c);
        // As tall as the line, narrower when the text is narrower than the help box's proportions
        float h = std::min(q.cellHeight, (q.width - 2.0f * gap) / aspect);
        float w = h * aspect;
        float left = q.x + (q.width - w) * 0.5f, top = q.y + (q.cellHeight - h) * 0.5f;
        CRect rect(left, top, left + w, top + h);
        CRGBA color(255, 255, 255, q.alpha);
        CSprite2d sprite{ picture->texture };
        auto draw = reinterpret_cast<void(__thiscall*)(CSprite2d*, CRect*, CRGBA*, float, float, float, float, float, float, float, float)>(Sprite2dDraw);
        // CSprite2d's first corner is the rectangle's left and "top" (its larger y, the lower edge on screen)
        draw(&sprite, &rect, &color, picture->u0, picture->v1, picture->u1, picture->v1, picture->u0, picture->v0, picture->u1, picture->v0);
    }

    // CFont::RenderFontBuffer: the text's characters, then its pictures
    SafetyHookInline shRenderFontBuffer;
    void __cdecl RenderFontBuffer()
    {
        Queue.clear();
        PendingAdvance = false;
        shRenderFontBuffer.unsafe_ccall();
        if (Queue.empty())
            return;
        for (auto& q : Queue)
            DrawPicture(q);
        Queue.clear();
        // No texture left set for whatever draws next
        if (Rw.RenderStateSet)
            Rw.RenderStateSet(1, nullptr);
    }

    // The help message as the script gave it, so that it can be written again for another device
    void (__cdecl* InsertPlayerControlKeysInString)(wchar_t*) = nullptr;
    wchar_t* HelpMessage = nullptr;         // CHud::m_HelpMessage
    wchar_t* LastHelpMessage = nullptr;     // CHud::m_LastHelpMessage
    wchar_t* HelpMessageToPrint = nullptr;  // CHud::m_HelpMessageToPrint: what the box draws
    constexpr size_t HelpLength = 256;
    std::array<wchar_t, HelpLength> HelpSource{};
    int HelpState = -1;

    // What the prompts' pictures depend on: the device, the layout, the seat (foot, vehicle, aircraft), the key pictures
    // setting
    int PromptState()
    {
        if (Pad::IsPadLastDevice())
            return 1 + (Pad::IsModernLayout() ? 2 : 0) + int(Pad::PlayerSeat()) * 4;
        static auto& keyIcons = FixSettings().Int("MAIN.KeyboardIcons");
        return keyIcons ? 16 : 0;
    }

    SafetyHookInline shSetHelpMessage;
    void __cdecl SetHelpMessage(wchar_t* message, bool quick, bool displayForever)
    {
        if (message)
            wcsncpy_s(HelpSource.data(), HelpSource.size(), message, _TRUNCATE);
        else
            HelpSource[0] = 0;
        HelpState = PromptState();
        shSetHelpMessage.unsafe_ccall(message, quick, displayForever);
    }

    void RefreshHelpMessage()
    {
        if (!HelpMessage || !InsertPlayerControlKeysInString || !HelpSource[0])
            return;
        int state = PromptState();
        if (state == HelpState)
            return;
        HelpState = state;
        if (std::wcsstr(HelpSource.data(), L"~k~") == nullptr)
            return;
        std::array<wchar_t, HelpLength> text{};
        wcsncpy_s(text.data(), text.size(), HelpSource.data(), _TRUNCATE);
        InsertPlayerControlKeysInString(text.data());
        // A message already on screen changes in place; one not yet shown starts as usual
        bool shown = HelpMessageToPrint && std::wcsncmp(HelpMessageToPrint, HelpMessage, HelpLength) == 0;
        bool kept = LastHelpMessage && std::wcsncmp(LastHelpMessage, HelpMessage, HelpLength) == 0;
        std::wmemcpy(HelpMessage, text.data(), HelpLength);
        if (shown)
            std::wmemcpy(HelpMessageToPrint, text.data(), HelpLength);
        if (kept)
            std::wmemcpy(LastHelpMessage, text.data(), HelpLength);
    }
}

class ButtonIcons
{
public:
    ButtonIcons()
    {
        // CSprite2d::Draw, found before the fix's sprite hooks change its start
        WFP::onInitEvent() += []()
        {
            Sprite2dDraw = hook::pattern("8B 44 24 04 8B 54 24 08 53 89").get_first();
        };

        WFP::onGameInitEvent() += []()
        {
            if (GetModuleHandleW(L"GInputVC.asi"))
                return;

            auto commandKeys = hook::pattern("E8 ? ? ? ? 8B 44 24 10 03");
            auto stringWidth = hook::pattern("D9 05 ? ? ? ? 53 56 83");
            auto renderBuffer = hook::pattern("53 56 55 83 EC 18 81 3D");
            auto readChar = hook::pattern("66 8B 1E 83 C3 E0 80 3D");
            auto nextChar = hook::pattern("83 C6 02 3B 35 ? ? ? ? DD");
            auto rasterCreate = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 56 8B 54 01 60 52 FF 90 40");
            auto rasterLock = hook::pattern("8B 44 24 08 8B 54 24 0C 8B");
            auto rasterUnlock = hook::pattern("A1 ? ? ? ? 56 8B 74 ? ? ? ? 56 6A 00 FF 90 88");
            auto textureCreate = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 56 8B 54 01 08");
            auto renderState = hook::pattern("A1 ? ? ? ? 83 EC 08 83");
            // CFont::GetStringWidth reads the width table: movsx eax, word ptr [table + edx * 2]
            auto widthRead = stringWidth.get_first<uint8_t>();
            const int16_t* table = nullptr;
            for (int i = 0; i < 0x80 && !table; ++i)
            {
                auto p = widthRead + i;
                if (p[0] == 0x0F && p[1] == 0xBF && (p[2] & 0xC7) == 0x04 && (p[3] >> 6) == 1)
                    table = *reinterpret_cast<const int16_t**>(p + 4);
            }
            auto [codeBegin, codeEnd] = CodeRange();
            if (!table || !codeBegin || !ExtendWidthTable(table, codeBegin, codeEnd))
                return;

            Rw.RasterCreate = reinterpret_cast<decltype(Rw.RasterCreate)>(rasterCreate.get_first());
            Rw.RasterLock = reinterpret_cast<decltype(Rw.RasterLock)>(rasterLock.get_first());
            Rw.RasterUnlock = reinterpret_cast<decltype(Rw.RasterUnlock)>(rasterUnlock.get_first());
            Rw.TextureCreate = reinterpret_cast<decltype(Rw.TextureCreate)>(textureCreate.get_first());
            Rw.RenderStateSet = reinterpret_cast<decltype(Rw.RenderStateSet)>(renderState.get_first());

            hbGetWideStringOfCommandKeys.fun = injector::MakeCALL(commandKeys.get_first(), GetWideStringOfCommandKeys, true).get();
            shRenderFontBuffer = safetyhook::create_inline(renderBuffer.get_first(), RenderFontBuffer);

            // The character the renderer reads: a picture is queued and drawn as a space, x and y on the stack
            static auto ReadCharHook = safetyhook::create_mid(readChar.get_first(), [](SafetyHookContext& regs)
            {
                auto text = reinterpret_cast<wchar_t*>(regs.esi);
                if (!IsIcon(*text))
                    return;
                auto& state = *CFont::RenderState;
                float x = *reinterpret_cast<float*>(regs.esp);
                float y = *reinterpret_cast<float*>(regs.esp + 4);
                PendingX = x;
                PendingWidth = WidthUnits(*text) * state.scaleX;
                PendingAdvance = true;
                // Not for the drop shadow's and the outline's passes
                if (!state.bIsShadow && state.color.a)
                    Queue.push_back({ x, y, PendingWidth, 16.0f * state.scaleY, *text, state.color.a });
                *text = L' ';
            });

            // The next character's x: after the picture's width
            static auto NextCharHook = safetyhook::create_mid(nextChar.get_first(), [](SafetyHookContext& regs)
            {
                if (!PendingAdvance)
                    return;
                *reinterpret_cast<float*>(regs.esp) = PendingX + PendingWidth;
                PendingAdvance = false;
            });

            // CHud::SetHelpMessage, its buffers and CMessages::InsertPlayerControlKeysInString
            auto setHelp = hook::pattern("80 3D ? ? ? ? 01 53 8B");
            auto buffers = hook::pattern("68 00 01 00 00 55 68 ? ? ? ? E8 ? ? ? ? 83 C4 0C 68 ? ? ? ? E8 ? ? ? ? 83 3D ? ? ? ? 00 59 74 ? 68 00 01 00 00 68");
            auto start = buffers.get_first<uint8_t>();
            HelpMessage = *reinterpret_cast<wchar_t**>(start + 7);
            LastHelpMessage = *reinterpret_cast<wchar_t**>(start + 45);
            InsertPlayerControlKeysInString = reinterpret_cast<decltype(InsertPlayerControlKeysInString)>(injector::GetBranchDestination(start + 24).as_int());
            shSetHelpMessage = safetyhook::create_inline(setHelp.get_first(), SetHelpMessage);

            // CHud::Draw copies the message to m_HelpMessageToPrint when it shows it
            auto toPrint = hook::pattern("68 00 01 00 00 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 0C 84 C0 0F");
            HelpMessageToPrint = *toPrint.get_first<wchar_t*>(6);

            WFP::onGameProcessEvent() += []()
            {
                RefreshHelpMessage();
            };
        };
    }
} ButtonIcons;
