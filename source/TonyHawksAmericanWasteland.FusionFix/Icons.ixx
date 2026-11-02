module;

#include <stdafx.h>
#include <d3d9.h>
#include <mutex>
#include <unordered_map>

export module Icons;

import Hud;
import Controller;
import ComVars;

// Rasterize each icon into the game font's existing glyph rectangles.
namespace THAWIconArt
{
    struct Rect { int x, y, w, h; };
    inline constexpr std::array<std::array<uint8_t, 7>, 18> Letters{{
        {{14,17,17,31,17,17,17}}, // A
        {{30,17,17,30,17,17,30}}, // B
        {{14,17,16,16,16,17,14}}, // C
        {{30,17,17,17,17,17,30}}, // D
        {{31,16,16,30,16,16,31}}, // E
        {{31,16,16,30,16,16,16}}, // F
        {{14,17,16,23,17,17,15}}, // G
        {{17,17,17,31,17,17,17}}, // H
        {{31,4,4,4,4,4,31}},      // I
        {{7,2,2,2,18,18,12}},     // J
        {{17,18,20,24,20,18,17}}, // K
        {{16,16,16,16,16,16,31}}, // L
        {{17,27,21,21,17,17,17}}, // M
        {{17,25,21,19,17,17,17}}, // N
        {{14,17,17,17,17,17,14}}, // O
        {{30,17,17,30,16,16,16}}, // P
        {{14,17,17,17,21,18,13}}, // Q
        {{30,17,17,30,20,18,17}}  // R
    }};
    inline constexpr std::array<std::array<uint8_t, 7>, 8> Tail{{
        {{15,16,16,14,1,1,30}},   // S
        {{31,4,4,4,4,4,4}},       // T
        {{17,17,17,17,17,17,14}}, // U
        {{17,17,17,17,17,10,4}},  // V
        {{17,17,17,21,21,21,10}}, // W
        {{17,17,10,4,10,17,17}},  // X
        {{17,17,10,4,4,4,4}},     // Y
        {{31,1,2,4,8,16,31}}      // Z
    }};
    inline uint8_t Row(char c, int row)
    {
        if (c >= 'A' && c <= 'R') return Letters[c-'A'][row];
        if (c >= 'S' && c <= 'Z') return Tail[c-'S'][row];
        if (c == '1') { constexpr uint8_t rows[]{4,12,4,4,4,4,14}; return rows[row]; }
        if (c == '2') { constexpr uint8_t rows[]{14,17,1,2,4,8,31}; return rows[row]; }
        if (c == '3') { constexpr uint8_t rows[]{30,1,1,14,1,1,30}; return rows[row]; }
        if (c == '+') { constexpr uint8_t rows[]{0,4,4,31,4,4,0}; return rows[row]; }
        if (c == '-') { constexpr uint8_t rows[]{0,0,0,31,0,0,0}; return rows[row]; }
        return 0;
    }
    inline void Paint(uint32_t* pixels, int pitch, Rect r, int icon, int family)
    {
        if (r.w < 5 || r.h < 5) return;
        // Some keyboard glyphs (notably Escape and Shift) are several times
        // wider than they are tall. Keep the pad symbol proportional while
        // retaining the native text advance and transparent padding.
        const bool round = icon < 4 || icon >= 12;
        const int maxWidth = round ? r.h : r.h * 3 / 2;
        if (r.w > maxWidth) { r.x += (r.w - maxWidth) / 2; r.w = maxWidth; }
        const int maxHeight = round ? r.w : r.w * 3 / 2;
        if (r.h > maxHeight) { r.y += (r.h - maxHeight) / 2; r.h = maxHeight; }
        constexpr const char* labels[24] = {"A","B","X","Y","LB","RB","LT","RT","BK","ST","LS","RS",
                                             "D","D","D","D","LS","LS","LS","LS","RS","RS","RS","RS"};
        constexpr const char* nintendo[] = {"B","A","Y","X"};
        constexpr const char* playstation[] = {"X","O","S","T"};
        constexpr const char* psExtra[] = {"L1","R1","L2","R2","SH","OP","L3","R3"};
        constexpr const char* nintendoExtra[] = {"L","R","ZL","ZR","-","+","LS","RS"};
        const char* label = icon == 38 ? "D" : icon == 37 ? "RS" : icon == 36 ? "LS" :
                            icon >= 32 ? "RS" : icon >= 28 ? "D" : icon >= 24 ? "LS" :
                            icon < 4 && family == 2 ? playstation[icon] :
                            icon < 4 && family == 3 ? nintendo[icon] :
                            icon >= 4 && icon < 12 && family == 2 ? psExtra[icon-4] :
                            icon >= 4 && icon < 12 && family == 3 ? nintendoExtra[icon-4] : labels[icon];
        constexpr uint32_t xbox[] = {0xff247e3d,0xffb1262e,0xff2368a9,0xffa78613};
        constexpr uint32_t ps[] = {0xff2765aa,0xffad333e,0xff9d4d91,0xff348b73};
        const uint32_t fill = icon < 4 ? (family == 2 ? ps[icon] : family == 3 ? 0xff55585c : xbox[icon]) : 0xff3d4146;
        const int n = static_cast<int>(std::strlen(label));
        const int textWidth = n*5 + n-1;
        const int sx = std::max(1, (r.w-4)/textWidth);
        const int sy = std::max(1, (r.h-4)/7);
        const int scale = std::min(sx,sy);
        const int ox = r.x+(r.w-textWidth*scale)/2;
        const int oy = r.y+(r.h-7*scale)/2;
        for (int y=0;y<r.h;++y)
            for (int x=0;x<r.w;++x)
            {
                const float nx = (2.0f*x+1-r.w)/r.w, ny = (2.0f*y+1-r.h)/r.h;
                const float d = nx*nx+ny*ny;
                const bool circle = round;
                const bool inside = circle ? d <= 0.93f : x>0 && y>0 && x<r.w-1 && y<r.h-1;
                if (!inside) continue;
                const bool edge = circle ? d > 0.70f : x==1 || y==1 || x==r.w-2 || y==r.h-2;
                pixels[(r.y+y)*pitch+r.x+x] = edge ? 0xffe0e0e0 : fill;
            }
        for (int i=0;i<n && !(family==2 && icon<4);++i)
            for (int y=0;y<7;++y)
                for (int x=0;x<5;++x)
                    if ((Row(label[i],y) >> (4-x)) & 1)
                        for (int dy=0;dy<scale;++dy)
                            for (int dx=0;dx<scale;++dx)
                            {
                                const int px=ox+(i*6+x)*scale+dx, py=oy+y*scale+dy;
                                if(px>=r.x && px<r.x+r.w && py>=r.y && py<r.y+r.h)
                                    pixels[py*pitch+px]=0xffffffff;
                            }
        if (family==2 && icon<4)
        {
            for (int y=0;y<r.h;++y) for (int x=0;x<r.w;++x)
            {
                const float nx=(2.0f*x+1-r.w)/r.w, ny=(2.0f*y+1-r.h)/r.h;
                const float ax=nx<0?-nx:nx, ay=ny<0?-ny:ny;
                bool mark=false;
                if(icon==0) mark=ax<0.46f && ay<0.46f && (nx-ny<0.12f && nx-ny>-0.12f || nx+ny<0.12f && nx+ny>-0.12f);
                if(icon==1) { const float d=nx*nx+ny*ny; mark=d>0.10f && d<0.23f; }
                if(icon==2) mark=ax<0.43f && ay<0.43f && (ax>0.28f || ay>0.28f);
                if(icon==3) mark=ny>-0.42f && ny<0.4f && (ny>0.25f && ax<0.43f || ax>(ny+0.42f)*0.47f-0.07f && ax<(ny+0.42f)*0.47f+0.07f);
                if(mark) pixels[(r.y+y)*pitch+r.x+x]=0xffffffff;
            }
        }
        if (icon >= 24 && icon < 36)
        {
            const int direction=(icon-24)%4;
            const bool right=(direction&1)!=0, down=(direction&2)!=0;
            for(int k=0;k<3;++k)
            {
                const int px=right ? r.x+r.w-2-k : r.x+1+k;
                const int py=down ? r.y+r.h-2-k : r.y+1+k;
                pixels[py*pitch+px]=0xffffffff;
            }
        }
        else if (icon >= 12 && icon < 24)
        {
            // Direction marker is kept inside the native rectangle.
            const int direction=(icon-12)%4;
            const int cx=r.x+r.w/2, cy=r.y+r.h/2;
            for(int k=0;k<3;++k)
            {
                const int px=direction==2 ? r.x+1+k : direction==3 ? r.x+r.w-2-k : cx;
                const int py=direction==0 ? r.y+1+k : direction==1 ? r.y+r.h-2-k : cy;
                pixels[py*pitch+px]=0xffffffff;
            }
            if(direction<2)
            {
                const int py=direction==0 ? r.y+3 : r.y+r.h-4;
                pixels[py*pitch+cx-1]=pixels[py*pitch+cx+1]=0xffffffff;
            }
            else
            {
                const int px=direction==2 ? r.x+3 : r.x+r.w-4;
                pixels[(cy-1)*pitch+px]=pixels[(cy+1)*pitch+px]=0xffffffff;
            }
        }
    }
}

