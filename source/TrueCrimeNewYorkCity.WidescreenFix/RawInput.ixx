module;

#include <stdafx.h>
#include <xmmintrin.h>
#include <cmath>

export module RawInput;

import ComVars;

export float fRawInputMouse = 0.0f;

bool* bPause = nullptr;
bool* bCutscene = nullptr;
uint32_t* nLoading = nullptr;
float* fMouseSens = nullptr;
bool bIsAimingModeSwitch = false;
bool bNonMouseCameraMovement = false;
uint8_t* byte_850D8A = nullptr;

SafetyHookInline shsub_41D3C0 = {};
int __fastcall sub_41D3C0(int8_t* _this, void* edx, int a2, void* a3)
{
    static int8_t PreviousCameraMode = -1;
    PreviousCameraMode = CurrentCameraMode;
    CurrentCameraMode = _this[145];

    if (PreviousCameraMode != CurrentCameraMode)
    {
        bIsAimingModeSwitch = ((PreviousCameraMode == CameraPrecisionAim && (CurrentCameraMode == CameraCrosshair || CurrentCameraMode == CameraOnFoot)) ||
                                 ((PreviousCameraMode == CameraCrosshair || PreviousCameraMode == CameraOnFoot) && CurrentCameraMode == CameraPrecisionAim));
    }

    return shsub_41D3C0.unsafe_fastcall<int>(_this, edx, a2, a3);
}

HWND hGameWindow = NULL;
SafetyHookInline shsub_653250 = {};
LRESULT __fastcall sub_653250(void* _this, void* edx, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    static bool bOnce = false;
    if (!bOnce)
    {
        hGameWindow = hWnd;
        RawCursorHandler<float>::Initialize(hGameWindow, fRawInputMouse * *fMouseSens / 1000.0f);
        bOnce = true;
    }
    return shsub_653250.unsafe_fastcall<LRESULT>(_this, edx, hWnd, Msg, wParam, lParam);
}

__m128* xmmword_778BF0 = nullptr;
__m128* xmmword_778BD0 = nullptr;
__m128* xmmword_778BA0 = nullptr;
float* float_70F390 = nullptr;
uint8_t** dword_79430C = nullptr;
static float idle_timer = 0.0f;

