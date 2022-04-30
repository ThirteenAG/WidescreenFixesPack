EE_BIN = ../../data/GTAVCS.PCSX2F.WidescreenFix/PLUGINS/GTAVCS.PCSX2F.WidescreenFix.elf
EE_OBJS = main.o cpad.o ckey.o ../../includes/pcsx2/memalloc.o ../../includes/pcsx2/patterns.o ../../includes/pcsx2/injector.o ../../includes/pcsx2/rini.o ../../includes/pcsx2/inireader.o ../../includes/pcsx2/mips.o 

BASE_ADDRESS = 0x02100000
EE_LINKFILE = linkfile
EE_LIBS += -l:libc.a
EE_LDFLAGS = -Wl,--entry=init -Wl,-Map,../../data/GTAVCS.PCSX2F.WidescreenFix/PLUGINS/GTAVCS.PCSX2F.WidescreenFix.map -nostdlib -nodefaultlibs -Wl,'--defsym=BASE_ADDRESS=$(BASE_ADDRESS)'

all: clean main-build

main-build: $(EE_BIN)

clean:
	rm -f $(EE_OBJS) $(EE_BIN)

PS2SDK = ../../external/ps2sdk/ps2sdk
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