namespace THAWIcons
{
    SafetyHookInline MetaHook;
    SafetyHookMid ButtonBoundary, TextureHook;
    inline int (__cdecl* Number)(const char*) = nullptr;
    inline const int32_t* Family = nullptr;
    inline thread_local uintptr_t TextElement = 0;
    inline thread_local const char* CurrentString = nullptr;
    inline thread_local int Pending = -1, Active = -1;
    inline std::mutex TextMutex;
    std::unordered_map<std::string, bool> TextContexts;
    inline IDirect3DDevice9* AtlasDevice = nullptr;
    inline IDirect3DTexture9* Atlas[3][39]{};

    inline bool IsMenuHelper(uintptr_t element)
    {
        for (int depth = 0; element && depth < 32; ++depth)
        {
            const auto id = *reinterpret_cast<const uint32_t*>(element + 24);
            if (id == THAWHud::Checksum("helper_text_anchor") || id == THAWHud::Checksum("dialog_box_anchor"))
                return true;
            element = *reinterpret_cast<const uintptr_t*>(element + 96);
        }
        return false;
    }

    inline uintptr_t SetTextContext(uintptr_t element)
    {
        const auto previous = TextElement;
        TextElement = element;
        if (element && THAWController::UsingPad.load(std::memory_order_relaxed))
        {
            const auto* text = *reinterpret_cast<const char* const*>(element + 388);
            bool directional = false;
            if (text)
                for (const char* p = text; *p && !directional; ++p)
                    directional = p[0] == '\\' && (p[1] == 'b' || p[1] == 'B') && p[2] >= '4' && p[2] <= '7';
            if (directional)
            {
                const bool helper = IsMenuHelper(element);
                std::lock_guard lock(TextMutex);
                if (TextContexts.size() >= 512) TextContexts.clear();
                TextContexts[text] = helper;
            }
        }
        return previous;
    }

