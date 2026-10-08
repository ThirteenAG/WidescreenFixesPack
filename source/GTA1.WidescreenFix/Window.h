#pragma once
#include <ddraw.h>

// Native windowed mode. GTA1 draws through SciTech MGL, which loads DirectDraw
// itself, takes the screen in exclusive mode and changes the display mode. Here
// MGL gets a small DirectDraw of this fix instead: surfaces are plain system
// memory, the display mode only sizes a borderless window, and every finished
// frame is presented to that window with GDI, scaled to fit when the window and
// the game differ in size. Alt+Enter switches between the window and the whole
// monitor.
namespace GTA1Window
{
    // Interface IDs, defined here instead of linking the DirectX GUID library.
    inline constexpr GUID DirectDrawId{ 0x6C14DB80, 0xA733, 0x11CE, { 0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60 } };
    inline constexpr GUID SurfaceId{ 0x6C14DB81, 0xA733, 0x11CE, { 0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60 } };
    inline constexpr GUID PaletteId{ 0x6C14DB84, 0xA733, 0x11CE, { 0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60 } };

    inline bool Enabled = true;
    inline bool Fullscreen = false;                // the window covers the monitor
    inline HWND Window = nullptr;
    inline int ModeWidth = 640, ModeHeight = 480, ModeBits = 16;
    inline RECT Placement{};                       // where the window belongs, once a mode is set

    struct Palette;
    struct Surface;
    inline void Present(Surface* surface);

    struct Palette final : IDirectDrawPalette
    {
        ULONG references = 1;
        PALETTEENTRY entries[256]{};

        STDMETHOD(QueryInterface)(REFIID id, LPVOID* out) override
        {
            if (!out) return E_POINTER;
            if (id == IID_IUnknown || id == PaletteId) { *out = this; AddRef(); return S_OK; }
            *out = nullptr; return E_NOINTERFACE;
        }
        STDMETHOD_(ULONG, AddRef)() override { return ++references; }
        STDMETHOD_(ULONG, Release)() override { ULONG left = --references; if (!left) delete this; return left; }
        STDMETHOD(GetCaps)(LPDWORD caps) override { if (caps) *caps = DDPCAPS_8BIT | DDPCAPS_ALLOW256; return DD_OK; }
        STDMETHOD(GetEntries)(DWORD, DWORD start, DWORD count, LPPALETTEENTRY out) override
        {
            if (!out || start + count > 256) return DDERR_INVALIDPARAMS;
            memcpy(out, entries + start, count * sizeof(PALETTEENTRY)); return DD_OK;
        }
        STDMETHOD(Initialize)(LPDIRECTDRAW, DWORD, LPPALETTEENTRY) override { return DDERR_ALREADYINITIALIZED; }
        STDMETHOD(SetEntries)(DWORD, DWORD start, DWORD count, LPPALETTEENTRY in) override
        {
            if (!in || start + count > 256) return DDERR_INVALIDPARAMS;
            memcpy(entries + start, in, count * sizeof(PALETTEENTRY)); return DD_OK;
        }
    };

    inline void Format(DDPIXELFORMAT& format, int bits)
    {
        format = {}; format.dwSize = sizeof(format);
        format.dwRGBBitCount = bits;
        if (bits == 8) { format.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8; return; }
        format.dwFlags = DDPF_RGB;
        if (bits == 16) { format.dwRBitMask = 0xF800; format.dwGBitMask = 0x07E0; format.dwBBitMask = 0x001F; }
        else { format.dwRBitMask = 0xFF0000; format.dwGBitMask = 0x00FF00; format.dwBBitMask = 0x0000FF; }
    }

    struct Surface final : IDirectDrawSurface
    {
        ULONG references = 1;
        int width = 0, height = 0, bits = 16, pitch = 0;
        DWORD caps = 0;
        std::vector<uint8_t> memory;
        Surface* next = nullptr;                   // the flipping chain: primary, back buffers
        Palette* palette = nullptr;