SafetyHookInline shsub_40D990 = {};
void __stdcall sub_40D990(float a1)
{
    static float prev_idle_timer = 0.0f;
    static float accumulated_yaw = 0.0f;
    static float accumulated_pitch = 0.0f;
    static int8_t prev_camera_mode_local = -1;

    float delta_x = RawCursorHandler<float>::MouseDeltaX;
    float delta_y = RawCursorHandler<float>::MouseDeltaY;

    if ((*dword_79430C)[4111])
        delta_x = -delta_x;

    if ((*dword_79430C)[4110])
        delta_y = -delta_y;

    // Reset deltas for next frame
    RawCursorHandler<float>::MouseDeltaX = 0.0f;
    RawCursorHandler<float>::MouseDeltaY = 0.0f;

    bool mouse_moved = (std::fabs(delta_x) > 1e-6f || std::fabs(delta_y) > 1e-6f);

    // Reset idle_timer if camera mode changed
    if (CurrentCameraMode != prev_camera_mode_local)
    {
        idle_timer = 10.0f;  // Set to a value >= 5.0f to force use of original code
        mouse_moved = false;

        if (bIsAimingModeSwitch)
            idle_timer = 0.0f;

        prev_camera_mode_local = CurrentCameraMode;
    }

    if (mouse_moved || CurrentCameraMode == CameraPrecisionAim)
    {
        idle_timer = 0.0f;
    }
    else
    {
        idle_timer += a1;
    }

    bool use_new_code = (idle_timer < 5.0f || mouse_moved);
    bool switching_to_new = (prev_idle_timer >= 5.0f && idle_timer < 5.0f);
    prev_idle_timer = idle_timer;

    if (*nLoading != 0 || *bCutscene || *bPause || bNonMouseCameraMovement)
    {
        shsub_40D990.unsafe_stdcall(a1);
    }
    else
    {
        if (use_new_code)
        {
            if (switching_to_new)
            {
                // Extract current quaternion from memory (assumed to be the normalized quaternion at this address)
                __m128 current_quat = *xmmword_778BF0;
                float x_f = current_quat.m128_f32[0];
                float y_f = current_quat.m128_f32[1];
                float z_f = current_quat.m128_f32[2];
                float w_f = current_quat.m128_f32[3];

                // Undo the 180° roll to get original quaternion
                float qx_o = z_f;
                float qy_o = -y_f;
                float qz_o = -x_f;
                float qw_o = w_f;

                // Extract yaw
                float sin_y = -qy_o;
                float cos_y = qw_o;
                float yaw = std::atan2(sin_y, cos_y);

                // Extract pitch
                float cos_p = std::sqrt(qw_o * qw_o + qy_o * qy_o);
                cos_y = (cos_p > 0.0f) ? qw_o / cos_p : 0.0f;
                sin_y = (cos_p > 0.0f) ? -qy_o / cos_p : 0.0f;

                float sin_p;
                if (std::fabs(cos_y) >= std::fabs(sin_y))
                {
                    sin_p = (cos_y != 0.0f) ? -qx_o / cos_y : 0.0f;
                }
                else
                {
                    sin_p = (sin_y != 0.0f) ? qz_o / sin_y : 0.0f;
                }

                float pitch = std::atan2(sin_p, cos_p);

                // Back-calculate accumulated angles
                float angle_factor = 0.5f;
                accumulated_yaw = yaw / angle_factor;
                accumulated_pitch = pitch / angle_factor;
            }

            // Raw input accumulation for direct, responsive camera movement
            float max_pitch = 60.0f * (3.1415926535f / 180.0f);  // Clamp to prevent gimbal lock
            float min_pitch = -89.0f * (3.1415926535f / 180.0f);

            if (CurrentCameraMode == CameraInVehicle)
                max_pitch = 10.0f * (3.1415926535f / 180.0f);

            // Accumulate raw mouse deltas
            accumulated_yaw += -delta_x;
            accumulated_pitch += delta_y;

            // Clamp pitch
            accumulated_pitch = std::max(min_pitch, std::min(max_pitch, accumulated_pitch));

            // Compute angles
            float angle_factor = 0.5f;
            float yaw = accumulated_yaw * angle_factor;
            float pitch = accumulated_pitch * angle_factor;

            // Compute quaternion using SSE for consistency with original code
            __m128 cos_sin_y = _mm_set_ps(std::cos(yaw), std::sin(yaw), std::cos(yaw), std::sin(yaw));  // cos_y, sin_y, cos_y, sin_y
            __m128 cos_sin_p = _mm_set_ps(std::cos(pitch), std::sin(pitch), std::cos(pitch), std::sin(pitch));  // cos_p, sin_p, cos_p, sin_p

            __m128 qx = _mm_mul_ps(_mm_set_ps1(-1.0f), _mm_mul_ps(_mm_shuffle_ps(cos_sin_p, cos_sin_p, 1), _mm_shuffle_ps(cos_sin_y, cos_sin_y, 0)));  // -sin_p * cos_y
            __m128 qy = _mm_mul_ps(_mm_set_ps1(-1.0f), _mm_mul_ps(_mm_shuffle_ps(cos_sin_p, cos_sin_p, 0), _mm_shuffle_ps(cos_sin_y, cos_sin_y, 1)));  // -cos_p * sin_y
            __m128 qz = _mm_mul_ps(_mm_shuffle_ps(cos_sin_p, cos_sin_p, 1), _mm_shuffle_ps(cos_sin_y, cos_sin_y, 1));  // sin_p * sin_y
            __m128 qw = _mm_mul_ps(_mm_shuffle_ps(cos_sin_p, cos_sin_p, 0), _mm_shuffle_ps(cos_sin_y, cos_sin_y, 0));  // cos_p * cos_y

            // Combine into quaternion vector (x,y,z,w)
            __m128 quat = _mm_set_ps(qw.m128_f32[0], qz.m128_f32[0], qy.m128_f32[0], qx.m128_f32[0]);

            // Normalize quaternion (using original-style dot product and inverse sqrt)
            __m128 dot = _mm_mul_ps(quat, quat);
            dot.m128_f32[0] = dot.m128_f32[0] + _mm_shuffle_ps(dot, dot, 85).m128_f32[0] + _mm_shuffle_ps(dot, dot, 170).m128_f32[0] + _mm_shuffle_ps(dot, dot, 255).m128_f32[0];
            __m128 inv_sqrt = _mm_set_ps1(1.0f / std::sqrt(dot.m128_f32[0]));
            quat = _mm_mul_ps(quat, inv_sqrt);

            // --- APPLY 180° ROLL TO FLIP CAMERA (fix upside-down startup) ---
            // roll quaternion r = (0,0,1,0); multiply r * quat (scalar implementation)
            {
                float qx_f = quat.m128_f32[0];
                float qy_f = quat.m128_f32[1];
                float qz_f = quat.m128_f32[2];
                float qw_f = quat.m128_f32[3];

                float nx = -qy_f;
                float ny = qx_f;
                float nz = qw_f;
                float nw = -qz_f;

                quat = _mm_set_ps(nw, nz, ny, nx); // (w,z,y,x) -> stored as (x,y,z,w) in m128_f32[]
            }

            // Set normalized quaternion directly as v1 (replacing interpolation)
            __m128 v1 = quat;

            // Proceed with original normalization and computations (adapted for direct quaternion)
            __m128 v2 = _mm_mul_ps(v1, v1);
            v2.m128_f32[0] = (float)((float)(v2.m128_f32[0] + _mm_shuffle_ps(v2, v2, 85).m128_f32[0]) + _mm_shuffle_ps(v2, v2, 170).m128_f32[0]) + _mm_shuffle_ps(v2, v2, 255).m128_f32[0];
            __m128 v9 = v2;
            v9.m128_f32[0] = 1.0f / std::sqrt(v2.m128_f32[0]);
            __m128 v3 = _mm_mul_ps(v1, _mm_shuffle_ps(v9, v9, 0));
            v9.m128_u64[1] = *(unsigned int*)float_70F390;
            v9.m128_u64[0] = 0;
            __m128 v4 = v9;
            v9.m128_u64[0] = _mm_shuffle_ps(v3, v3, 255).m128_u32[0];
            __m128 v5 = _mm_shuffle_ps(v3, v3, 201);
            __m128 v6 = _mm_sub_ps(_mm_mul_ps(v3, _mm_shuffle_ps(v4, v4, 201)), _mm_mul_ps(v5, v4));
            __m128 v7 = _mm_sub_ps(_mm_shuffle_ps(v6, v6, 201), _mm_mul_ps(v4, _mm_set_ps1(*(float*)&v9.m128_u32[0])));
            *xmmword_778BF0 = v3;
            __m128 v8 = _mm_sub_ps(_mm_mul_ps(v7, v5), _mm_mul_ps(_mm_shuffle_ps(v7, v7, 201), v3));
            *xmmword_778BD0 = _mm_sub_ps(*xmmword_778BA0, _mm_sub_ps(v4, _mm_mul_ps(_mm_shuffle_ps(v8, v8, 201), _mm_set_ps1(2.0f))));
        }
        else
        {
            shsub_40D990.unsafe_stdcall(a1);
        }
    }
}

