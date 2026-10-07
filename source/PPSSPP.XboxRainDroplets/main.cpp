#include "Game.hpp"
extern "C" {
#include <pspkernel.h>
PSP_MODULE_INFO("PPSSPP.XboxRainDroplets",PSP_MODULE_USER,2,0);
alignas(16) char XboxRainDropletsData[255]="XBOXRAINDROPLETSDATA";
}
namespace rain { BeforeUIData beforeUI={"PPSSPPBEFOREUIDATA",0,0}; }
extern "C" int module_start(SceSize,void*) {
    if (!console::portable::Start("GTA3","ms0:/PSP/PLUGINS/PPSSPP.XboxRainDroplets/PPSSPP.XboxRainDroplets.ini","ms0:/PSP/PLUGINS/PPSSPP.XboxRainDroplets/PPSSPP.XboxRainDroplets.log")) {
        SceKernelModuleInfo info{};
        if (!console::portable::Emulator() || !console::portable::FindModule("SplinterCellPSP",info)) return -1;
        console::portable::Attach(info);
    }
    rain::beforeUI.packet=uint32_t(uintptr_t(XboxRainDropletsData));
    sceKernelDelayThread(110000);
    return (rain::Stories() || rain::Essentials()) ? 0 : -1;
}
