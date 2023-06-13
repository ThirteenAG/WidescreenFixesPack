![gta2](http://thirteenag.github.io/screens/gta2/main2.jpg)

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Resolution

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed HUD

![](https://habrastorage.org/webt/ow/yy/mg/owyymgpibfqzfbwyf_iqoiqrede.png) Fixed Field of View

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) Added an option to quicksave via customizable hotkey

![](https://habrastorage.org/webt/d_/eg/ym/d_egymd6w_tem2erocab-e9ikna.png) If you have the issue with broken graphics when driving at high speeds, try setting the ini parameter `CameraZoomFactor = auto` to a hardcoded value. 
It's currently calculated this way: [((AspectRatio) / (4.0 / 3.0)) \* 2.5](<https://www.google.com/search?q=((2560/1080)+/+(4.0+/+3.0))+*+2.5>). 
Find out what that value is for your resolution using this [link](<https://www.google.com/search?q=((2560/1080)+/+(4.0+/+3.0))+*+2.5>) (e.g. for `2560x1080`, `CameraZoomFactor = 4.44`) and set `CameraZoomFactor` to anything less than that (e.g. for `2560x1080`, `CameraZoomFactor = 3.8`).

[Website](http://thirteenag.github.io/wfp#gta2) | [Source](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/source/GTA2.WidescreenFix/dllmain.cpp) | [Default INI File](https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/data/GTA2.WidescreenFix/scripts/GTA2.WidescreenFix.ini)