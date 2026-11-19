; Reflective water, Direct3D 8 path (vertex part)
; c0-c3   object to clip space (transposed)
; c4-c6   object to world space (transposed rows)
; c7      camera position
; c8-c11  object to reflection texture space: (u*w, v*w, w) scaled down
; c12-c15 two wave layers: u = dot((x, z, 1), c12), v = dot((x, z, 1), c13) ...
; c16     0, 1, 0.01, 2
; c17     fog end, 1 / (fog end - fog start)
; c18     direction to the sun
vs_1_1
dcl_position v0
dcl_color v1
dcl_texcoord v2

m4x4 r7, v0, c0
mov oPos, r7
m4x3 r0.xyz, v0, c4
m4x4 r1, v0, c8
mov oT0.xyz, r1.xyz

mov r2.x, r0.x
mov r2.y, r0.z
mov r2.z, c16.y
dp3 oT1.x, r2, c12
dp3 oT1.y, r2, c13
dp3 oT2.x, r2, c14
dp3 oT2.y, r2, c15

; view vector and sun half vector, stored as (x, z, y) like the wave normals and left unnormalized: the water's
; triangles are large, so they are interpolated as the exact affine vector and normalized per pixel
add r3.xyz, c7, -r0
dp3 r3.w, r3, r3
rsq r4.w, r3.w
mul r4.w, r3.w, r4.w
mul r5.xyz, c18, r4.w
add r5.xyz, r3, r5
mul r3.xyz, r3, c16.z
mul r5.xyz, r5, c16.z
mov oT3.x, r3.x
mov oT3.y, r3.z
mov oT3.z, r3.y
mov oT5.x, r5.x
mov oT5.y, r5.z
mov oT5.z, r5.y

mov oT4.xy, v2
mov oD0, v1

; linear fog from the view depth
add r6.x, c17.x, -r7.w
mul oFog, r6.x, c17.y
