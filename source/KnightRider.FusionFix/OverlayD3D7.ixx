module;
#include <stdafx.h>
#include "overlay/Overlay.hpp"
#include <ddraw.h>
#include <d3d.h>

export module OverlayD3D7;

#pragma comment(lib, "dxguid.lib")

export namespace KnightRiderOverlay
{
    class D3D7Overlay : public asi_overlay::Overlay
    {
        IDirect3DDevice7* Device = nullptr;
        IDirectDraw7* DirectDraw = nullptr;
        struct Vertex
        {
            float X, Y, Z, RHW;
            D3DCOLOR Color;
            float U, V;
        };
        std::vector<Vertex> Triangles;
        struct CharacterEvent
        {
            wchar_t Character;
            bool Native;
            ULONGLONG Time;
        };
        std::vector<CharacterEvent> Characters;

        bool Character(wchar_t character, bool native)
        {
            const auto now = GetTickCount64();
            std::erase_if(Characters, [now](const auto& event) { return now - event.Time > 100; });
            const auto duplicate = std::find_if(Characters.begin(), Characters.end(), [&](const auto& event)
            {
                return event.Character == character && event.Native != native;
            });
            if (duplicate != Characters.end())
            {
                Characters.erase(duplicate);
                return true;
            }
            Characters.push_back({ character, native, now });
            return false;
        }

        static D3DCOLOR Color(ImU32 color)
        {
            return (color & 0xFF00FF00) | ((color & 0xFF) << 16) | ((color >> 16) & 0xFF);
        }

        void Texture(ImTextureData* texture)
        {
            if (texture->Status == ImTextureStatus_WantDestroy)
            {
                if (texture->TexID != ImTextureID_Invalid)
                    reinterpret_cast<IDirectDrawSurface7*>(static_cast<uintptr_t>(texture->TexID))->Release();
                texture->SetTexID(ImTextureID_Invalid);
                texture->SetStatus(ImTextureStatus_Destroyed);
                return;
            }
            auto surface = texture->TexID == ImTextureID_Invalid ? nullptr
                : reinterpret_cast<IDirectDrawSurface7*>(static_cast<uintptr_t>(texture->TexID));
            bool upload = texture->Status != ImTextureStatus_OK;
            if (surface && surface->IsLost() == DDERR_SURFACELOST)
            {
                if (FAILED(surface->Restore()))
                    return;
                upload = true;
            }
            if (!surface)
            {
                DDSURFACEDESC2 desc{ sizeof(desc) };
                desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
                desc.dwWidth = texture->Width;
                desc.dwHeight = texture->Height;
                desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
                desc.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
                desc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
                desc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
                desc.ddpfPixelFormat.dwRGBBitCount = 32;
                desc.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
                desc.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
                desc.ddpfPixelFormat.dwBBitMask = 0x000000FF;
                desc.ddpfPixelFormat.dwRGBAlphaBitMask = 0xFF000000;
                if (FAILED(DirectDraw->CreateSurface(&desc, &surface, nullptr)))
                    return;
                texture->SetTexID(static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(surface)));
                upload = true;
            }
            if (upload)
            {
                DDSURFACEDESC2 locked{ sizeof(locked) };
                if (FAILED(surface->Lock(nullptr, &locked, DDLOCK_WAIT | DDLOCK_WRITEONLY, nullptr)))
                    return;
                for (int y = 0; y < texture->Height; ++y)
                {
                    auto destination = reinterpret_cast<D3DCOLOR*>(static_cast<uint8_t*>(locked.lpSurface) + y * locked.lPitch);
                    for (int x = 0; x < texture->Width; ++x)
                        destination[x] = Color(*reinterpret_cast<const ImU32*>(texture->GetPixelsAt(x, y)));
                }
                surface->Unlock(nullptr);
                texture->SetStatus(ImTextureStatus_OK);
            }
        }

        static Vertex Interpolate(const Vertex& a, const Vertex& b, float t)
        {
            Vertex v{ a.X + (b.X - a.X) * t, a.Y + (b.Y - a.Y) * t, 0.0f, 1.0f, 0,
                a.U + (b.U - a.U) * t, a.V + (b.V - a.V) * t };
            for (unsigned shift = 0; shift < 32; shift += 8)
            {
                const float first = static_cast<float>((a.Color >> shift) & 255);
                const float second = static_cast<float>((b.Color >> shift) & 255);
                v.Color |= static_cast<DWORD>(std::clamp(std::lround(first + (second - first) * t), 0l, 255l)) << shift;
            }
            return v;
        }