        Surface(int w, int h, int b, DWORD surfaceCaps) : width(w), height(h), bits(b), caps(surfaceCaps)
        {
            pitch = ((width * ((bits + 7) / 8)) + 3) & ~3;
            memory.assign(size_t(pitch) * height, 0);
        }
        ~Surface() { if (palette) palette->Release(); }
        bool Primary() const { return (caps & DDSCAPS_PRIMARYSURFACE) != 0; }
        int Bytes() const { return (bits + 7) / 8; }

        void Describe(DDSURFACEDESC& desc, bool withPointer)
        {
            DWORD size = desc.dwSize ? desc.dwSize : sizeof(desc);
            memset(&desc, 0, std::min<DWORD>(size, sizeof(desc)));
            desc.dwSize = size;
            desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT | (withPointer ? DDSD_LPSURFACE : 0);
            desc.dwWidth = width; desc.dwHeight = height; desc.lPitch = pitch;
            desc.lpSurface = withPointer ? memory.data() : nullptr;
            Format(desc.ddpfPixelFormat, bits);
            desc.ddsCaps.dwCaps = caps;
        }
        RECT Clip(const RECT* rect) const
        {
            RECT area{ 0, 0, width, height };
            if (!rect) return area;
            RECT result{};
            IntersectRect(&result, rect, &area);
            return result;
        }
        void Copy(const RECT& to, Surface* source, const RECT& from)
        {
            int w = to.right - to.left, h = to.bottom - to.top, sw = from.right - from.left, sh = from.bottom - from.top;
            if (w <= 0 || h <= 0 || sw <= 0 || sh <= 0 || source->bits != bits) return;
            int bytes = Bytes();
            for (int y = 0; y < h; ++y)
            {
                auto out = memory.data() + size_t(to.top + y) * pitch + size_t(to.left) * bytes;
                auto in = source->memory.data() + size_t(from.top + y * sh / h) * source->pitch + size_t(from.left) * bytes;
                if (w == sw) memmove(out, in, size_t(w) * bytes);
                else for (int x = 0; x < w; ++x) memcpy(out + size_t(x) * bytes, in + size_t(x * sw / w) * bytes, bytes);
            }
        }

