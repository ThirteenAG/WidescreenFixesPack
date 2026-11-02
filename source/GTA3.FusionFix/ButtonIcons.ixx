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
// key's picture (MAIN.KeyboardIcons) is written as one character of the Unicode private use area, which the font
// draws as the picture and measures by the picture's width. A shown help message is written again when the device
// or the icon style changes, so its pictures follow the last device used.
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

    // The game's controls, by their index in CControllerConfigManager
    enum Action
    {
        PED_FIREWEAPON, PED_CYCLE_WEAPON_RIGHT, PED_CYCLE_WEAPON_LEFT, GO_FORWARD, GO_BACK, GO_LEFT, GO_RIGHT,
        PED_SNIPER_ZOOM_IN, PED_SNIPER_ZOOM_OUT, VEHICLE_ENTER_EXIT, CAMERA_CHANGE_VIEW_ALL_SITUATIONS, PED_JUMPING,
        PED_SPRINT, PED_LOOKBEHIND, VEHICLE_ACCELERATE, VEHICLE_BRAKE, VEHICLE_CHANGE_RADIO_STATION, VEHICLE_HORN,
        TOGGLE_SUBMISSIONS, VEHICLE_HANDBRAKE, PED_1RST_PERSON_LOOK_LEFT, PED_1RST_PERSON_LOOK_RIGHT,
        VEHICLE_LOOKLEFT, VEHICLE_LOOKRIGHT, VEHICLE_LOOKBEHIND, VEHICLE_TURRETLEFT, VEHICLE_TURRETRIGHT,
        VEHICLE_TURRETUP, VEHICLE_TURRETDOWN, PED_CYCLE_TARGET_LEFT, PED_CYCLE_TARGET_RIGHT,
        PED_CENTER_CAMERA_BEHIND_PLAYER, PED_LOCK_TARGET, NETWORK_TALK, PED_1RST_PERSON_LOOK_UP,
        PED_1RST_PERSON_LOOK_DOWN, ActionCount = 41
    };

    using P = PadIcon;
    constexpr P None = P::Count;

    struct PadPrompt
    {
        P first, second = None;     // second: pressed together (look behind with both triggers)
    };

    // The buttons of each control in the console layout (the game's first configuration) and in the modern one
    PadPrompt ClassicButton(int action, bool inVehicle)
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

    PadPrompt ModernButton(int action, bool inVehicle)
    {
        switch (action)
        {
            case PED_FIREWEAPON: return { inVehicle ? P::A : P::RT };
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
            case VEHICLE_ACCELERATE: return { P::RT };
            case VEHICLE_BRAKE: return { P::LT };
            case VEHICLE_CHANGE_RADIO_STATION: return { P::DPadRight };
            case VEHICLE_HORN: return { P::LS };
            case TOGGLE_SUBMISSIONS: return { P::DPadUp };
            case VEHICLE_HANDBRAKE: return { P::RB };
            case PED_1RST_PERSON_LOOK_LEFT: case PED_1RST_PERSON_LOOK_RIGHT: return { P::RightStickLeftRight };
            case PED_1RST_PERSON_LOOK_UP: case PED_1RST_PERSON_LOOK_DOWN: return { P::RightStickUpDown };
            case VEHICLE_LOOKLEFT: return { P::X };
            case VEHICLE_LOOKRIGHT: return { P::B };
            case VEHICLE_LOOKBEHIND: return { P::RS };
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
    constexpr ptrdiff_t SettingsOffset = 0xF08;
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
                bool inVehicle = FindPlayerVehicle && FindPlayerVehicle();
                auto prompt = Pad::IsModernLayout() ? ModernButton(action, inVehicle) : ClassicButton(action, inVehicle);
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
        float aspect;               // width / height
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
            return Picture{ texture, u, v, u + key.width / 1024.0f, v + 64.0f / 1024.0f, key.width / 64.0f };
        }
        auto& atlas = PadAtlases[int(Pad::Style()) - 1];
        auto texture = TextureOf(atlas);
        if (!texture)
            return std::nullopt;
        int cell = c - PadFirst;
        float u = (cell % 8) * 64.0f / 512.0f, v = (cell / 8) * 64.0f / 256.0f;
        return Picture{ texture, u, v, u + 64.0f / 512.0f, v + 64.0f / 256.0f, 1.0f };
    }

    // The picture's size in the font: a little taller than the capital letters
    constexpr float PictureHeight = 18.0f;
    constexpr float PictureGap = 1.5f;

    float PictureAdvance(wchar_t c)
    {
        float aspect = c >= KeyFirst ? KeyPictures[c - KeyFirst].width / 64.0f : 1.0f;
        auto& scale = CFont::Details->m_vecScale;
        return PictureHeight * aspect * scale.y + PictureGap * 2.0f * scale.x;
    }

    struct CSprite2d
    {
        RwTexture* m_pTexture;
    };
    void* Sprite2dDraw = nullptr;   // CSprite2d::Draw(const CRect&, const CRGBA&, u0, v0, u1, v1, u3, v3, u2, v2)

    void DrawPicture(float x, float y, wchar_t c)
    {
        // Not again for the outline's passes nor for the drop shadow
        if (CFont::OutlinePass || !Sprite2dDraw)
            return;
        auto picture = PictureOf(c);
        if (!picture)
            return;
        auto& details = *CFont::Details;
        float sx = details.m_vecScale.x, sy = details.m_vecScale.y;
        // The pictures keep their proportions whatever the font's width scale
        float h = PictureHeight * sy, w = h * picture->aspect;
        float left = x + PictureGap * sx, top = y + 1.0f * sy;
        CRect rect(left, top, left + w, top + h);
        CRGBA color(255, 255, 255, details.m_Color.a);
        CSprite2d sprite{ picture->texture };
        auto draw = reinterpret_cast<void(__thiscall*)(CSprite2d*, CRect*, CRGBA*, float, float, float, float, float, float, float, float)>(Sprite2dDraw);
        // CSprite2d's first corner is the rectangle's left and "top" (its larger y, the lower edge on screen)
        draw(&sprite, &rect, &color, picture->u0, picture->v1, picture->u1, picture->v1, picture->u0, picture->v0, picture->u1, picture->v0);
    }

    // CFont::PrintChar(x, y, character - ' ')
    SafetyHookInline shPrintChar;
    void __cdecl PrintChar(float x, float y, uint16_t c)
    {
        wchar_t ch = wchar_t(c + L' ');
        if (IsIcon(ch))
        {
            if (x > 0.0f && y > 0.0f && x < SCREEN_WIDTH && y < SCREEN_HEIGHT)
                DrawPicture(x, y, ch);
            return;
        }
        shPrintChar.unsafe_ccall(x, y, c);
    }

    // CFont::GetCharacterSize(character - ' ')
    SafetyHookInline shGetCharacterSize;
    float __cdecl GetCharacterSize(uint16_t c)
    {
        wchar_t ch = wchar_t(c + L' ');
        if (IsIcon(ch))
            return PictureAdvance(ch);
        return shGetCharacterSize.unsafe_ccall<float>(c);
    }

    // CFont::GetStringWidth: as the game's, with the pictures' widths
    SafetyHookInline shGetStringWidth;
    float __cdecl GetStringWidth(wchar_t* s, bool spaces)
    {
        float width = 0.0f;
        while (true)
        {
            wchar_t c = *s;
            if ((c == L' ' && !spaces) || c == 0)
                break;
            if (c == L'~')
            {
                do
                    ++s;
                while (*s != L'~');
                ++s;
                if (*s == L' ' && !spaces)
                    break;
            }
            width += GetCharacterSize(uint16_t(*s - L' '));
            ++s;
        }
        return width;
    }

    // The help message as the script gave it, so that it can be written again for another device
    void (__cdecl* InsertPlayerControlKeysInString)(wchar_t*) = nullptr;
    wchar_t* HelpMessage = nullptr;         // CHud::m_HelpMessage
    wchar_t* LastHelpMessage = nullptr;     // CHud::m_LastHelpMessage: the shown one
    wchar_t* HelpMessageToPrint = nullptr;  // CHud::m_HelpMessageToPrint: what the box draws
    constexpr size_t HelpLength = 256;
    std::array<wchar_t, HelpLength> HelpSource{};
    int HelpState = -1;

    // What the prompts' pictures depend on: the device, the layout, being in a vehicle, the key pictures setting
    int PromptState()
    {
        if (Pad::IsPadLastDevice())
            return 1 + (Pad::IsModernLayout() ? 2 : 0) + ((FindPlayerVehicle && FindPlayerVehicle()) ? 4 : 0);
        static auto& keyIcons = FixSettings().Int("MAIN.KeyboardIcons");
        return keyIcons ? 8 : 0;
    }

    SafetyHookInline shSetHelpMessage;
    void __cdecl SetHelpMessage(wchar_t* message, bool quick)
    {
        if (message)
            wcsncpy_s(HelpSource.data(), HelpSource.size(), message, _TRUNCATE);
        else
            HelpSource[0] = 0;
        HelpState = PromptState();
        shSetHelpMessage.unsafe_ccall(message, quick);
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
        bool shown = std::wcsncmp(LastHelpMessage, HelpMessage, HelpLength) == 0;
        std::wmemcpy(HelpMessage, text.data(), HelpLength);
        if (shown)
        {
            std::wmemcpy(LastHelpMessage, text.data(), HelpLength);
            if (HelpMessageToPrint)
                std::wmemcpy(HelpMessageToPrint, text.data(), HelpLength);
        }
    }
}

