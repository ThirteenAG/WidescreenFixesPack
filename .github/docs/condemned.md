![condemned](http://thirteenag.github.io/screens/condemned/main2.jpg)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution (Config.exe)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Aspect Ratio (Ultra-Wide)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD (Ultra-Wide)

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to fix stretched menu

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to fix low framerate

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to fix issue when controls aren't saved

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to make windowed mode borderless

[Website](http://thirteenag.github.io/wfp#condemned) | [Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/Condemned.WidescreenFix/dllmain.cpp) | [Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/Condemned.WidescreenFix/scripts/Condemned.WidescreenFix.ini)

## About steam files

From PCGW:
> The Steam version's CondemnedA.Arch00 is noticeably smaller when compared to the retail game. Because of this, and reversed audio files within the .Arch00 file, many sounds are simply missing from the Steam version. In particular, the train sounds at the end of Chapter 3 are missing. The footage from "The Dark" game prototype is also corrupted.

I compared the files and made a separate archive with missing/different files. That way there's no need to download and replace large files.
Installation: 
- Download ![Condemned.MissingSteamFilesFix.zip](https://github.com/ThirteenAG/WidescreenFixesPack/releases/download/condemned/Condemned.MissingSteamFilesFix.zip) for a full package **or** ![Condemned.MissingSteamFilesFix.Lite.zip](https://github.com/ThirteenAG/WidescreenFixesPack/releases/download/condemned/Condemned.MissingSteamFilesFix.Lite.zip) if you don't care about having retail TheDark_4-5-04.bik and FinalCreds.bik.
- Unpack everything to root directory, replacing **default.archcfg** in the process.

Widescreen fix also contains **default.archcfg**, but they are identical between archives. It is used to make the game load arch00 archives without needing to replace them.

Additionally, visit my [steam guide](https://steamcommunity.com/sharedfiles/filedetails/?id=1593741074) for more info about gamepad support in this game.