    inline char __cdecl PreserveMeta(const char* text)
    {
        const int action = Number(text);
        return action >= 0 && action < 32 ? static_cast<char>(0x80 + action) : '0';
    }

    inline bool MenuHelper()
    {
        if (TextElement) return IsMenuHelper(TextElement);
        if (!CurrentString) return false;
        std::lock_guard lock(TextMutex);
        const auto it = TextContexts.find(CurrentString);
        return it != TextContexts.end() && it->second;
    }

    inline int Action(int token, bool meta)
    {
        constexpr int metaActions[32]{ 8,9,11,10,17,10,10,10,11,11,16,17,9,10,10,16,
                                       17,11,11,15,17,19,15,10,16,14,18,15,10,16,17,11 };
        if (meta) return token >= 0 && token < 32 ? metaActions[token] : -1;
        switch (token)
        {
        case 0: return 11; case 1: return 10; case 2: return 9; case 3: return 8;
        case 4: return MenuHelper() ? 21 : 1;
        case 5: return MenuHelper() ? 23 : 3;
        case 6: return MenuHelper() ? 22 : 2;
        case 7: return MenuHelper() ? 20 : 0;
        case 8: return 12;
        case 14: return 16; case 15: return 17; case 16: return 14;
        case 17: return 15; case 18: return 19; case 19: return 18;
        default: return -1;
        }
    }

    inline const uint32_t* Bindings()
    {
        if (!GameSelected.is_initialized()) return nullptr;
        const int player = std::clamp(THAWController::ActivePlayer.load(std::memory_order_relaxed), 0, 3);
        return reinterpret_cast<const uint32_t*>(GameSelected.get_ptr() + 1) + 77 * player;
    }

