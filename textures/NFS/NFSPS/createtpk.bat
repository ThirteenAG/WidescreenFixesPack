set "XNFSTPKTool=%cd%\..\XNFSTPKTool.exe"

rmdir /Q /S unpacked
md unpacked

%XNFSTPKTool% GLOBALB.BUN.000175.tpk.bin unpacked

xcopy /S /Q /Y /F "../textures/a.dds" "unpacked/1B4F984/PC_KEY_ENTER.dds"
xcopy /S /Q /Y /F "../textures/b.dds" "unpacked/1B4F984/PC_KEY_ESC.dds"
xcopy /S /Q /Y /F "../textures/b.dds" "unpacked/1B4F984/PC_KEY_ESC_FRENCH.dds"
xcopy /S /Q /Y /F "../textures/quit.dds" "unpacked/1B4F984/PC_KEY_Q.dds"
xcopy /S /Q /Y /F "../textures/lb.dds" "unpacked/1B4F984/L1.dds"
xcopy /S /Q /Y /F "../textures/lb.dds" "unpacked/1B4F984/PC_KEY_9.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/1B4F984/PC_KEY_SPC.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/1B4F984/PC_KEY_SPC_CZECH.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/1B4F984/PC_KEY_SPC_DANISH.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/1B4F984/PC_KEY_SPC_DUTCH.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/1B4F984/PC_KEY_SPC_FRENCH.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/1B4F984/PC_KEY_SPC_GERMAN.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/1B4F984/PC_KEY_SPC_ITALIAN.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/1B4F984/PC_KEY_SPC_SPANISH.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/1B4F984/PC_X.dds"
xcopy /S /Q /Y /F "../textures/rb.dds" "unpacked/1B4F984/PC_KEY_0.dds"
xcopy /S /Q /Y /F "../textures/rb.dds" "unpacked/1B4F984/R1.dds"
xcopy /S /Q /Y /F "../textures/view.dds" "unpacked/1B4F984/PC_KEY_3.dds"
xcopy /S /Q /Y /F "../textures/x.dds" "unpacked/1B4F984/PC_KEY_2.dds"
xcopy /S /Q /Y /F "../textures/y.dds" "unpacked/1B4F984/PC_KEY_1.dds"

%XNFSTPKTool% -w unpacked/1B4F984.ini ../../../data/NFSProStreet.GenericFix/scripts/buttons-xbox.tpk
rmdir /Q /S unpacked


md unpacked

%XNFSTPKTool% GLOBALB.BUN.000175.tpk.bin unpacked



%XNFSTPKTool% -w unpacked/1B4F984.ini ../../../data/NFSProStreet.GenericFix/scripts/buttons-playstation.tpk
rmdir /Q /S unpacked
