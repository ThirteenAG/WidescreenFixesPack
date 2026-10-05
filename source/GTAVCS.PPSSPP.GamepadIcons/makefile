CONFIGURATION ?= Release
.PHONY: all clean
all:
	powershell -NoProfile -ExecutionPolicy Bypass -File "../../external/pspsdk/plugins/build-module.ps1" -Project module.json -Configuration "$(CONFIGURATION)"
clean:
	powershell -NoProfile -ExecutionPolicy Bypass -File "../../external/pspsdk/plugins/build-module.ps1" -Project module.json -Configuration "$(CONFIGURATION)" -Clean
