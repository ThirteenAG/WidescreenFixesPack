TARGET = ..\..\data\TheWarriors.PPSSPP.FusionMod\memstick\PSP\PLUGINS\TheWarriors.PPSSPP.FusionMod\TheWarriors.PPSSPP.FusionMod
OBJS = main.o exports.o ../../includes/psp/injector.o ../../includes/psp/log.o ../../includes/psp/patterns.o ../../includes/psp/minIni.o ../../includes/psp/inireader.o ../../includes/psp/gvm.o ../../includes/psp/mips.o

CFLAGS = -O2 -Os -G0 -Wall -fshort-wchar -fno-pic -mno-check-zero-division
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

BUILD_PRX = 1
PRX_EXPORTS = exports.exp

USE_PSPSDK_LIBC = 1

LIBS = -lpspsystemctrl_kernel -lm

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak
