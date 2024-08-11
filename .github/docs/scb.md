![scb](https://thirteenag.github.io/screens/scb/main2.jpg)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Added an option to skip intro

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Added an option to skip press any key screen

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Added an option to switch the game to fullscreen mode on startup

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Added an option to disable negative mouse acceleration

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Added an option to enable ultrawide support

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Added an option to change FOV

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Added an option to change Screen Cull Bias, for better draw distance

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Added an option to unlock DLC content that's unavailable after servers shut down

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to unlock all missions from the start

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to load packed files from disk

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to enable [Logitech G LIGHTSYNC RGB Lighting](https://www.logitechg.com/innovation/lightsync-rgb.html)

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Difficulty tweaks: **Mark and Execute**, sonar and drop crates enabled on **Perfectionist**, unlimited ammo disabled on **Rookie**. To go back to original behavior, delete `update/difficultyconfiguration.ini` and set `DisablePerfectionistChecks` to 0.

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to be able to run during forced walking sections

# Extraction Mode

Plugin adds an ability to modify the number of enemies for the Extraction game mode with `ExtractionWaveEnemyMultiplier` option.

New config for the Extraction game mode: Random, which randomizes the number and type of enemies in each wave using minimum and maximum values defined in the INI file.

To load custom configs for the Extraction game mode, create a folder inside `Splinter Cell Blacklist\src\SYSTEM\update\Data\ExtractionWaveConfigs\`, copy all XMLs into it, then set the name of the new folder in `ExtractionWaveConfigs` option in the INI file.

E.g.:

```
Splinter Cell Blacklist\src\SYSTEM\update\Data\ExtractionWaveConfigs\
NewConfig\
          DefaultWaveConfig.xml
          D_Amman.xml
          D_Bratislava.xml
          D_Kigali.xml
          D_Sanaa.xml
```

```
[EXTRACTION]
ExtractionWaveConfigs = NewConfig
```

Demo: https://youtu.be/su47XbCcVyw

# Hunter Mode and Coop

Plugin adds an ability to modify the number of reinforcements for the Hunter game mode and coop campaign with `ReinforcementsEnemyMultiplier` option.

New config for the Hunter game mode and coop campaign: Random, which randomizes the total number of reinforcements between minimum and maximum values defined in the INI file.

# Ghost Mode and Campaign

Plugin adds an ability to disable mission failure on detection in Ghost mode and Campaign (SP and COOP).

# Extra tweaks

Disable Active Sprint: [NoActiveSprint.zip](https://github.com/user-attachments/files/16575499/NoActiveSprint.zip)

Hold DPAD-UP to switch between vision modes: [HoldDPADUPToSwitchVisionMode.zip](https://github.com/user-attachments/files/16575500/HoldDPADUPToSwitchVisionMode.zip)

Put edited ini configs in `update` folder.

# Ultrawide Screenshot

![scb](https://github.com/user-attachments/assets/96d6a1e7-457e-4ae4-a2f6-88cc9a632c80)

    Installation:
        Download and extract the archive to the game directory, where the exe is located.

[Website](https://thirteenag.github.io/wfp#scb) | [Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/SplinterCellBlacklist.FusionMod/dllmain.cpp) | [Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/SplinterCellBlacklist.FusionMod/src/system/scripts/SplinterCellBlacklist.FusionMod.ini)

<a href="https://github.com/sponsors/ThirteenAG"><img src="https://img.shields.io/badge/Sponsor_me_on_GitHub-5c5c5c?style=for-the-badge&logo=github&logoColor=white" width="250"></a><br><a href="https://ko-fi.com/thirteenag"><img src="https://img.shields.io/badge/Support_me_on_Ko--Fi-ff5e5b?style=for-the-badge&logo=kofi&logoColor=white" width="250"></a><br><a href="https://paypal.me/SergeyP13"><img src="https://img.shields.io/badge/Donate_with_PayPal-009cde?style=for-the-badge&logo=paypal&logoColor=white" width="250"></a><br><a href="https://www.patreon.com/ThirteenAG"><img src="https://img.shields.io/badge/Support_me_on_Patreon-ff424d?style=for-the-badge&logo=patreon&logoColor=white" width="250"></a><br><a href="https://boosty.to/thirteenag/donate"><img src="https://img.shields.io/badge/Support_me_on_Boosty-f15e2d?style=for-the-badge&logo=data:image/png;base64,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&logoColor=white" width="250"></a>