#include "Game.hpp"
extern "C" {
int CompatibleCRCList[]={0x4F32A11F,static_cast<int>(0xB3AD1EA4),0x7EA439F5,0x42A9C4EC,0x7A9D67B8,0x77B4F13C,static_cast<int>(0xB1AC3BEB)};
int CompatibleElfCRCList[]={0x4F32A11F,static_cast<int>(0xB3AD1EA4),0x7EA439F5,0x42A9C4EC,0x1118ACD0,0x7A9D67B8,static_cast<int>(0xB73CDCFA),0x77B4F13C,static_cast<int>(0xA4334B91),static_cast<int>(0xB1AC3BEB),static_cast<int>(0xB2D44C6C)};
alignas(16) char XboxRainDropletsData[255]="XBOXRAINDROPLETSDATA";
int PCSX2Data[PCSX2Data_Size]={1};
void init() {
    if (injector::InitializeRuntime()!=PCSX2_HOOK_OK) return;
    if (!rain::Stories()) (void)rain::TrueCrime();
}
int main() { return 0; }
}