export void InitRawInput()
{
    auto pattern = hook::pattern("88 15 ? ? ? ? 8D 45");
    bPause = *pattern.get_first<bool*>(2);

    pattern = hook::pattern("32 C0 88 81 ? ? ? ? A2 ? ? ? ? E8 ? ? ? ? 33 C0 C3");
    bCutscene = *pattern.get_first<bool*>(9);

    pattern = hook::pattern("83 3D ? ? ? ? ? 74 ? 84 DB");
    nLoading = *pattern.get_first<uint32_t*>(2);

    pattern = hook::pattern("D8 0D ? ? ? ? 6A 00 68 3F 8F F7 79");
    fMouseSens = *pattern.get_first<float*>(2);

    pattern = hook::pattern("38 1D ? ? ? ? 0F 84 ? ? ? ? 8B 86");
    byte_850D8A = *pattern.get_first<uint8_t*>(2);

    pattern = hook::pattern("0F 29 05 ? ? ? ? 0F 28 46 ? 6A 00");
    xmmword_778BF0 = *pattern.get_first<__m128*>(3);

    pattern = hook::pattern("0F 29 05 ? ? ? ? 0F 28 46 ? 68 8F C2 F5 3C");
    xmmword_778BD0 = *pattern.get_first<__m128*>(3);

    pattern = hook::pattern("0F 28 05 ? ? ? ? 0F 5C C1 0F 28 D0");
    xmmword_778BA0 = *pattern.get_first<__m128*>(3);

    pattern = hook::pattern("F3 0F 11 05 ? ? ? ? F3 0F 10 05 ? ? ? ? F3 0F 59 05 ? ? ? ? 8D 4C 24");
    float_70F390 = *pattern.get_first<float*>(4);

    pattern = hook::pattern("A1 ? ? ? ? 80 BC 28 ? ? ? ? ? 8D B4 28 ? ? ? ? C7 03 00 00 00 00 0F 8C ? ? ? ? 8B CE E8 ? ? ? ? 84 C0 0F 84");
    dword_79430C = *pattern.get_first<uint8_t**>(1);

    pattern = hook::pattern("64 A1 ? ? ? ? 6A FF 68 ? ? ? ? 50 8B 44 24 ? 83 F8 51 64 89 25 ? ? ? ? 56 8B F1 77");
    shsub_41D3C0 = safetyhook::create_inline(pattern.get_first(0), sub_41D3C0);

    pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 20 F3 0F 10 05 ? ? ? ? F3 0F 59 45");
    shsub_40D990 = safetyhook::create_inline(pattern.get_first(0), sub_40D990);

    pattern = find_pattern("81 EC 90 00 00 00 53", "83 EC 48 53 8B 5C 24");
    shsub_653250 = safetyhook::create_inline(pattern.get_first(0), sub_653250);

    static float menuDeltaX = 0.0f;
    static float menuDeltaY = 0.0f;

    pattern = hook::pattern("C6 44 24 ? ? E8 ? ? ? ? 8B 4C 24 ? 3B CD");
    static auto UpdateInputHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (hGameWindow != GetForegroundWindow())
            return;

        RawCursorHandler<float>::UpdateMouseInput(false);

        if (*byte_850D8A)
            bNonMouseCameraMovement = true;
        else if (std::fabs(RawCursorHandler<float>::MouseDeltaX) > 1e-6f || std::fabs(RawCursorHandler<float>::MouseDeltaY) > 1e-6f)
            bNonMouseCameraMovement = false;

        if (*nLoading != 0 || *bCutscene || *bPause || bNonMouseCameraMovement)
        {
            menuDeltaX = RawCursorHandler<float>::MouseDeltaX;
            menuDeltaY = RawCursorHandler<float>::MouseDeltaY;
            idle_timer = 10.0f;
            RawCursorHandler<float>::MouseDeltaX = 0.0f;
            RawCursorHandler<float>::MouseDeltaY = 0.0f;
        }
    });

    // menu map cursor
    pattern = hook::pattern("76 ? F3 0F 10 05 ? ? ? ? F3 0F 10 4C 24 ? F3 0F 59 C8 F3 0F 58 4E");
    injector::MakeNOP(pattern.get_first(), 2, true);

    pattern = hook::pattern("F3 0F 11 4E ? F3 0F 10 4C 24 ? F3 0F 59 C8");
    injector::MakeNOP(pattern.get_first(), 5, true);
    static auto MouseX = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bool mouse_moved = (std::fabs(menuDeltaX) > 1e-6f);
        if (mouse_moved)
        {
            *(float*)(regs.esi + 0x54) += menuDeltaX * 27.5f;
        }
        else
        {
            *(float*)(regs.esi + 0x54) = regs.xmm1.f32[0];
        }
        menuDeltaX = 0.0f;
    });

    pattern = hook::pattern("F3 0F 11 4E ? 8B 46");
    injector::MakeNOP(pattern.get_first(), 5, true);
    static auto MouseY = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        bool mouse_moved = (std::fabs(menuDeltaY) > 1e-6f);
        if (mouse_moved)
        {
            *(float*)(regs.esi + 0x58) += -menuDeltaY * 27.5f;
        }
        else
        {
            *(float*)(regs.esi + 0x58) = regs.xmm1.f32[0];
        }
        menuDeltaY = 0.0f;
    });
}