module;

#include <stdafx.h>

#include "common.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/ButtonIconAtlas.hpp"
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

export module ButtonIcons;

import Pad;
import Skeleton;

// Button pictures in the game's prompts. The game writes a control's keys into a prompt where its text has
// ~k~~ACTION~ (CMessages::InsertPlayerControlKeysInString): there the pad's button (while the pad is used) or the
// key's picture (MAIN.KeyboardIcons) is written as a token of the font, "~" and one character from 0x80 up and "~",
// the form of the font's own button tokens (~X~, ~<~...). The font's token parser shows them like its button symbols:
// the text renderer draws the picture in the symbol's place and moves on by the picture's width, and the font's width
// measuring counts that width, which it leaves out for symbols. A shown help message is written again when the
// device or the icon style changes, so its pictures follow the last device used.
// With a controller mod loaded that draws its own button pictures, none of it is installed.

using namespace RWGTA::ButtonIcons;

namespace
{
    // Token characters: the pad's pictures, then the keys' pictures by their cell in the atlas (without the two
    // Windows keys, so that every cell fits up to 0xFF)
    constexpr uint8_t PadFirst = 0x80;
    constexpr uint8_t KeyFirst = PadFirst + uint8_t(PadIcon::Count);
    constexpr int SkippedCell = 47;         // LWIN, RWIN
    constexpr int SkippedCells = 2;

    int CellCount()
    {
        int cells = 0;
        for (auto& key : KeyPictures)
            cells = std::max(cells, key.cell + 1);
        return cells;
    }

    int CodeOfCell(int cell)
    {
        if (cell >= SkippedCell && cell < SkippedCell + SkippedCells)
            return -1;
        int code = KeyFirst + (cell < SkippedCell ? cell : cell - SkippedCells);
        return code <= 0xFF ? code : -1;
    }

    int CellOfCode(uint8_t code)
    {
        int cell = code - KeyFirst;
        return cell < SkippedCell ? cell : cell + SkippedCells;
    }

    bool IsIcon(uint8_t c)
    {
        return c >= PadFirst && (c < KeyFirst ? true : CellOfCode(c) < CellCount());
    }

    const KeyPicture* KeyOf(uint8_t c)
    {
        int cell = CellOfCode(c);
        for (auto& key : KeyPictures)
            if (key.cell == cell)
                return &key;
        return nullptr;
    }

    float Aspect(uint8_t c)
    {
        if (c < KeyFirst)
            return 1.0f;
        auto key = KeyOf(c);
        return key ? key->width / 64.0f : 1.0f;
    }

    // The game's controls, by their index in CControllerConfigManager
    enum Action
    {
        PED_FIREWEAPON, PED_FIREWEAPON_ALT, PED_CYCLE_WEAPON_RIGHT, PED_CYCLE_WEAPON_LEFT, GO_FORWARD, GO_BACK, GO_LEFT,
        GO_RIGHT, PED_SNIPER_ZOOM_IN, PED_SNIPER_ZOOM_OUT, VEHICLE_ENTER_EXIT, CAMERA_CHANGE_VIEW_ALL_SITUATIONS,
        PED_JUMPING, PED_SPRINT, PED_LOOKBEHIND, PED_DUCK, PED_ANSWER_PHONE, SNEAK_ABOUT, VEHICLE_FIREWEAPON,
        VEHICLE_FIREWEAPON_ALT, VEHICLE_STEERLEFT, VEHICLE_STEERRIGHT, VEHICLE_STEERUP, VEHICLE_STEERDOWN,
        VEHICLE_ACCELERATE, VEHICLE_BRAKE, VEHICLE_RADIO_STATION_UP, VEHICLE_RADIO_STATION_DOWN, VEHICLE_RADIO_TRACK_SKIP,
        VEHICLE_HORN, TOGGLE_SUBMISSIONS, VEHICLE_HANDBRAKE, PED_1RST_PERSON_LOOK_LEFT, PED_1RST_PERSON_LOOK_RIGHT,
        VEHICLE_LOOKLEFT, VEHICLE_LOOKRIGHT, VEHICLE_LOOKBEHIND, VEHICLE_MOUSELOOK, VEHICLE_TURRETLEFT,
        VEHICLE_TURRETRIGHT, VEHICLE_TURRETUP, VEHICLE_TURRETDOWN, PED_CYCLE_TARGET_LEFT, PED_CYCLE_TARGET_RIGHT,
        PED_CENTER_CAMERA_BEHIND_PLAYER, PED_LOCK_TARGET, NETWORK_TALK, CONVERSATION_YES, CONVERSATION_NO,
        GROUP_CONTROL_FWD, GROUP_CONTROL_BWD, PED_1RST_PERSON_LOOK_UP, PED_1RST_PERSON_LOOK_DOWN, ActionCount = 59
    };