class ButtonIcons
{
public:
    ButtonIcons()
    {
        WFP::onGameInitEvent() += []()
        {
            if (GetModuleHandleW(L"GInputIII.asi"))
                return;

            auto pattern = hook::pattern("E8 ? ? ? ? 8D 84 24 18 02 00 00 01");
            auto printChar = hook::pattern("8B 15 ? ? ? ? 53 83 EC 48");
            auto characterSize = hook::pattern("E8 ? ? ? ? D8 44 24 3C");
            auto stringWidth = hook::pattern("0F BF 15 ? ? ? ? 53");
            auto draw = hook::pattern("E8 ? ? ? ? E9 ? ? ? ? 89 C0 8D 40 ? 8D 8C 24 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? 50");
            auto rasterCreate = hook::pattern("E8 ? ? ? ? 83 C4 ? 85 C0 A3 ? ? ? ? 75 ? 68");
            auto rasterLock = find_pattern("E8 ? ? ? ? 8B 46 ? 56", "E8 ? ? ? ? 85 C0 74 ? 8B 84 24 ? ? ? ? ? ? ? 8B 84 84");
            auto rasterUnlock = find_pattern("E8 ? ? ? ? 8D 4C 24 ? 8D 54 24 ? 51 8D 44 24", "E8 ? ? ? ? 8B 84 24 ? ? ? ? 8B 84 84");
            auto textureCreate = hook::pattern("E8 ? ? ? ? 89 C3 59 55");
            Rw.RasterCreate = reinterpret_cast<decltype(Rw.RasterCreate)>(injector::GetBranchDestination(rasterCreate.get_first()).as_int());
            Rw.RasterLock = reinterpret_cast<decltype(Rw.RasterLock)>(injector::GetBranchDestination(rasterLock.get_first()).as_int());
            Rw.RasterUnlock = reinterpret_cast<decltype(Rw.RasterUnlock)>(injector::GetBranchDestination(rasterUnlock.get_first()).as_int());
            Rw.TextureCreate = reinterpret_cast<decltype(Rw.TextureCreate)>(injector::GetBranchDestination(textureCreate.get_first()).as_int());
            Sprite2dDraw = injector::GetBranchDestination(draw.get_first()).get<void>();

            hbGetWideStringOfCommandKeys.fun = injector::MakeCALL(pattern.get_first(), GetWideStringOfCommandKeys, true).get();
            shPrintChar = safetyhook::create_inline(printChar.get_first(), PrintChar);
            shGetCharacterSize = safetyhook::create_inline(injector::GetBranchDestination(characterSize.get_first()).as_int(), GetCharacterSize);
            shGetStringWidth = safetyhook::create_inline(stringWidth.get_first(), GetStringWidth);

            // CHud::SetHelpMessage, its buffers and CMessages::InsertPlayerControlKeysInString
            auto setHelp = hook::pattern("80 3D ? ? ? ? 01 75 07 C3");
            auto insertKeys = hook::pattern("53 56 57 55 81 EC 18 04");
            auto start = setHelp.get_first<uint8_t>();
            // push offset m_HelpMessage; call InsertPlayerControlKeysInString; ... mov word ptr m_LastHelpMessage[ecx*2], 0
            const auto buffersAt = reinterpret_cast<uintptr_t>(start + 0x27);
            hook::range_pattern(buffersAt, buffersAt + 24, "68 ? ? ? ? E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 66").get_first();
            {
                HelpMessage = *reinterpret_cast<wchar_t**>(start + 0x28);
                LastHelpMessage = *reinterpret_cast<wchar_t**>(start + 0x42);
                InsertPlayerControlKeysInString = reinterpret_cast<decltype(InsertPlayerControlKeysInString)>(insertKeys.get_first());
                shSetHelpMessage = safetyhook::create_inline(start, SetHelpMessage);
            }

            // CHud::DrawAfterFade copies the message to m_HelpMessageToPrint when it shows it
            auto toPrint = hook::pattern("68 ? ? ? ? 68 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00 C7 05 ? ? ? ? 02 00 00 00");
            HelpMessageToPrint = *toPrint.get_first<wchar_t*>(6);

            WFP::onGameProcessEvent() += []()
            {
                RefreshHelpMessage();
            };
        };
    }
} ButtonIcons;
