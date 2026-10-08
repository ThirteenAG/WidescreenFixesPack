# Shared Release dependency objects

Ordinary `premake5 vs2026` generation creates native MSBuild utility
projects that compile SafetyHook, MinHook, Zydis, Hooking.Patterns, and
FunctionHookMinHook once per architecture in `build/obj/shared`.
Plugins link the loose objects directly. No static library or PCH is created.

All native Release builds use shared objects, including local builds and manual
release workflows. Debug compilation and PS2/PSP compilation retain their
existing behavior. The Debug utility project is a no-op.

For troubleshooting, `premake5 --no-shared-release-deps vs2026` restores
compilation of dependencies separately in each plugin. Regenerate with the
ordinary command to enable sharing again.

The utility project's `ClCompile` target uses native MSBuild header, command,
and output tracking. Project references support standalone plugin builds, and
solution dependencies coordinate parallel builds. Missing shared objects are
regenerated; shared object changes are tracked as linker inputs. Win32/CXBXR
and Win64/Dolphin reuse their architecture's objects in the sequential CI build.

`shared-dependencies.targets` defines the common Release compilation settings.
If a dependency needs a plugin-specific macro or compiler setting in the future,
give that variant its own producer or leave that source in the plugin. Do not
reuse an object across incompatible architecture, runtime, or compiler settings.

`stdafx.cpp`, plugin sources, resources, shaders/modules, and the optional
Kananlib/bddisasm sources are still compiled by their original projects.