    inline int Icon(int action)
    {
        if (action < 0 || action >= 24) return -1;
        const auto* bindings = Bindings();
        if (!bindings) return -1;
        const uint32_t code = bindings[action];
        if (code >= 11 && code <= 22) return static_cast<int>(code - 11);
        if (!(action <= 7 || action >= 20)) return -1;
        const bool negative = (action % 2) == 0;
        int direction = -1;
        if (code == 1 || code == 3 || code == 4) direction = negative ? 2 : 3;
        if (code == 2 || code == 8 || code == 5) direction = negative ? 0 : 1;
        if (direction < 0) return -1;
        return (code == 4 || code == 5 ? 12 : code == 1 || code == 2 ? 16 : 20) + direction;
    }

    inline int Diagonal(int token)
    {
        const auto* bindings = Bindings();
        if (!bindings || token < 10 || token > 13) return -1;
        // Native font glyphs 10..13 are down-right, up-left, down-left, up-right.
        const bool down = token == 10 || token == 12;
        const bool right = token == 10 || token == 13;
        const uint32_t vertical = bindings[down ? 1 : 0], horizontal = bindings[right ? 3 : 2];
        auto axis = [](uint32_t code) -> int
        {
            switch (code) { case 1: return 0; case 2: return 1; case 4: return 2;
                            case 5: return 3; case 3: return 4; case 8: return 5; default: return -1; }
        };
        const int a = axis(vertical), b = axis(horizontal);
        if (a < 0 || b < 0 || a / 2 != b / 2 || a % 2 == b % 2) return -1;
        const bool xPositive = a % 2 == 0 ? down : right;
        const bool yPositive = a % 2 == 1 ? down : right;
        return 24 + (a / 2) * 4 + (xPositive ? 1 : 0) + (yPositive ? 2 : 0);
    }

    inline int Stick(int token)
    {
        const auto* bindings = Bindings();
        if (!bindings || (token != 20 && token != 21)) return -1;
        const int first = token == 20 ? 0 : 4;
        const uint32_t a = bindings[first], b = bindings[first + 2];
        if ((a == 1 || a == 2) && (b == 1 || b == 2) && a != b) return 36;
        if ((a == 3 || a == 8) && (b == 3 || b == 8) && a != b) return 37;
        if ((a == 4 || a == 5) && (b == 4 || b == 5) && a != b) return 38;
        return -1;
    }

    inline int Resolve(const char* text, bool draw)
    {
        const auto marker = static_cast<uint8_t>(*text);
        const bool meta = marker >= 0x80 && marker < 0xa0;
        const int token = meta ? marker - 0x80 : Number(text);
        const char mapped = meta ? MetaTable.get_ptr()[token] : *text;
        const int nativeGlyph = Number(&mapped);
        if (draw)
            Pending = THAWController::UsingPad.load(std::memory_order_relaxed) ?
                (!meta && token >= 10 && token <= 13 ? Diagonal(token) :
                 !meta && (token == 20 || token == 21) ? Stick(token) : Icon(Action(token, meta))) : -1;
        return nativeGlyph;
    }
    inline int __cdecl WidthButton(const char* text) { return Resolve(text, false); }
    inline int __cdecl DrawButtonWithContext(const char* text, uintptr_t renderer)
    {
        CurrentString = *reinterpret_cast<const char* const*>(renderer + 36);
        return Resolve(text, true);
    }
    __declspec(naked) inline void DrawButton()
    {
        __asm
        {
            push edi
            push dword ptr [esp + 8]
            call DrawButtonWithContext
            add esp, 8
            ret
        }
    }

