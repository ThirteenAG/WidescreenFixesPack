![scpt](https://thirteenag.github.io/screens/scpt/main2.jpg)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed FMVs

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Force highest character LOD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Disabled mouse acceleration

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Improved gamepad support (via [Xidi](https://github.com/samuelgr/Xidi))

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to skip intro

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to increase night vision resolution

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to increase shadows resolution

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to increase reflections resolution

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to change goggles' light color

    Installation:
        Download and extract the archive to the game directory, where the exe is located.

> [!WARNING]
> Non-Windows users (Proton/Wine) need to perform a **DLL override**.
>
> <details>
> <summary>Click here for details</summary>
> <br>
>
> You need to tell Wine explicitly to use the correct DLL overrides, which include `dinput8.dll`, `msacm32.dll`, `msvfw32.dll`, and `Xidi.32.dll`. There's more than one way to achieve it.
>
> ###
>
> **Method 1**: `WINEDLLOVERRIDES` variable lets you temporarily specify DLL overrides. It can be used from a command line as well as in the Steam launcher. In the case of the command line, simply prepend the usual start command with:
>    ```
>    `WINEDLLOVERRIDES="dinput8,msacm32,msvfw32,Xidi.32=n,b"`
>    ```
> For Steam, head to the game’s properties and set `LAUNCH OPTIONS` to `WINEDLLOVERRIDES="dinput8,msacm32,msvfw32,Xidi.32=n,b" %command%`.
>
>  ![steam-wine-dll-override](https://cookieplmonster.github.io/assets/img/setup/steam-wine-dll-override.png)
>
> **Method 2**: Use `winecfg` tool to make a permanent override for a specific Wine prefix. In case of Proton, Steam creates the Wine prefix for Splinter Cell: Pandora Tomorrow in `$HOME/.steam/steam/steamapps/compatdata/3929740/pfx`. Then you need to run `winecfg` with that path:
> ```
> WINEPREFIX="$HOME/.steam/steam/steamapps/compatdata/3929740/pfx" winecfg
> ```
> Select the `Libraries` tab and fill the combo box with the name of the library you wish to override and hit `Add`. You can verify that it’s been added to the list below with `(native, builtin)` suffix. Then close the window with the `OK` button.
>
> ![winecfg-dll-override](https://cookieplmonster.github.io/assets/img/setup/winecfg-dll-override.png)
>
> Related Wine documentation:
> * [More on DLL overrides](https://wiki.winehq.org/Wine_User's_Guide#DLL_Overrides)
> * [More on WINEDLLOVERRIDES method](https://wiki.winehq.org/Wine_User's_Guide#WINEDLLOVERRIDES.3DDLL_Overrides)
> </details>

[Website](https://thirteenag.github.io/wfp#scpt) | [Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellPandoraTomorrow.WidescreenFix/dllmain.cpp) | [Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/SplinterCellPandoraTomorrow.WidescreenFix/system/scripts/SplinterCellPandoraTomorrow.WidescreenFix.ini)

<a href="https://github.com/sponsors/ThirteenAG"><img src="https://img.shields.io/badge/Sponsor_me_on_GitHub-5c5c5c?style=for-the-badge&logo=github&logoColor=white" width="250"></a><br><a href="https://ko-fi.com/thirteenag"><img src="https://img.shields.io/badge/Support_me_on_Ko--Fi-ff5e5b?style=for-the-badge&logo=kofi&logoColor=white" width="250"></a><br><a href="https://paypal.me/SergeyP13"><img src="https://img.shields.io/badge/Donate_with_PayPal-009cde?style=for-the-badge&logo=paypal&logoColor=white" width="250"></a><br><a href="https://www.patreon.com/ThirteenAG"><img src="https://img.shields.io/badge/Support_me_on_Patreon-ff424d?style=for-the-badge&logo=patreon&logoColor=white" width="250"></a><br><a href="https://boosty.to/thirteenag/donate"><img src="https://img.shields.io/badge/Support_me_on_Boosty-f15e2d?style=for-the-badge&logo=data:image/png;base64,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&logoColor=white" width="250"></a>
