// The 2D overlays (the game's vertex shader 4F0EE939): low health, partner waiting, pause texts. Laid out for the 16:9
// middle of the screen; the fix widens them to the whole screen: clip space x times c230.x (screen aspect / 16:9, at
// least 1; set by the fix when the shader is bound).
vs_3_0
def c0, 32768, -128, 0.00390625, 0.25
def c4, 1, 0, -128, 4
def c5, 0.000244140654, 0.5, 6.28318548, -3.14159274
def c6, 2, -1, 1, 9.99999997e-007
dcl_position v0
dcl_normal v1
dcl_tangent v2
dcl_binormal v3
dcl_texcoord v4
dcl_position o0
dcl_texcoord o1
dcl_texcoord1 o2
mov r0.x, v3.x
add o0.z, r0.x, v0.z
add r0.xyz, c0.x, v3.zwyw
mul r0.xz, r0, c0.z
mad r0.y, r0.y, c5.x, c5.y
frc r0.y, r0.y
mad r0.y, r0.y, c5.z, c5.w
sincos r1.xy, r0.y
mul o1.xyz, r0.z, v1
mul o2.xy, c3, v2
add r0.yz, c0.y, v4.xxyw
mul r0.xy, r0.x, r0.yzzw
mad r0.zw, v4.z, c4.xyxy, c4
mul r0.xy, r0.zwzw, r0
mul r0.x, r0.x, c0.w
mul r0.yz, r1.xyxw, r0.y
mad r2.x, r0.x, r1.x, -r0.y
mad r2.y, r0.x, r1.y, r0.z
add r0.xy, r2, v0
mul r0.xy, r0, c2
mad r0.xy, r0, c6.x, c6.yzzw
slt r0.z, v1.w, c6.w
add r0.z, -r0.z, c4.x
mul r0.x, r0.x, c230.x
mad o0.x, c1.x, -r0.z, r0.x
mad o0.y, c1.y, r0.z, r0.y
mov o0.w, r0.z
mov o1.w, v1.w
mov o2.zw, c4.y
