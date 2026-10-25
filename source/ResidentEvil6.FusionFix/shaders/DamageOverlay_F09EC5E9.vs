// The low health frame (the game's vertex shader F09EC5E9): a blurred frame in the HUD's 3D space, which spans the
// 16:9 middle of the screen. The fix widens it to the whole screen: clip space x times c230.x (screen aspect / 16:9,
// at least 1; set by the fix when the shader is bound).
vs_3_0
def c0, 32768, 1, 3.90624991e-005, 0
def c10, 0.000244140625, 0, 0, 0
dcl_position v0
dcl_normal v1
dcl_tangent v2
dcl_binormal v3
dcl_position o0
dcl_texcoord o1
dcl_texcoord1 o2
dcl_texcoord2 o3
mov r0.x, c1.w
mov r0.y, c2.w
mov r0.z, c3.w
mov r0.w, c4.w
mov r1.xyz, v0
mad r1.xyz, c7, -v3.x, r1
mov r1.w, c0.y
dp4 r0.x, r1, r0
rcp r0.x, r0.x
mul r0.yzw, c2.xxyw, v0.y
mad r0.yzw, v0.x, c1.xxyw, r0
mad r0.yzw, v0.z, c3.xxyw, r0
add r0.yzw, r0, c4.xxyw
mul r2.x, r0.w, c1.z
mul r2.y, r0.w, c2.z
mul r2.z, r0.w, c3.z
mul r2.w, r0.w, c4.z
mul r2, r0.x, r2
dp4 o0.z, r1, r2
add r0.x, c0.x, v3.y
mul r0.x, r0.x, c9.z
mul o2.w, r0.x, c0.z
mul r0.y, r0.y, c230.x
mad o0.x, c8.x, -r0.w, r0.y
mad o0.y, c8.y, r0.w, r0.z
mov o0.w, r0.w
mov o1, v1
mov o2.xyz, c0.ywww
mul o3, c10.xxyy, v2.xyxx
