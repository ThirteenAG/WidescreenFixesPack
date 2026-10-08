-- Default native Release object reuse. Debug and console builds retain their
-- original compilation. Utility projects use MSBuild's native ClCompile target
-- and file tracking, and never invoke a librarian or linker.
local imports = {}
local root = path.getabsolute("../..", _SCRIPT_DIR)
local sources = {
   "external/hooking/Hooking.Patterns.cpp",
   "external/injector/safetyhook/src/**.cpp",
   "external/injector/minhook/src/**.c",
   "external/injector/utility/FunctionHookMinHook.cpp",
   "external/injector/zydis/**.c"
}
local objects = {}
local objectNames = {}
local sharedSources = {}
for _, pattern in ipairs(sources) do
   for _, source in ipairs(os.matchfiles(path.join(root, pattern))) do
      table.insert(sharedSources, source)
   end
end
-- Preserve the source order used by the generated native projects.
table.sort(sharedSources)
for _, source in ipairs(sharedSources) do
   local name = path.getbasename(source) .. ".obj"
   assert(not objectNames[name:lower()], "Shared dependency object name collision: " .. name)
   objectNames[name:lower()] = true
   table.insert(objects, "$(ProjectDir)obj\\shared\\$(Platform)\\$(Configuration)\\" .. name)
end

for wks in premake.global.eachWorkspace() do
   local platform = ({ Win32 = "Win32", Win64 = "x64", Dolphin = "x64", CXBXR = "Win32" })[wks.name:match("^(.-)%.")]
   if platform == "Win32" or platform == "x64" then
      local consumers = {}
      for _, prj in ipairs(wks.projects) do
         table.insert(consumers, prj)
      end
      local dependency = "ReleaseDependencies." .. platform
      workspace(wks.name)
      for _, prj in ipairs(consumers) do
         project(prj.name)
         imports[prj.name] = { dependency = dependency }
         dependson { dependency }
         for _, source in ipairs(sources) do
            filter { "configurations:Release", "files:**/" .. source }
               excludefrombuild "On"
         end
         filter {}
      end

      group "Build dependencies"
      project(dependency)
         kind "Utility"
         -- The same architecture's project is used by multiple solutions.
         -- Their sequential builds reuse the same tracked intermediate files.
         objdir(path.join(root, "build/obj/shared/%{cfg.platform}/%{cfg.buildcfg}"))
         targetdir(path.join(root, "build/obj/shared/%{cfg.platform}/%{cfg.buildcfg}"))
         removefiles { path.join(root, "includes/stdafx.cpp"), path.join(root, "Resources/*.rc") }
         imports[dependency] = { producer = true }
      filter {}
      group ""
   end
end

premake.override(premake.vstudio.vc2010.elements, "project", function(base, prj)
   local elements = base(prj)
   local settings = imports[prj.name]
   if settings then
      table.insert(elements, function()
         premake.push('<PropertyGroup Condition="\'$(Configuration)\'==\'Release\'">')
         if settings.producer then
            premake.w('<WFPSharedDependencyProducer>true</WFPSharedDependencyProducer>')
         else
            premake.w('<WFPSharedDependencyObjects>%s</WFPSharedDependencyObjects>', table.concat(objects, ";"))
         end
         premake.pop('</PropertyGroup>')
         if settings.dependency then
            premake.push('<ItemGroup Condition="\'$(Configuration)\'==\'Release\'">')
            premake.push('<ProjectReference Include="%s.vcxproj">', settings.dependency)
            premake.w('<LinkLibraryDependencies>false</LinkLibraryDependencies>')
            premake.w('<ReferenceOutputAssembly>false</ReferenceOutputAssembly>')
            premake.pop('</ProjectReference>')
            premake.pop('</ItemGroup>')
         end
         premake.w('<Import Project="..\\tools\\msbuild\\shared-dependencies.targets" />')
      end)
   end
   return elements
end)