    using P = PadIcon;
    constexpr P None = P::Count;

    struct PadPrompt
    {
        P first, second = None;     // second: pressed together (look behind with both triggers)
    };

    // The buttons of each control in the console layout and in the modern one
    PadPrompt ClassicButton(int action)
    {
        switch (action)
        {
            case PED_FIREWEAPON: case VEHICLE_FIREWEAPON: return { P::B };
            case PED_FIREWEAPON_ALT: case VEHICLE_FIREWEAPON_ALT: return { P::LB };
            case PED_CYCLE_WEAPON_RIGHT: return { P::RT };
            case PED_CYCLE_WEAPON_LEFT: return { P::LT };
            case GO_FORWARD: case GO_BACK: case VEHICLE_STEERUP: case VEHICLE_STEERDOWN: return { P::LeftStickUpDown };
            case GO_LEFT: case GO_RIGHT: case VEHICLE_STEERLEFT: case VEHICLE_STEERRIGHT: return { P::LeftStickLeftRight };
            case SNEAK_ABOUT: return { P::LeftStick };
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
            case VEHICLE_RADIO_STATION_UP: return { P::DPadUp };
            case VEHICLE_RADIO_STATION_DOWN: return { P::DPadDown };
            case VEHICLE_HORN: return { P::LS };
            case TOGGLE_SUBMISSIONS: return { P::RS };
            case VEHICLE_HANDBRAKE: return { P::RB };
            case PED_1RST_PERSON_LOOK_LEFT: case PED_1RST_PERSON_LOOK_RIGHT: return { P::RightStickLeftRight };
            case PED_1RST_PERSON_LOOK_UP: case PED_1RST_PERSON_LOOK_DOWN: return { P::RightStickUpDown };
            case VEHICLE_LOOKLEFT: return { P::LT };
            case VEHICLE_LOOKRIGHT: return { P::RT };
            case VEHICLE_LOOKBEHIND: return { P::LT, P::RT };
            case VEHICLE_MOUSELOOK: return { P::RightStick };
            case VEHICLE_TURRETLEFT: case VEHICLE_TURRETRIGHT: return { P::RightStickLeftRight };
            case VEHICLE_TURRETUP: case VEHICLE_TURRETDOWN: return { P::RightStickUpDown };
            case PED_CYCLE_TARGET_LEFT: return { P::LT };
            case PED_CYCLE_TARGET_RIGHT: return { P::RT };
            case PED_CENTER_CAMERA_BEHIND_PLAYER: return { P::LB };
            case PED_LOCK_TARGET: return { P::RB };
            case CONVERSATION_YES: return { P::DPadRight };
            case CONVERSATION_NO: return { P::DPadLeft };
            case GROUP_CONTROL_FWD: return { P::DPadUp };
            case GROUP_CONTROL_BWD: return { P::DPadDown };
            default: return { None };
        }
    }

    enum class Seat { Foot, Vehicle, Aircraft };

