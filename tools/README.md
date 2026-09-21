# tools

Standalone Windows binaries used by the build scripts (see `prebuildcommands` in `premake5.lua`).
They are kept here, outside of the include/library folders, so that both the 32-bit and 64-bit
copies can coexist next to their own runtime dependencies.

## x86

Used by every project that compiles shaders at build time (`add_postfx()` and the per-project
`prebuildcommands` loops).

| File | Purpose | Depends on |
| --- | --- | --- |
| `fxc.exe` | DirectX SDK (June 2010) effect/vertex/pixel shader compiler | `D3DCompiler_43.dll`, `D3DX9_43.dll` |
| `asm_shader.exe` | Shader assembler for the `*.ps` / `*.vs` files of the Splinter Cell projects | `D3DX9_43.dll` |
| `D3DCompiler_43.dll` | Runtime dependency of `fxc.exe` | |
| `D3DX9_43.dll` | Runtime dependency of `fxc.exe` and `asm_shader.exe` | |

## x64

Same tools, 64-bit builds (only `fxc.exe` is needed there, asm_shader is 32-bit only).

| File | Purpose | Depends on |
| --- | --- | --- |
| `fxc.exe` | DirectX SDK (June 2010) effect/vertex/pixel shader compiler | `D3DCompiler_43.dll`, `D3DX9_43.dll` |
| `D3DCompiler_43.dll` | Runtime dependency of `fxc.exe` | |
| `D3DX9_43.dll` | Runtime dependency of `fxc.exe` | |
