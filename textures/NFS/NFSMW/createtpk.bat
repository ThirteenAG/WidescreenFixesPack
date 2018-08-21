set "XNFSTPKTool=%cd%\..\XNFSTPKTool.exe"

rmdir /Q /S unpacked
md unpacked
%XNFSTPKTool% -2 GLOBALB.BUN.000158.tpk.bin unpacked
xcopy /S /Q /Y /F "../textures/lb.dds" "unpacked/8C37F6B1/L1.dds"
xcopy /S /Q /Y /F "../textures/quit.dds" "unpacked/8C37F6B1/PC_KEY_Q.dds"
xcopy /S /Q /Y /F "../textures/lbrb.dds" "unpacked/8C37F6B1/PC_GREATERTHAN_LESSTHAN.dds"
xcopy /S /Q /Y /F "../textures/rb.dds" "unpacked/8C37F6B1/R1.dds"
xcopy /S /Q /Y /F "../textures/a.dds" "unpacked/8C37F6B1/PC_KEY_ENTER.dds"
xcopy /S /Q /Y /F "../textures/b.dds" "unpacked/8C37F6B1/PC_KEY_ESC_FRENCH.dds"
xcopy /S /Q /Y /F "../textures/b.dds" "unpacked/8C37F6B1/PC_KEY_ESC.dds"
xcopy /S /Q /Y /F "../textures/y.dds" "unpacked/8C37F6B1/PC_KEY_1.dds"
xcopy /S /Q /Y /F "../textures/x.dds" "unpacked/8C37F6B1/PC_KEY_2.dds"
xcopy /S /Q /Y /F "../textures/view.dds" "unpacked/8C37F6B1/PC_KEY_3.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/8C37F6B1/PC_KEY_4.dds"
xcopy /S /Q /Y /F "../textures/dummy.dds" "unpacked/8C37F6B1/PC_LOAD.dds"
%XNFSTPKTool% -w2 unpacked/8C37F6B1.ini ../../../data/NFSMostWanted.WidescreenFix/scripts/buttons-xbox.tpk
rmdir /Q /S unpacked


md unpacked
%XNFSTPKTool% -2 GLOBALB.BUN.000158.tpk.bin unpacked
xcopy /S /Q /Y /F "../textures/l1.dds" "unpacked/8C37F6B1/L1.dds"
xcopy /S /Q /Y /F "../textures/quit.dds" "unpacked/8C37F6B1/PC_KEY_Q.dds"
xcopy /S /Q /Y /F "../textures/l1r1.dds" "unpacked/8C37F6B1/PC_GREATERTHAN_LESSTHAN.dds"
xcopy /S /Q /Y /F "../textures/r1.dds" "unpacked/8C37F6B1/R1.dds"
xcopy /S /Q /Y /F "../textures/cross.dds" "unpacked/8C37F6B1/PC_KEY_ENTER.dds"
xcopy /S /Q /Y /F "../textures/circle.dds" "unpacked/8C37F6B1/PC_KEY_ESC_FRENCH.dds"
xcopy /S /Q /Y /F "../textures/circle.dds" "unpacked/8C37F6B1/PC_KEY_ESC.dds"
xcopy /S /Q /Y /F "../textures/triangle.dds" "unpacked/8C37F6B1/PC_KEY_1.dds"
xcopy /S /Q /Y /F "../textures/square.dds" "unpacked/8C37F6B1/PC_KEY_2.dds"
xcopy /S /Q /Y /F "../textures/select.dds" "unpacked/8C37F6B1/PC_KEY_3.dds"
xcopy /S /Q /Y /F "../textures/start.dds" "unpacked/8C37F6B1/PC_KEY_4.dds"
xcopy /S /Q /Y /F "../textures/dummy.dds" "unpacked/8C37F6B1/PC_LOAD.dds"
%XNFSTPKTool% -w2 unpacked/8C37F6B1.ini ../../../data/NFSMostWanted.WidescreenFix/scripts/buttons-playstation.tpk
rmdir /Q /S unpacked