    PadPrompt ModernButton(int action, Seat seat)
    {
        bool air = seat == Seat::Aircraft;
        switch (action)
        {
            case PED_FIREWEAPON: return { P::RT };
            case VEHICLE_FIREWEAPON: return { P::A };
            case PED_FIREWEAPON_ALT: return { P::RB };
            case VEHICLE_FIREWEAPON_ALT: return { P::DPadLeft };
            case PED_CYCLE_WEAPON_RIGHT: return { P::LB };
            case PED_CYCLE_WEAPON_LEFT: return { None };
            case GO_FORWARD: case GO_BACK: case VEHICLE_STEERUP: case VEHICLE_STEERDOWN: return { P::LeftStickUpDown };
            case GO_LEFT: case GO_RIGHT: case VEHICLE_STEERLEFT: case VEHICLE_STEERRIGHT: return { P::LeftStickLeftRight };
            case SNEAK_ABOUT: return { P::LeftStick };
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
            case VEHICLE_RADIO_STATION_UP: return { P::DPadRight };
            case VEHICLE_RADIO_STATION_DOWN: return { P::DPadDown };
            case VEHICLE_HORN: return { P::LS };
            case TOGGLE_SUBMISSIONS: return { P::DPadUp };
            case VEHICLE_HANDBRAKE: return { P::RB };
            case PED_1RST_PERSON_LOOK_LEFT: case PED_1RST_PERSON_LOOK_RIGHT: return { P::RightStickLeftRight };
            case PED_1RST_PERSON_LOOK_UP: case PED_1RST_PERSON_LOOK_DOWN: return { P::RightStickUpDown };
            case VEHICLE_LOOKLEFT: return { air ? P::LB : P::X };
            case VEHICLE_LOOKRIGHT: return { air ? P::RB : P::B };
            case VEHICLE_LOOKBEHIND: return { air ? None : P::RS };
            case VEHICLE_MOUSELOOK: return { P::RightStick };
            case VEHICLE_TURRETLEFT: case VEHICLE_TURRETRIGHT: return { P::RightStickLeftRight };
            case VEHICLE_TURRETUP: case VEHICLE_TURRETDOWN: return { P::RightStickUpDown };
            case PED_CYCLE_TARGET_LEFT: case PED_CYCLE_TARGET_RIGHT: return { P::RightStickLeftRight };
            case PED_CENTER_CAMERA_BEHIND_PLAYER: return { P::RB };
            case PED_LOCK_TARGET: return { P::LT };
            case CONVERSATION_YES: return { seat == Seat::Foot ? P::DPadRight : None };
            case CONVERSATION_NO: return { seat == Seat::Foot ? P::DPadLeft : None };
            case GROUP_CONTROL_FWD: return { P::DPadUp };
            case GROUP_CONTROL_BWD: return { P::DPadDown };
            default: return { None };
        }
    }

    // CControllerConfigManager's settings: per control and device type (keyboard, second keyboard key, mouse,
    // joystick) the key and its order in the control's list
    struct ControlBinding
    {
        int32_t key;
        int32_t order;
    };
    constexpr ptrdiff_t SettingsOffset = 0xB70;
    constexpr int32_t KeyNone = 1056;
    enum DeviceType { KEYBOARD, OPTIONAL_EXTRA, MOUSE, JOYSTICK };

    // The key the game names for the control (its mouse button, else its first key, else its second key), as the
    // token character of its picture; -1 when that key has no picture, so the game's name stays
    int KeyTokenOf(uint8_t* manager, int action)
    {
        auto bindings = reinterpret_cast<ControlBinding*>(manager + SettingsOffset + action * 4 * sizeof(ControlBinding));
        for (int type : { MOUSE, KEYBOARD, OPTIONAL_EXTRA })
        {
            auto& binding = bindings[type];
            bool bound = type == MOUSE ? binding.key != 0 : binding.key != KeyNone;
            if (!bound)
                continue;
            int code = binding.key;
            if (type != MOUSE && code >= 'a' && code <= 'z')
                code -= 'a' - 'A';
            if (type == MOUSE)
                code += 2000;
            for (auto& key : KeyPictures)
            {
                if (key.code == code)
                    return CodeOfCell(key.cell);
            }
            return -1;
        }
        return -1;
    }