        void Triangle(const Vertex& a, const Vertex& b, const Vertex& c, const ImVec4& clip)
        {
            std::array<Vertex, 12> polygon{ a, b, c }, output;
            size_t count = 3;
            // Direct3D 7 has no scissor rectangle: clip the UI triangles, including their UVs and colours.
            for (int edge = 0; edge < 4 && count; ++edge)
            {
                const bool horizontal = edge == 0 || edge == 2;
                const float boundary = edge == 0 ? clip.x : edge == 1 ? clip.y : edge == 2 ? clip.z : clip.w;
                const auto coordinate = [horizontal](const Vertex& v) { return horizontal ? v.X : v.Y; };
                const auto inside = [&](const Vertex& v) { return edge < 2 ? coordinate(v) >= boundary : coordinate(v) <= boundary; };
                size_t written = 0;
                auto previous = polygon[count - 1];
                for (size_t i = 0; i < count; ++i)
                {
                    const auto current = polygon[i];
                    if (inside(previous) != inside(current))
                        output[written++] = Interpolate(previous, current,
                            (boundary - coordinate(previous)) / (coordinate(current) - coordinate(previous)));
                    if (inside(current))
                        output[written++] = current;
                    previous = current;
                }
                std::copy_n(output.begin(), written, polygon.begin());
                count = written;
            }
            for (size_t i = 2; i < count; ++i)
            {
                Triangles.push_back(polygon[0]);
                Triangles.push_back(polygon[i - 1]);
                Triangles.push_back(polygon[i]);
            }
        }