        STDMETHOD(QueryInterface)(REFIID id, LPVOID* out) override
        {
            if (!out) return E_POINTER;
            if (id == IID_IUnknown || id == SurfaceId) { *out = this; AddRef(); return S_OK; }
            *out = nullptr; return E_NOINTERFACE;
        }
        STDMETHOD_(ULONG, AddRef)() override { return ++references; }
        STDMETHOD_(ULONG, Release)() override
        {
            ULONG left = --references;
            if (!left)
            {
                // The primary owns its back buffers.
                if (Primary()) for (auto back = next; back && back != this;) { auto following = back->next; back->next = nullptr; delete back; back = following; }
                delete this;
            }
            return left;
        }
        STDMETHOD(AddAttachedSurface)(LPDIRECTDRAWSURFACE) override { return DDERR_UNSUPPORTED; }
        STDMETHOD(AddOverlayDirtyRect)(LPRECT) override { return DDERR_UNSUPPORTED; }
        STDMETHOD(Blt)(LPRECT to, LPDIRECTDRAWSURFACE source, LPRECT from, DWORD flags, LPDDBLTFX effects) override
        {
            RECT target = Clip(to);
            if (flags & DDBLT_COLORFILL)
            {
                if (!effects) return DDERR_INVALIDPARAMS;
                DWORD color = effects->dwFillColor; int bytes = Bytes();
                for (LONG y = target.top; y < target.bottom; ++y)
                    for (LONG x = target.left; x < target.right; ++x)
                        memcpy(memory.data() + size_t(y) * pitch + size_t(x) * bytes, &color, bytes);
            }
            else if (source)
            {
                auto in = static_cast<Surface*>(source);
                Copy(target, in, in->Clip(from));
            }
            if (Primary()) Present(this);
            return DD_OK;
        }
        STDMETHOD(BltBatch)(LPDDBLTBATCH, DWORD, DWORD) override { return DDERR_UNSUPPORTED; }
        STDMETHOD(BltFast)(DWORD x, DWORD y, LPDIRECTDRAWSURFACE source, LPRECT from, DWORD) override
        {
            if (!source) return DDERR_INVALIDPARAMS;
            auto in = static_cast<Surface*>(source);
            RECT area = in->Clip(from);
            RECT to{ LONG(x), LONG(y), LONG(x) + area.right - area.left, LONG(y) + area.bottom - area.top };
            RECT target = Clip(&to);
            area.right = area.left + (target.right - target.left); area.bottom = area.top + (target.bottom - target.top);
            Copy(target, in, area);
            if (Primary()) Present(this);
            return DD_OK;
        }
        STDMETHOD(DeleteAttachedSurface)(DWORD, LPDIRECTDRAWSURFACE) override { return DDERR_UNSUPPORTED; }
        STDMETHOD(EnumAttachedSurfaces)(LPVOID context, LPDDENUMSURFACESCALLBACK callback) override
        {
            if (!callback) return DDERR_INVALIDPARAMS;
            if (next)
            {
                DDSURFACEDESC desc{ sizeof(desc) };
                next->Describe(desc, false);
                next->AddRef();
                callback(next, &desc, context);
            }
            return DD_OK;
        }
        STDMETHOD(EnumOverlayZOrders)(DWORD, LPVOID, LPDDENUMSURFACESCALLBACK) override { return DDERR_UNSUPPORTED; }
        STDMETHOD(Flip)(LPDIRECTDRAWSURFACE target, DWORD) override
        {
            if (!Primary() || !next) return DDERR_NOTFLIPPABLE;
            // The memory moves, not the surfaces: what was drawn becomes the front.
            auto front = target ? static_cast<Surface*>(target) : next;
            std::swap(memory, front->memory);
            Present(this);
            return DD_OK;
        }
        STDMETHOD(GetAttachedSurface)(LPDDSCAPS wanted, LPDIRECTDRAWSURFACE* out) override
        {
            if (!out) return DDERR_INVALIDPARAMS;
            if (!next || next == this) { *out = nullptr; return DDERR_NOTFOUND; }
            (void)wanted;
            next->AddRef(); *out = next; return DD_OK;
        }
        STDMETHOD(GetBltStatus)(DWORD) override { return DD_OK; }
        STDMETHOD(GetCaps)(LPDDSCAPS out) override { if (!out) return DDERR_INVALIDPARAMS; out->dwCaps = caps; return DD_OK; }
        STDMETHOD(GetClipper)(LPDIRECTDRAWCLIPPER*) override { return DDERR_NOCLIPPERATTACHED; }
        STDMETHOD(GetColorKey)(DWORD, LPDDCOLORKEY) override { return DDERR_NOCOLORKEY; }
        STDMETHOD(GetDC)(HDC*) override { return DDERR_UNSUPPORTED; }
        STDMETHOD(GetFlipStatus)(DWORD) override { return DD_OK; }
        STDMETHOD(GetOverlayPosition)(LPLONG, LPLONG) override { return DDERR_NOTAOVERLAYSURFACE; }
        STDMETHOD(GetPalette)(LPDIRECTDRAWPALETTE* out) override
        {
            if (!out) return DDERR_INVALIDPARAMS;
            if (!palette) { *out = nullptr; return DDERR_NOPALETTEATTACHED; }
            palette->AddRef(); *out = palette; return DD_OK;
        }
        STDMETHOD(GetPixelFormat)(LPDDPIXELFORMAT out) override { if (!out) return DDERR_INVALIDPARAMS; Format(*out, bits); return DD_OK; }
        STDMETHOD(GetSurfaceDesc)(LPDDSURFACEDESC out) override { if (!out) return DDERR_INVALIDPARAMS; Describe(*out, false); return DD_OK; }
        STDMETHOD(Initialize)(LPDIRECTDRAW, LPDDSURFACEDESC) override { return DDERR_ALREADYINITIALIZED; }
        STDMETHOD(IsLost)() override { return DD_OK; }
        STDMETHOD(Lock)(LPRECT rect, LPDDSURFACEDESC out, DWORD, HANDLE) override
        {
            if (!out) return DDERR_INVALIDPARAMS;
            Describe(*out, true);
            if (rect) out->lpSurface = memory.data() + size_t(rect->top) * pitch + size_t(rect->left) * Bytes();
            return DD_OK;
        }
        STDMETHOD(ReleaseDC)(HDC) override { return DDERR_UNSUPPORTED; }
        STDMETHOD(Restore)() override { return DD_OK; }
        STDMETHOD(SetClipper)(LPDIRECTDRAWCLIPPER) override { return DD_OK; }
        STDMETHOD(SetColorKey)(DWORD, LPDDCOLORKEY) override { return DD_OK; }
        STDMETHOD(SetOverlayPosition)(LONG, LONG) override { return DDERR_NOTAOVERLAYSURFACE; }
        STDMETHOD(SetPalette)(LPDIRECTDRAWPALETTE in) override
        {
            auto value = static_cast<Palette*>(in);
            if (value) value->AddRef();
            if (palette) palette->Release();
            palette = value;
            return DD_OK;
        }
        STDMETHOD(Unlock)(LPVOID) override { if (Primary()) Present(this); return DD_OK; }
        STDMETHOD(UpdateOverlay)(LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDOVERLAYFX) override { return DDERR_NOTAOVERLAYSURFACE; }
        STDMETHOD(UpdateOverlayDisplay)(DWORD) override { return DDERR_NOTAOVERLAYSURFACE; }
        STDMETHOD(UpdateOverlayZOrder)(DWORD, LPDIRECTDRAWSURFACE) override { return DDERR_NOTAOVERLAYSURFACE; }
    };