    Seat CurrentSeat()
    {
        auto vehicle = FindPlayerVehicle ? FindPlayerVehicle(-1, false) : nullptr;
        if (!vehicle)
            return Seat::Foot;
        // CVehicle's subclass: 3 helicopter, 4 plane
        auto subclass = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(vehicle) + 0x594);
        return subclass == 3 || subclass == 4 ? Seat::Aircraft : Seat::Vehicle;
    }

    void WriteToken(char* text, int& n, uint8_t c)
    {
        text[n++] = '~';
        text[n++] = char(c);
        text[n++] = '~';
    }

    // CControllerConfigManager::GetDefinedKeyByGxtName at its call in InsertPlayerControlKeysInString
    using CommandKeysFn = int(__fastcall*)(void*, void*, uint16_t, char*, int16_t);
    CommandKeysFn CommandKeysPrevious = nullptr;
    int __fastcall CommandKeys(void* manager, void* edx, uint16_t action, char* text, int16_t length)
    {
        if (length >= 8 && action < ActionCount)
        {
            if (Pad::IsPadLastDevice())
            {
                auto prompt = Pad::IsModernLayout() ? ModernButton(action, CurrentSeat()) : ClassicButton(action);
                if (prompt.first != None)
                {
                    int n = 0;
                    WriteToken(text, n, uint8_t(PadFirst + int(prompt.first)));
                    if (prompt.second != None)
                    {
                        text[n++] = '+';
                        WriteToken(text, n, uint8_t(PadFirst + int(prompt.second)));
                    }
                    text[n] = 0;
                    return n;
                }
            }
            else
            {
                static auto& keyIcons = FixSettings().Int("MAIN.KeyboardIcons");
                if (keyIcons)
                {
                    int token = KeyTokenOf(static_cast<uint8_t*>(manager), action);
                    if (token >= 0)
                    {
                        int n = 0;
                        WriteToken(text, n, uint8_t(token));
                        text[n] = 0;
                        return n;
                    }
                }
            }
        }
        return CommandKeysPrevious(manager, edx, action, text, length);
    }

    // The atlases, as RenderWare textures made on first use
    struct RwFunctions
    {
        RwRaster* (__cdecl* RasterCreate)(int, int, int, int) = nullptr;
        uint8_t* (__cdecl* RasterLock)(RwRaster*, int, int) = nullptr;
        RwRaster* (__cdecl* RasterUnlock)(RwRaster*) = nullptr;
        RwTexture* (__cdecl* TextureCreate)(RwRaster*) = nullptr;
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

    std::optional<Picture> PictureOf(uint8_t c)
    {
        if (c >= KeyFirst)
        {
            auto key = KeyOf(c);
            auto texture = TextureOf(KeyAtlas);
            if (!key || !texture)
                return std::nullopt;
            float u = (key->cell % 8) * 128.0f / 1024.0f, v = (key->cell / 8) * 64.0f / 1024.0f;
            return Picture{ texture, u, v, u + key->width / 1024.0f, v + 64.0f / 1024.0f };
        }
        auto& atlas = PadAtlases[int(Pad::Style()) - 1];
        auto texture = TextureOf(atlas);
        if (!texture)
            return std::nullopt;
        int cell = c - PadFirst;
        float u = (cell % 8) * 64.0f / 512.0f, v = (cell / 8) * 64.0f / 256.0f;
        return Picture{ texture, u, v, u + 64.0f / 512.0f, v + 64.0f / 256.0f };
    }

    // The font's state
    GameRef<uint8_t> PS2Symbol;             // CFont::PS2Symbol: the button symbol of the token being drawn
    GameRef<float> RenderHeight;            // CFont::RenderState.m_fHeigth
    GameRef<uint8_t> RenderAlpha;           // CFont::RenderState.m_color.alpha
    GameRef<float> FontScaleY;              // CFont::Scale.y
    void* Sprite2dDraw = nullptr;           // CSprite2d::Draw(const CRect&, const CRGBA&, u1, v1, u2, v2, u3, v3, u4, v4)

    // The font's button symbols are 17 units of the height scale wide
    constexpr float SymbolWidth = 17.0f;

    struct CSprite2d
    {
        RwTexture* m_pTexture;
    };

    // CFont::ParseToken: the pictures' tokens are button symbols
    SafetyHookInline shParseToken;
    char* __cdecl ParseToken(char* text, void* color, bool noColor, char* tag)
    {
        auto c = uint8_t(text[1]);
        if (IsIcon(c) && text[2] == '~' && PS2Symbol.is_initialized())
        {
            *PS2Symbol = c;
            return text + 3;
        }
        return shParseToken.unsafe_ccall<char*>(text, color, noColor, tag);
    }

    // CFont::PrintChar: a picture's symbol is drawn here
    SafetyHookInline shPrintChar;
    void __cdecl PrintChar(float x, float y, char letter)
    {
        auto c = PS2Symbol.is_initialized() ? *PS2Symbol : 0;
        if (!IsIcon(c))
            return shPrintChar.unsafe_ccall(x, y, letter);
        if (y < 0.0f || y > SCREEN_HEIGHT || x < 0.0f || x > SCREEN_WIDTH || !Sprite2dDraw)
            return;
        auto picture = PictureOf(c);
        if (!picture)
            return;
        // As the font's symbols: from 2 to 19 units below the line's top, as wide as the picture is
        float h = *RenderHeight;
        float width = SymbolWidth * h * Aspect(c), inset = h;
        // CRect: left, bottom, right, top (y grows down: "bottom" is the larger y)
        float rect[4] = { x + inset * 0.5f, y + 19.0f * h, x + width - inset * 0.5f, y + 2.0f * h };
        uint8_t color[4] = { 255, 255, 255, *RenderAlpha };
        CSprite2d sprite{ picture->texture };
        auto draw = reinterpret_cast<void(__thiscall*)(CSprite2d*, float*, uint8_t*, float, float, float, float, float, float, float, float)>(Sprite2dDraw);
        draw(&sprite, rect, color, picture->u0, picture->v0, picture->u1, picture->v0, picture->u0, picture->v1, picture->u1, picture->v1);
    }

    void (__cdecl* InsertPlayerControlKeysInString)(char*) = nullptr;

    // The help message as the script gave it, so that it can be written again for another device
    char* HelpMessage = nullptr;            // CHud::m_pHelpMessage
    char* LastHelpMessage = nullptr;        // CHud::m_pLastHelpMessage
    char* HelpMessageToPrint = nullptr;     // CHud::m_pHelpMessageToPrint: what the box draws
    constexpr size_t HelpLength = 400;
    std::array<char, HelpLength> HelpSource{};
    int HelpState = -1;

    // What the prompts' pictures depend on: the device, the layout, being in a vehicle, the key pictures setting
    int PromptState()
    {
        if (Pad::IsPadLastDevice())
            return 1 + (Pad::IsModernLayout() ? 2 : 0) + int(CurrentSeat()) * 4 + int(Pad::Style()) * 16;
        static auto& keyIcons = FixSettings().Int("MAIN.KeyboardIcons");
        return keyIcons ? 128 : 0;
    }

    SafetyHookInline shSetHelpMessage;
    void __cdecl SetHelpMessage(const char* message, bool quick, bool permanent, bool addToBrief)
    {
        if (message)
            strncpy_s(HelpSource.data(), HelpSource.size(), message, _TRUNCATE);
        else
            HelpSource[0] = 0;
        HelpState = PromptState();
        shSetHelpMessage.unsafe_ccall(message, quick, permanent, addToBrief);
    }

    void RefreshHelpMessage()
    {
        if (!HelpMessage || !InsertPlayerControlKeysInString || !HelpSource[0])
            return;
        int state = PromptState();
        if (state == HelpState)
            return;
        HelpState = state;
        if (std::strstr(HelpSource.data(), "~k~") == nullptr)
            return;
        std::array<char, HelpLength> text{};
        strncpy_s(text.data(), text.size(), HelpSource.data(), _TRUNCATE);
        InsertPlayerControlKeysInString(text.data());
        // A message already on screen changes in place; one not yet shown starts as usual
        bool shown = HelpMessageToPrint && std::strncmp(HelpMessageToPrint, HelpMessage, HelpLength) == 0;
        bool kept = LastHelpMessage && std::strncmp(LastHelpMessage, HelpMessage, HelpLength) == 0;
        std::memcpy(HelpMessage, text.data(), HelpLength);
        if (shown)
            std::memcpy(HelpMessageToPrint, text.data(), HelpLength);
        if (kept)
            std::memcpy(LastHelpMessage, text.data(), HelpLength);
    }
}