        void Render(ImDrawData* data, DWORD width, DWORD height)
        {
            if (data->Textures)
                for (auto texture : *data->Textures)
                    Texture(texture);
            D3DVIEWPORT7 viewport{ 0, 0, width, height, 0.0f, 1.0f };
            Device->SetViewport(&viewport);
            Device->SetRenderState(D3DRENDERSTATE_ZENABLE, FALSE);
            Device->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
            Device->SetRenderState(D3DRENDERSTATE_LIGHTING, FALSE);
            Device->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);
            Device->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, FALSE);
            Device->SetRenderState(D3DRENDERSTATE_STENCILENABLE, FALSE);
            Device->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
            Device->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
            Device->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
            Device->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
            Device->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
            Device->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);
            Device->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
            Device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
            Device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            Device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            Device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
            Device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            Device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
            Device->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
            Device->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
            Device->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_NONE);
            Device->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
            Device->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
            Device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
            Device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
            const float scaleX = width / data->DisplaySize.x, scaleY = height / data->DisplaySize.y;
            for (const auto list : data->CmdLists)
                for (const auto& command : list->CmdBuffer)
                {
                    if (command.UserCallback)
                    {
                        if (command.UserCallback != ImDrawCallback_ResetRenderState)
                            command.UserCallback(list, &command);
                        continue;
                    }
                    const ImVec4 clip{ (command.ClipRect.x - data->DisplayPos.x) * scaleX - 0.5f,
                        (command.ClipRect.y - data->DisplayPos.y) * scaleY - 0.5f,
                        (command.ClipRect.z - data->DisplayPos.x) * scaleX - 0.5f,
                        (command.ClipRect.w - data->DisplayPos.y) * scaleY - 0.5f };
                    const auto vertex = [&](unsigned index)
                    {
                        const auto& v = list->VtxBuffer[index + command.VtxOffset];
                        return Vertex{ (v.pos.x - data->DisplayPos.x) * scaleX - 0.5f,
                            (v.pos.y - data->DisplayPos.y) * scaleY - 0.5f, 0.0f, 1.0f, Color(v.col), v.uv.x, v.uv.y };
                    };
                    Triangles.clear();
                    for (unsigned i = 0; i + 2 < command.ElemCount; i += 3)
                    {
                        const auto indices = list->IdxBuffer.Data + command.IdxOffset + i;
                        Triangle(vertex(indices[0]), vertex(indices[1]), vertex(indices[2]), clip);
                    }
                    Device->SetTexture(0, reinterpret_cast<IDirectDrawSurface7*>(static_cast<uintptr_t>(command.GetTexID())));
                    if (!Triangles.empty())
                    {
                        Device->DrawPrimitive(D3DPT_TRIANGLELIST, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1,
                            Triangles.data(), static_cast<DWORD>(Triangles.size()), 0);
                    }
                }
        }

    public:
        bool CharacterMessage(wchar_t character)
        {
            return IsOpen() && Character(character, false);
        }

        void Keyboard(const uint8_t* keys, const uint8_t* previous)
        {
            if (!context || !IsOpen())
                return;
            Scope scope(*this);
            auto& io = ImGui::GetIO();
            BYTE state[256]{};
            const auto layout = GetKeyboardLayout(0);
            for (UINT scan = 0; scan < 256; ++scan)
            {
                const auto code = (scan & 0x7F) | (scan & 0x80 ? 0xE000 : 0);
                state[MapVirtualKeyExW(code, MAPVK_VSC_TO_VK_EX, layout) & 255] = keys[scan];
            }
            state[VK_SHIFT] = (keys[0x2A] || keys[0x36]) ? 0x80 : 0;
            state[VK_CONTROL] = (keys[0x1D] || keys[0x9D]) ? 0x80 : 0;
            state[VK_MENU] = (keys[0x38] || keys[0xB8]) ? 0x80 : 0;
            state[VK_CAPITAL] = static_cast<BYTE>(GetKeyState(VK_CAPITAL) & 1);
            for (UINT scan = 0; scan < 256; ++scan)
            {
                if (scan == 0x44 || (keys[scan] & 0x80) == (previous[scan] & 0x80))
                    continue;
                const auto code = (scan & 0x7F) | (scan & 0x80 ? 0xE000 : 0);
                const auto key = MapVirtualKeyExW(code, MAPVK_VSC_TO_VK_EX, layout);
                const bool down = (keys[scan] & 0x80) != 0;
                const auto parameter = LPARAM(1 | ((scan & 0x7F) << 16) | (scan & 0x80 ? 1 << 24 : 0))
                    | (down ? 0 : LPARAM(3) << 30);
                ImGui_ImplWin32_WndProcHandler(window, down ? WM_KEYDOWN : WM_KEYUP, key, parameter);
                if (down && !state[VK_CONTROL] && !state[VK_MENU])
                {
                    WCHAR characters[8]{};
                    const auto count = ToUnicodeEx(key, code, state, characters, 8, 4, layout);
                    for (int i = 0; i < count; ++i)
                        if (!Character(characters[i], true))
                            io.AddInputCharacterUTF16(characters[i]);
                }
            }
            // Preserve native modifiers even when DirectInput suppresses legacy key messages.
            io.AddKeyEvent(ImGuiMod_Ctrl, keys[0x1D] || keys[0x9D]);
            io.AddKeyEvent(ImGuiMod_Shift, keys[0x2A] || keys[0x36]);
            io.AddKeyEvent(ImGuiMod_Alt, keys[0x38] || keys[0xB8]);
        }

        bool Available() const { return Device && DirectDraw; }

        void ShutdownRenderer()
        {
            if (context)
            {
                Scope scope(*this);
                for (auto texture : ImGui::GetPlatformIO().Textures)
                {
                    texture->SetStatus(ImTextureStatus_WantDestroy);
                    Texture(texture);
                }
                if (scope.previous == context)
                    scope.previous = nullptr;
                Shutdown();
            }
            if (DirectDraw)
                DirectDraw->Release();
            DirectDraw = nullptr;
            Device = nullptr;
        }

        void Present(IDirect3DDevice7* device, HWND window, HMODULE module)
        {
            if (!device || !window)
                return;
            if (device != Device)
            {
                ShutdownRenderer();
                if (!Initialize(window, module))
                    return;
                Device = device;
                IDirectDrawSurface7* target = nullptr;
                if (FAILED(device->GetRenderTarget(&target)))
                    return;
                IUnknown* parent = nullptr;
                const auto owner = target->GetDDInterface(reinterpret_cast<void**>(&parent));
                target->Release();
                if (FAILED(owner) || !parent)
                    return;
                const auto result = parent->QueryInterface(IID_IDirectDraw7, reinterpret_cast<void**>(&DirectDraw));
                parent->Release();
                if (FAILED(result))
                    return;
                Scope scope(*this);
                auto& io = ImGui::GetIO();
                io.BackendRendererName = "KnightRider_Direct3D7";
                io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures;
            }
            if (!DirectDraw || !IsOpen())
                return;
            Scope scope(*this);
            IDirectDrawSurface7* target = nullptr;
            if (FAILED(device->GetRenderTarget(&target)))
                return;
            DDSURFACEDESC2 desc{ sizeof(desc) };
            const auto result = target->GetSurfaceDesc(&desc);
            target->Release();
            if (FAILED(result) || !desc.dwWidth || !desc.dwHeight)
                return;
            DWORD state = 0;
            D3DVIEWPORT7 previousViewport{};
            device->GetViewport(&previousViewport);
            if (FAILED(device->CreateStateBlock(D3DSBT_ALL, &state)))
                return;
            device->CaptureStateBlock(state);
            const bool scene = SUCCEEDED(device->BeginScene());
            NewFrame(static_cast<float>(desc.dwWidth), static_cast<float>(desc.dwHeight), true);
            Render(ImGui::GetDrawData(), desc.dwWidth, desc.dwHeight);
            if (scene)
                device->EndScene();
            device->ApplyStateBlock(state);
            device->DeleteStateBlock(state);
            device->SetViewport(&previousViewport);
        }
    };
}