    inline IDirect3DTexture9* GetAtlas(uint8_t* font, int icon)
    {
        auto* wrapper = *reinterpret_cast<uint8_t**>(font + 376);
        if (!wrapper) return nullptr;
        auto* native = *reinterpret_cast<IDirect3DTexture9**>(wrapper + 20);
        if (!native) return nullptr;
        IDirect3DDevice9* device = nullptr;
        if (FAILED(native->GetDevice(&device))) return nullptr;
        if (device != AtlasDevice)
        {
            for (auto& family : Atlas) for (auto*& texture : family)
                if (texture) { texture->Release(); texture = nullptr; }
            if (AtlasDevice) AtlasDevice->Release();
            AtlasDevice = device;
        }
        else device->Release();
        const int family = std::clamp(*Family, 0, 3);
        const int index = family == 0 ? 0 : family - 1;
        auto*& result = Atlas[index][icon];
        if (result) return result;
        D3DSURFACE_DESC desc{};
        if (FAILED(native->GetLevelDesc(0, &desc)) || !desc.Width || !desc.Height) return nullptr;
        IDirect3DTexture9* texture = nullptr;
        if (FAILED(AtlasDevice->CreateTexture(desc.Width, desc.Height, 1, 0, D3DFMT_A8R8G8B8,
                                             D3DPOOL_MANAGED, &texture, nullptr))) return nullptr;
        D3DLOCKED_RECT lock{};
        if (FAILED(texture->LockRect(0, &lock, nullptr, 0))) { texture->Release(); return nullptr; }
        for (UINT y = 0; y < desc.Height; ++y)
            std::memset(static_cast<uint8_t*>(lock.pBits) + y * lock.Pitch, 0, desc.Width * 4);
        const auto* glyphs = *reinterpret_cast<const uint8_t* const*>(font + 8);
        const auto* map = font + 268;
        for (int i = 0; i < 24; ++i)
        {
            const auto* uv = reinterpret_cast<const float*>(glyphs + 24 * map[i]);
            THAWIconArt::Rect r{ static_cast<int>(uv[0]*desc.Width), static_cast<int>(uv[1]*desc.Height),
                static_cast<int>((uv[2]-uv[0])*desc.Width+0.5f), static_cast<int>((uv[3]-uv[1])*desc.Height+0.5f) };
            if (r.x >= 0 && r.y >= 0 && r.x+r.w <= static_cast<int>(desc.Width) && r.y+r.h <= static_cast<int>(desc.Height))
                THAWIconArt::Paint(static_cast<uint32_t*>(lock.pBits), lock.Pitch/4, r, icon, index+1);
        }
        texture->UnlockRect(0);
        return result = texture;
    }

    export void Install(const int32_t* family)
    {
        Family = family;
        THAWHud::SetTextContext = SetTextContext;
        auto meta = hook::pattern("8B 44 24 04 50 E8 ? ? ? ? 8A 80 ? ? ? ? 83 C4 04 C3");
        auto widthButton = hook::pattern("46 56 E8 ? ? ? ? 8B 3D ? ? ? ? 0F B6 84 38 0C 01 00 00");
        auto widthMeta = hook::pattern("E8 ? ? ? ? 8D 4C 24 18 51 88 44 24 1C E8 ? ? ? ? 8B 3D ? ? ? ? 0F B6 84 38 0C 01 00 00");
        auto drawButton = hook::pattern("43 53 E8 ? ? ? ? 8B 0D ? ? ? ? 0F B6 84 08 0C 01 00 00");
        auto boundary = hook::pattern("8B 4C 24 30 03 F0 89 0D ? ? ? ? E9 ? ? ? ? 43 E9 ? ? ? ? 83 C3 02");
        auto texture = hook::pattern("8B 83 78 01 00 00 8B 48 14 51 33 FF 57 E8 ? ? ? ? A1 ? ? ? ? 83 C4 0C");
        Number = reinterpret_cast<decltype(Number)>(injector::GetBranchDestination(meta.get_first(5)).as_int());
        MetaTable.SetAddress(*meta.get_first<char*>(12));
        ButtonFont.SetAddress(*drawButton.get_first<uint8_t**>(9));
        MetaHook = safetyhook::create_inline(meta.get_first(), PreserveMeta);
        injector::MakeCALL(widthButton.get_first(2), WidthButton, true);
        injector::MakeCALL(widthMeta.get_first(14), WidthButton, true);
        injector::MakeCALL(drawButton.get_first(2), DrawButton, true);
        ButtonBoundary = safetyhook::create_mid(boundary.get_first(12), [](SafetyHookContext&) { Active = Pending; });
        TextureHook = safetyhook::create_mid(texture.get_first(13), [](SafetyHookContext& regs)
        {
            if (Active < 0 || !ButtonFont.is_initialized() || regs.ebx != reinterpret_cast<uintptr_t>(ButtonFont.get())) return;
            if (auto* atlas = GetAtlas(ButtonFont.get(), Active))
                reinterpret_cast<uintptr_t*>(regs.esp)[1] = reinterpret_cast<uintptr_t>(atlas);
        });
    }
}
