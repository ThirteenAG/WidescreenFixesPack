<div align="center">

[![wfp](https://thirteenag.github.io/img/logo-stroke.svg)](https://thirteenag.github.io/wfp)

[![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/ThirteenAG/WidescreenFixesPack/main.yml?branch=master&label=GitHub%20Actions%20Build&logo=GitHub)](https://github.com/ThirteenAG/WidescreenFixesPack/actions/workflows/main.yml)
[![AppVeyor](https://img.shields.io/appveyor/build/ThirteenAG/WidescreenFixesPack?label=AppVeyor%20Build&logo=Appveyor&logoColor=white)](https://ci.appveyor.com/project/ThirteenAG/widescreenfixespack)
[![Azure DevOps builds](https://img.shields.io/azure-devops/build/ThirteenAG/f555b128-da05-4bad-a972-90d529123a2e/3?label=Azure%20Pipelines%20Build&logo=Azure%20Pipelines)](https://dev.azure.com/thirteenag/WidescreenFixesPack/_build/latest?definitionId=3&branchName=master)
[![CircleCI](https://img.shields.io/circleci/build/github/ThirteenAG/WidescreenFixesPack?label=CircleCI%20Build&logo=circleci)](https://circleci.com/gh/ThirteenAG/WidescreenFixesPack/tree/master)

[![GitHub license](https://img.shields.io/github/license/ThirteenAG/WidescreenFixesPack?color=blue)](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/license)
[![GitHub stars](https://img.shields.io/github/stars/ThirteenAG/WidescreenFixesPack)](https://github.com/ThirteenAG/WidescreenFixesPack/stargazers)
[![Commits](https://img.shields.io/github/commit-activity/m/ThirteenAG/WidescreenFixesPack?label=commits)](https://github.com/ThirteenAG/WidescreenFixesPack/commits)
[![GitHub last commit](https://img.shields.io/github/last-commit/ThirteenAG/WidescreenFixesPack?color=blue)](https://github.com/ThirteenAG/WidescreenFixesPack/commits)

[![Platforms](https://img.shields.io/badge/platforms:-grey)](https://github.com/ThirteenAG/WidescreenFixesPack/releases)
[![WINDOWS](https://img.shields.io/badge/WINDOWS-0078D4)](https://github.com/ThirteenAG/WidescreenFixesPack/releases?q=NOT+CXBXR+NOT+PCSX2F+NOT+PPSSPP&expanded=true)
[![PCSX2F](https://img.shields.io/badge/PCSX2F-0271A6)](https://github.com/ThirteenAG/WidescreenFixesPack/releases?q=PCSX2F&expanded=true)
[![PPSSPP](https://img.shields.io/badge/PPSSPP-33b5e5)](https://github.com/ThirteenAG/WidescreenFixesPack/releases?q=PPSSPP&expanded=true)
[![CXBXR](https://img.shields.io/badge/CXBXR-96CE49)](https://github.com/ThirteenAG/WidescreenFixesPack/releases?q=CXBXR&expanded=true)
[![DOLPHIN](https://img.shields.io/badge/DOLPHIN-30b5ff)](https://github.com/ThirteenAG/WidescreenFixesPack/releases?q=DOLPHIN&expanded=true)

[![Downloads](https://img.shields.io/github/downloads/ThirteenAG/WidescreenFixesPack/total?color=red)](https://github.com/ThirteenAG/WidescreenFixesPack/releases/)

https://thirteenag.github.io/wfp

Plugins to make or improve widescreen resolutions support in PC games, add more features and fix bugs.

<a href="https://ko-fi.com/thirteenag"><img src="https://github.com/ThirteenAG/thirteenag.github.io/raw/master/img/buttons/kofi.svg" height="40"></a> <a href="https://paypal.me/SergeyP13"><img src="https://github.com/ThirteenAG/thirteenag.github.io/raw/master/img/buttons/paypal.svg" height="40"></a> <a href="https://www.patreon.com/ThirteenAG"><img src="https://github.com/ThirteenAG/thirteenag.github.io/raw/master/img/buttons/patreon.svg" height="40"></a> <a href="https://boosty.to/thirteenag/donate"><img src="https://github.com/ThirteenAG/thirteenag.github.io/raw/master/img/buttons/boosty.svg" height="40"></a>

</div>

# Building and Installing

Requirements:

- [Premake 5](https://premake.github.io/) _(pre-built executable available in this repository root)_
- [Visual Studio](http://www.visualstudio.com/downloads)

Run the following command in the root of this directory to generate the project files (or simply launch **premake5.bat**):

    premake5 vs2022

The usage is as simple as inserting the files into game's root directory. Uninstalling is as easy as that too, delete the files and you are done.

# Important notes

- Not compatible with Windows XP (without recompiling with `_xp` toolset and `/Zc:threadSafeInit-`).
- For using with WINE, follow [this guide](https://cookieplmonster.github.io/setup-instructions/#proton-wine).
