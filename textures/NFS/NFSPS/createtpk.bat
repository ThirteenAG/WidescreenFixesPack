set "XNFSTPKTool=%cd%\..\XNFSTPKTool.exe"

rmdir /Q /S unpacked
md unpacked
%XNFSTPKTool% GLOBALB.BUN.000175.tpk.bin unpacked
xcopy /S /Q /Y /F "../textures/b.dds" "unpacked/1B4F984/PC_KEY_ESC.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/1B4F984/PC_KEY_SPC.dds"
xcopy /S /Q /Y /F "../textures/rs.dds" "unpacked/1B4F984/PC_KEY_M_FRENCH.dds"
xcopy /S /Q /Y /F "../textures/rt.dds" "unpacked/1B4F984/PC_KEY_0.dds"
xcopy /S /Q /Y /F "../textures/y.dds" "unpacked/1B4F984/PC_KEY_1.dds"
xcopy /S /Q /Y /F "../textures/x.dds" "unpacked/1B4F984/PC_KEY_2.dds"
xcopy /S /Q /Y /F "../textures/rb.dds" "unpacked/1B4F984/PC_KEY_3.dds"
xcopy /S /Q /Y /F "../textures/lb.dds" "unpacked/1B4F984/PC_KEY_4.dds"
REM xcopy /S /Q /Y /F "../textures/dummy.dds" "unpacked/1B4F984/PC_KEY_5.dds"
xcopy /S /Q /Y /F "../textures/lt.dds" "unpacked/1B4F984/PC_KEY_9.dds"
REM xcopy /S /Q /Y /F "../textures/dummy.dds" "unpacked/1B4F984/PC_KEY_E.dds"
xcopy /S /Q /Y /F "../textures/rs.dds" "unpacked/1B4F984/PC_KEY_M.dds"
xcopy /S /Q /Y /F "../textures/quit.dds" "unpacked/1B4F984/PC_KEY_Q.dds"
REM xcopy /S /Q /Y /F "../textures/dummy.dds" "unpacked/1B4F984/PC_KEY_T.dds"
xcopy /S /Q /Y /F "../textures/ltrt.dds" "unpacked/1B4F984/PC_GREATERTHAN_LESSTHAN_KEYS.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/1B4F984/PC_KEY_SPC_SPANISH.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/1B4F984/PC_KEY_SPC_ITALIAN.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/1B4F984/PC_KEY_SPC_DANISH.dds"
xcopy /S /Q /Y /F "../textures/xdpadud.dds" "unpacked/1B4F984/PC_ARROW_KEYS_UP_DOWN.dds"
REM xcopy /S /Q /Y /F "../textures/dummy.dds" "unpacked/1B4F984/PC_HELP.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/1B4F984/PC_KEY_SPC_FRENCH.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/1B4F984/PC_KEY_SPC_GERMAN.dds"
xcopy /S /Q /Y /F "../textures/a.dds" "unpacked/1B4F984/PC_KEY_ENTER.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/1B4F984/PC_KEY_SPC_CZECH.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/1B4F984/PC_KEY_SPC_DUTCH.dds"
xcopy /S /Q /Y /F "../textures/b.dds" "unpacked/1B4F984/PC_KEY_ESC_FRENCH.dds"
%XNFSTPKTool% -w unpacked/1B4F984.ini ../../../data/NFSProStreet.GenericFix/scripts/buttons-xbox.tpk
rmdir /Q /S unpacked


rmdir /Q /S unpacked
md unpacked
%XNFSTPKTool% GLOBALB.BUN.000175.tpk.bin unpacked
xcopy /S /Q /Y /F "../textures/circle.dds" "unpacked/1B4F984/PC_KEY_ESC.dds"
xcopy /S /Q /Y /F "../textures/start.dds" "unpacked/1B4F984/PC_KEY_SPC.dds"
xcopy /S /Q /Y /F "../textures/r3.dds" "unpacked/1B4F984/PC_KEY_M_FRENCH.dds"
xcopy /S /Q /Y /F "../textures/r2.dds" "unpacked/1B4F984/PC_KEY_0.dds"
xcopy /S /Q /Y /F "../textures/triangle.dds" "unpacked/1B4F984/PC_KEY_1.dds"
xcopy /S /Q /Y /F "../textures/square.dds" "unpacked/1B4F984/PC_KEY_2.dds"
xcopy /S /Q /Y /F "../textures/r1.dds" "unpacked/1B4F984/PC_KEY_3.dds"
xcopy /S /Q /Y /F "../textures/l1.dds" "unpacked/1B4F984/PC_KEY_4.dds"
REM xcopy /S /Q /Y /F "../textures/dummy.dds" "unpacked/1B4F984/PC_KEY_5.dds"
xcopy /S /Q /Y /F "../textures/l2.dds" "unpacked/1B4F984/PC_KEY_9.dds"
REM xcopy /S /Q /Y /F "../textures/dummy.dds" "unpacked/1B4F984/PC_KEY_E.dds"
xcopy /S /Q /Y /F "../textures/r3.dds" "unpacked/1B4F984/PC_KEY_M.dds"
xcopy /S /Q /Y /F "../textures/quit.dds" "unpacked/1B4F984/PC_KEY_Q.dds"
REM xcopy /S /Q /Y /F "../textures/dummy.dds" "unpacked/1B4F984/PC_KEY_T.dds"
xcopy /S /Q /Y /F "../textures/l2r2.dds" "unpacked/1B4F984/PC_GREATERTHAN_LESSTHAN_KEYS.dds"
xcopy /S /Q /Y /F "../textures/start.dds" "unpacked/1B4F984/PC_KEY_SPC_SPANISH.dds"
xcopy /S /Q /Y /F "../textures/start.dds" "unpacked/1B4F984/PC_KEY_SPC_ITALIAN.dds"
xcopy /S /Q /Y /F "../textures/start.dds" "unpacked/1B4F984/PC_KEY_SPC_DANISH.dds"
xcopy /S /Q /Y /F "../textures/pdpadud.dds" "unpacked/1B4F984/PC_ARROW_KEYS_UP_DOWN.dds"
REM xcopy /S /Q /Y /F "../textures/dummy.dds" "unpacked/1B4F984/PC_HELP.dds"
xcopy /S /Q /Y /F "../textures/start.dds" "unpacked/1B4F984/PC_KEY_SPC_FRENCH.dds"
xcopy /S /Q /Y /F "../textures/start.dds" "unpacked/1B4F984/PC_KEY_SPC_GERMAN.dds"
xcopy /S /Q /Y /F "../textures/cross.dds" "unpacked/1B4F984/PC_KEY_ENTER.dds"
xcopy /S /Q /Y /F "../textures/start.dds" "unpacked/1B4F984/PC_KEY_SPC_CZECH.dds"
xcopy /S /Q /Y /F "../textures/start.dds" "unpacked/1B4F984/PC_KEY_SPC_DUTCH.dds"
xcopy /S /Q /Y /F "../textures/circle.dds" "unpacked/1B4F984/PC_KEY_ESC_FRENCH.dds"
%XNFSTPKTool% -w unpacked/1B4F984.ini ../../../data/NFSProStreet.GenericFix/scripts/buttons-playstation.tpk
rmdir /Q /S unpacked