    // The window ----------------------------------------------------------------
    inline RECT MonitorArea(HWND window)
    {
        MONITORINFO info{ sizeof(info) };
        if (GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &info)) return info.rcMonitor;
        return { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
    }
    // A borderless window the size of the game, centred; at the monitor's size or
    // larger, or in fullscreen, it covers the monitor.
    inline void Place()
    {
        if (!Window) return;
        RECT area = MonitorArea(Window);
        int mw = area.right - area.left, mh = area.bottom - area.top;
        bool cover = Fullscreen || (ModeWidth >= mw && ModeHeight >= mh);
        int w = cover ? mw : std::min(ModeWidth, mw), h = cover ? mh : std::min(ModeHeight, mh);
        SetWindowLongW(Window, GWL_STYLE, (GetWindowLongW(Window, GWL_STYLE) & ~(WS_CAPTION | WS_THICKFRAME)) | WS_POPUP | WS_VISIBLE);
        SetWindowLongW(Window, GWL_EXSTYLE, GetWindowLongW(Window, GWL_EXSTYLE) & ~WS_EX_TOPMOST);
        Placement = { area.left + (mw - w) / 2, area.top + (mh - h) / 2, area.left + (mw - w) / 2 + w, area.top + (mh - h) / 2 + h };
        SetWindowPos(Window, HWND_NOTOPMOST, Placement.left, Placement.top, w, h, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
    // MGL places its window itself, at the top left corner and topmost: keep it where it belongs.
    inline BOOL WINAPI PositionWindow(HWND window, HWND after, int x, int y, int width, int height, UINT flags)
    {
        if (Enabled && window && window == Window && Placement.right > Placement.left)
        {
            if (after == HWND_TOPMOST) after = HWND_NOTOPMOST;
            x = Placement.left; y = Placement.top;
            width = Placement.right - Placement.left; height = Placement.bottom - Placement.top;
            if (!(flags & SWP_NOMOVE) || !(flags & SWP_NOSIZE)) flags &= ~(SWP_NOMOVE | SWP_NOSIZE);
        }
        return SetWindowPos(window, after, x, y, width, height, flags);
    }
    inline BOOL WINAPI MoveGameWindow(HWND window, int x, int y, int width, int height, BOOL repaint)
    {
        if (Enabled && window && window == Window && Placement.right > Placement.left)
            return SetWindowPos(window, HWND_NOTOPMOST, Placement.left, Placement.top, Placement.right - Placement.left, Placement.bottom - Placement.top, repaint ? 0 : SWP_NOREDRAW);
        return MoveWindow(window, x, y, width, height, repaint);
    }
    inline void ToggleFullscreen() { Fullscreen = !Fullscreen; Place(); }

    inline void Present(Surface* surface)
    {
        if (!Window || !surface || surface->memory.empty()) return;
        RECT client{};
        if (!GetClientRect(Window, &client) || client.right <= 0 || client.bottom <= 0) return;
        struct { BITMAPINFOHEADER header; DWORD colors[256]; } info{};
        info.header.biSize = sizeof(info.header);
        info.header.biWidth = surface->pitch / surface->Bytes();
        info.header.biHeight = -surface->height;
        info.header.biPlanes = 1;
        info.header.biBitCount = WORD(surface->bits);
        if (surface->bits == 16)
        {
            info.header.biCompression = BI_BITFIELDS;
            info.colors[0] = 0xF800; info.colors[1] = 0x07E0; info.colors[2] = 0x001F;
        }
        else if (surface->bits == 8 && surface->palette)
        {
            info.header.biClrUsed = 256;
            for (int i = 0; i < 256; ++i)
            {
                auto& entry = surface->palette->entries[i];
                info.colors[i] = (DWORD(entry.peRed) << 16) | (DWORD(entry.peGreen) << 8) | entry.peBlue;
            }
        }
        // Fit the frame, keeping its shape.
        double scale = std::min(double(client.right) / surface->width, double(client.bottom) / surface->height);
        int w = std::max(1, int(std::lround(surface->width * scale))), h = std::max(1, int(std::lround(surface->height * scale)));
        int x = (client.right - w) / 2, y = (client.bottom - h) / 2;
        HDC dc = GetDC(Window);
        if (!dc) return;
        if (w != client.right || h != client.bottom)
        {
            auto black = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
            RECT bars[] = { { 0, 0, client.right, y }, { 0, y + h, client.right, client.bottom }, { 0, y, x, y + h }, { x + w, y, client.right, y + h } };
            for (auto& bar : bars) if (bar.right > bar.left && bar.bottom > bar.top) FillRect(dc, &bar, black);
        }
        SetStretchBltMode(dc, COLORONCOLOR);
        StretchDIBits(dc, x, y, w, h, 0, 0, surface->width, surface->height, surface->memory.data(),
            reinterpret_cast<BITMAPINFO*>(&info), DIB_RGB_COLORS, SRCCOPY);
        ReleaseDC(Window, dc);
    }

    // DirectDraw ------------------------------------------------------------------
    struct Display final : IDirectDraw
    {
        ULONG references = 1;

        STDMETHOD(QueryInterface)(REFIID id, LPVOID* out) override
        {
            if (!out) return E_POINTER;
            if (id == IID_IUnknown || id == DirectDrawId) { *out = this; AddRef(); return S_OK; }
            *out = nullptr; return E_NOINTERFACE;
        }
        STDMETHOD_(ULONG, AddRef)() override { return ++references; }
        STDMETHOD_(ULONG, Release)() override { ULONG left = --references; if (!left) delete this; return left; }
        STDMETHOD(Compact)() override { return DD_OK; }
        STDMETHOD(CreateClipper)(DWORD, LPDIRECTDRAWCLIPPER*, IUnknown*) override { return DDERR_UNSUPPORTED; }
        STDMETHOD(CreatePalette)(DWORD, LPPALETTEENTRY entries, LPDIRECTDRAWPALETTE* out, IUnknown*) override
        {
            if (!out) return DDERR_INVALIDPARAMS;
            auto palette = new Palette();
            if (entries) memcpy(palette->entries, entries, sizeof(palette->entries));
            *out = palette; return DD_OK;
        }
        STDMETHOD(CreateSurface)(LPDDSURFACEDESC desc, LPDIRECTDRAWSURFACE* out, IUnknown*) override
        {
            if (!desc || !out) return DDERR_INVALIDPARAMS;
            DWORD caps = (desc->dwFlags & DDSD_CAPS) ? desc->ddsCaps.dwCaps : 0;
            if (caps & DDSCAPS_PRIMARYSURFACE)
            {
                int count = (desc->dwFlags & DDSD_BACKBUFFERCOUNT) ? int(desc->dwBackBufferCount) : 0;
                auto primary = new Surface(ModeWidth, ModeHeight, ModeBits, (caps & ~DDSCAPS_VIDEOMEMORY) | DDSCAPS_FRONTBUFFER | DDSCAPS_VISIBLE);
                Surface* last = primary;
                for (int i = 0; i < count; ++i)
                {
                    auto back = new Surface(ModeWidth, ModeHeight, ModeBits, i == 0 ? DDSCAPS_BACKBUFFER | DDSCAPS_FLIP | DDSCAPS_COMPLEX : DDSCAPS_FLIP | DDSCAPS_COMPLEX);
                    last->next = back; last = back;
                }
                if (count) last->next = primary;
                *out = primary; return DD_OK;
            }
            if (!(desc->dwFlags & DDSD_WIDTH) || !(desc->dwFlags & DDSD_HEIGHT)) return DDERR_INVALIDPARAMS;
            int bits = (desc->dwFlags & DDSD_PIXELFORMAT) ? int(desc->ddpfPixelFormat.dwRGBBitCount) : ModeBits;
            *out = new Surface(int(desc->dwWidth), int(desc->dwHeight), bits ? bits : ModeBits, caps | DDSCAPS_SYSTEMMEMORY);
            return DD_OK;
        }
        STDMETHOD(DuplicateSurface)(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE*) override { return DDERR_UNSUPPORTED; }
        STDMETHOD(EnumDisplayModes)(DWORD, LPDDSURFACEDESC, LPVOID context, LPDDENUMMODESCALLBACK callback) override
        {
            if (!callback) return DDERR_INVALIDPARAMS;
            // The classic modes MGL knows; the fix replaces their sizes with modern ones.
            const std::pair<int, int> sizes[] = { {320, 200}, {320, 240}, {400, 300}, {512, 384}, {640, 400}, {640, 480},
                {800, 600}, {1024, 768}, {1152, 864}, {1280, 960}, {1280, 1024}, {1600, 1200} };
            for (int bits : { 8, 16, 32 })
                for (auto [w, h] : sizes)
                {
                    DDSURFACEDESC desc{ sizeof(desc) };
                    desc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_REFRESHRATE;
                    desc.dwWidth = w; desc.dwHeight = h; desc.lPitch = w * bits / 8; desc.dwRefreshRate = 60;
                    Format(desc.ddpfPixelFormat, bits);
                    if (callback(&desc, context) == DDENUMRET_CANCEL) return DD_OK;
                }
            return DD_OK;
        }
        STDMETHOD(EnumSurfaces)(DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMSURFACESCALLBACK) override { return DDERR_UNSUPPORTED; }
        STDMETHOD(FlipToGDISurface)() override { return DD_OK; }
        STDMETHOD(GetCaps)(LPDDCAPS driver, LPDDCAPS emulation) override
        {
            for (auto caps : { driver, emulation })
            {
                if (!caps) continue;
                DWORD size = caps->dwSize ? caps->dwSize : sizeof(DDCAPS_DX5);
                DDCAPS_DX5 values{};
                values.dwSize = size;
                values.dwCaps = DDCAPS_BLT | DDCAPS_BLTCOLORFILL | DDCAPS_BLTSTRETCH | DDCAPS_PALETTE;
                values.dwPalCaps = DDPCAPS_8BIT | DDPCAPS_ALLOW256;
                values.dwVidMemTotal = values.dwVidMemFree = 256u << 20;
                values.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_BACKBUFFER | DDSCAPS_FRONTBUFFER | DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
                memcpy(caps, &values, std::min<DWORD>(size, sizeof(values)));
            }
            return DD_OK;
        }
        STDMETHOD(GetDisplayMode)(LPDDSURFACEDESC out) override
        {
            if (!out) return DDERR_INVALIDPARAMS;
            Surface mode(ModeWidth, 1, ModeBits, 0);
            mode.height = ModeHeight;
            mode.Describe(*out, false);
            return DD_OK;
        }
        STDMETHOD(GetFourCCCodes)(LPDWORD count, LPDWORD) override { if (count) *count = 0; return DD_OK; }
        STDMETHOD(GetGDISurface)(LPDIRECTDRAWSURFACE*) override { return DDERR_UNSUPPORTED; }
        STDMETHOD(GetMonitorFrequency)(LPDWORD out) override { if (out) *out = 60; return DD_OK; }
        STDMETHOD(GetScanLine)(LPDWORD out) override { if (out) *out = 0; return DD_OK; }
        STDMETHOD(GetVerticalBlankStatus)(LPBOOL out) override { if (out) *out = TRUE; return DD_OK; }
        STDMETHOD(Initialize)(GUID*) override { return DDERR_ALREADYINITIALIZED; }
        STDMETHOD(RestoreDisplayMode)() override { return DD_OK; }
        STDMETHOD(SetCooperativeLevel)(HWND window, DWORD) override
        {
            if (window) Window = window;
            return DD_OK;
        }
        STDMETHOD(SetDisplayMode)(DWORD width, DWORD height, DWORD bits) override
        {
            if (!width || !height) return DDERR_INVALIDMODE;
            ModeWidth = int(width); ModeHeight = int(height); ModeBits = bits == 8 || bits == 32 ? int(bits) : 16;
            Place();
            return DD_OK;
        }
        STDMETHOD(WaitForVerticalBlank)(DWORD, HANDLE) override { return DD_OK; }
    };

    inline HRESULT WINAPI Create(GUID*, LPDIRECTDRAW* out, IUnknown*)
    {
        if (!out) return DDERR_INVALIDPARAMS;
        *out = new Display();
        return DD_OK;
    }
    inline HRESULT WINAPI CreateClipper(DWORD, LPDIRECTDRAWCLIPPER* out, IUnknown*)
    {
        if (out) *out = nullptr;
        return DDERR_UNSUPPORTED;
    }
    inline bool KeyDown(UINT message, WPARAM key, LPARAM flags)
    {
        if (!Enabled || !Window) return false;
        if (message == WM_SYSKEYDOWN && key == VK_RETURN && (flags & (1 << 29)) && !(flags & (1 << 30)))
        {
            ToggleFullscreen();
            CIniReader ini("");
            ini.WriteString("MAIN", "FullscreenWindow", Fullscreen ? "1" : "0");
            return true;
        }
        return false;
    }

    inline void Install()
    {
        CIniReader ini("");
        Enabled = ini.ReadBoolean("MAIN", "WindowedMode", true);
        Fullscreen = ini.ReadBoolean("MAIN", "FullscreenWindow", false);
        if (!Enabled) return;
        SetProcessDPIAware();
        // MGL looks DirectDraw up when it starts its display driver: give it this one.
        auto lookup = hook::pattern("FF D6 68 ? ? ? ? 8B 0D ? ? ? ? 51 A3 ? ? ? ? FF D6 33 F6 8B 0D ? ? ? ? A3");
        if (lookup.size() != 1) { Log::Write("GTA1 windowed mode signature unavailable."); Enabled = false; return; }
        static SafetyHookMid create = safetyhook::create_mid(lookup.get_first(14), [](SafetyHookContext& context) { context.eax = reinterpret_cast<uintptr_t>(&Create); });
        static SafetyHookMid clipper = safetyhook::create_mid(lookup.get_first(29), [](SafetyHookContext& context) { context.eax = reinterpret_cast<uintptr_t>(&CreateClipper); });
        IATHook::Replace(GetModuleHandleW(nullptr), "user32.dll", std::make_tuple("SetWindowPos", PositionWindow), std::make_tuple("MoveWindow", MoveGameWindow));
        Log::Write("GTA1 native windowed mode installed.");
    }
}