class ButtonIcons
{
public:
    ButtonIcons()
    {
        WFP::onGameInitEvent() += []()
        {
            if (GetModuleHandleW(L"GInputSA.asi") || !Pad::IsInstalled() && !FixSettings().Int("MAIN.KeyboardIcons"))
                return;

            auto commandKeys = hook::pattern("C6 44 24 1F 01 E8");
            auto insertKeys = hook::pattern("8B 54 24 04 81 EC");
            auto parseToken = hook::pattern("83 EC 20 57 8B 7C");
            auto printChar = hook::pattern("D9 44 24 08 83 EC 20");
            auto advance = hook::pattern("8A 15 ? ? ? ? 8A 4C 24 ? 83 C4 0C 84 D2 74 25");
            auto stringWidth = hook::pattern("8A 46 01 46 3C 7E 74 0F");
            auto setScale = hook::pattern("8B 44 24 04 8B 4C 24 08 A3 ? ? ? ? 89 0D ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 90 90 0F BE 05");
            auto draw = hook::pattern("8B 44 24 28 8B 54 24 20 56");
            auto rasterCreate = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 56 68 07 04");
            auto rasterLock = hook::pattern("8B 44 24 08 8B 54 24 0C 8B 4C 24 04 25");
            auto rasterUnlock = hook::pattern("A1 ? ? ? ? 56 8B 74 ? ? ? ? 56 6A 00 FF 90 88");
            auto textureCreate = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 56 68 06");
            PS2Symbol.SetAddress(*advance.get_first<uint8_t*>(2));
            RenderHeight.SetAddress(*advance.get_first<float*>(26));
            RenderAlpha.SetAddress(reinterpret_cast<uint8_t*>(RenderHeight.get_ptr()) + 7);     // RenderState.m_color, after the height
            FontScaleY.SetAddress(*setScale.get_first<float*>(15));
            Sprite2dDraw = draw.get_first();
            InsertPlayerControlKeysInString = reinterpret_cast<decltype(InsertPlayerControlKeysInString)>(insertKeys.get_first());
            Rw.RasterCreate = reinterpret_cast<decltype(Rw.RasterCreate)>(rasterCreate.get_first());
            Rw.RasterLock = reinterpret_cast<decltype(Rw.RasterLock)>(rasterLock.get_first());
            Rw.RasterUnlock = reinterpret_cast<decltype(Rw.RasterUnlock)>(rasterUnlock.get_first());
            Rw.TextureCreate = reinterpret_cast<decltype(Rw.TextureCreate)>(textureCreate.get_first());

            CommandKeysPrevious = reinterpret_cast<CommandKeysFn>(injector::MakeCALL(commandKeys.get_first(5), CommandKeys, true).as_int());
            shParseToken = safetyhook::create_inline(parseToken.get_first(), ParseToken);
            shPrintChar = safetyhook::create_inline(printChar.get_first(), PrintChar);

            // CFont::GetStringWidth skips a token with no width: a picture's token counts its width
            static auto WidthHook = safetyhook::create_mid(stringWidth.get_first(), [](SafetyHookContext& regs)
            {
                auto token = reinterpret_cast<const uint8_t*>(regs.esi);
                if (IsIcon(token[1]) && token[2] == '~')
                    *reinterpret_cast<float*>(regs.esp + 0xC) += SymbolWidth * *FontScaleY * Aspect(token[1]);
            });

            // CFont::RenderFontBuffer moves on by 17 height units after a symbol: a wider picture by its width
            static auto AdvanceHook = safetyhook::create_mid(advance.get_first(13), [](SafetyHookContext& regs)
            {
                auto c = uint8_t(regs.edx & 0xFF);
                if (!IsIcon(c))
                    return;
                auto& x = *reinterpret_cast<float*>(regs.esp + 0xC);
                x += SymbolWidth * *RenderHeight * (Aspect(c) - 1.0f);
            });

            // CHud::SetHelpMessage and its buffers
            auto start = hook::pattern("A0 ? ? ? ? 84 C0 0F 85 4F").get_first<uint8_t>();
            const auto buffersAt = reinterpret_cast<uintptr_t>(start + 65);
            hook::range_pattern(buffersAt, buffersAt + 35, "BF ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? BF ? ? ? ? ? ? ? ? ? ? ? ? BF ? ? ? ?").get_first();
            {
                HelpMessageToPrint = *reinterpret_cast<char**>(start + 66);
                LastHelpMessage = *reinterpret_cast<char**>(start + 83);
                HelpMessage = *reinterpret_cast<char**>(start + 96);
                shSetHelpMessage = safetyhook::create_inline(start, SetHelpMessage);
            }

            WFP::onGameProcessEvent() += []()
            {
                RefreshHelpMessage();
            };
        };
    }
} ButtonIcons;
