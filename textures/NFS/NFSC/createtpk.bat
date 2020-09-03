set "XNFSTPKTool=%cd%\..\XNFSTPKTool.exe"

rmdir /Q /S unpacked
md unpacked
%XNFSTPKTool% GLOBALB.BUN.000164.tpk.bin unpacked
xcopy /S /Q /Y /F "../textures/a.dds" "unpacked/D444E031/PC_KEY_ENTER.dds"
xcopy /S /Q /Y /F "../textures/b.dds" "unpacked/D444E031/PC_KEY_ESC.dds"
xcopy /S /Q /Y /F "../textures/b.dds" "unpacked/D444E031/PC_KEY_ESC_FRENCH.dds"
xcopy /S /Q /Y /F "../textures/quit.dds" "unpacked/D444E031/PC_KEY_Q.dds"
xcopy /S /Q /Y /F "../textures/lb.dds" "unpacked/D444E031/L1.dds"
xcopy /S /Q /Y /F "../textures/lb.dds" "unpacked/D444E031/PC_KEY_9.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/D444E031/PC_KEY_SPC.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/D444E031/PC_KEY_SPC_CZECH.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/D444E031/PC_KEY_SPC_DANISH.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/D444E031/PC_KEY_SPC_DUTCH.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/D444E031/PC_KEY_SPC_FRENCH.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/D444E031/PC_KEY_SPC_GERMAN.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/D444E031/PC_KEY_SPC_ITALIAN.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/D444E031/PC_KEY_SPC_SPANISH.dds"
xcopy /S /Q /Y /F "../textures/menu.dds" "unpacked/D444E031/PC_X.dds"
xcopy /S /Q /Y /F "../textures/rb.dds" "unpacked/D444E031/PC_KEY_0.dds"
xcopy /S /Q /Y /F "../textures/rb.dds" "unpacked/D444E031/R1.dds"
xcopy /S /Q /Y /F "../textures/view.dds" "unpacked/D444E031/PC_KEY_3.dds"
xcopy /S /Q /Y /F "../textures/x.dds" "unpacked/D444E031/PC_KEY_2.dds"
xcopy /S /Q /Y /F "../textures/y.dds" "unpacked/D444E031/PC_KEY_1.dds"
xcopy /S /Q /Y /F "../textures/cursor.dds" "unpacked/D444E031/PC_CURSOR.dds"
xcopy /S /Q /Y /F "../textures/dummy.dds" "unpacked/D444E031/PC_LOAD.dds"
%XNFSTPKTool% -w unpacked/D444E031.ini ../../../data/NFSCarbon.WidescreenFix/scripts/buttons-xbox.tpk
rmdir /Q /S unpacked


md unpacked
%XNFSTPKTool% GLOBALB.BUN.000164.tpk.bin unpacked
xcopy /S /Q /Y /F "../textures/cross.dds" "unpacked/D444E031/PC_KEY_ENTER.dds"
xcopy /S /Q /Y /F "../textures/circle.dds" "unpacked/D444E031/PC_KEY_ESC.dds"
xcopy /S /Q /Y /F "../textures/circle.dds" "unpacked/D444E031/PC_KEY_ESC_FRENCH.dds"
xcopy /S /Q /Y /F "../textures/quit.dds" "unpacked/D444E031/PC_KEY_Q.dds"
xcopy /S /Q /Y /F "../textures/l1.dds" "unpacked/D444E031/L1.dds"
xcopy /S /Q /Y /F "../textures/l1.dds" "unpacked/D444E031/PC_KEY_9.dds"
xcopy /S /Q /Y /F "../textures/options.dds" "unpacked/D444E031/PC_KEY_SPC.dds"
xcopy /S /Q /Y /F "../textures/options.dds" "unpacked/D444E031/PC_KEY_SPC_CZECH.dds"
xcopy /S /Q /Y /F "../textures/options.dds" "unpacked/D444E031/PC_KEY_SPC_DANISH.dds"
xcopy /S /Q /Y /F "../textures/options.dds" "unpacked/D444E031/PC_KEY_SPC_DUTCH.dds"
xcopy /S /Q /Y /F "../textures/options.dds" "unpacked/D444E031/PC_KEY_SPC_FRENCH.dds"
xcopy /S /Q /Y /F "../textures/options.dds" "unpacked/D444E031/PC_KEY_SPC_GERMAN.dds"
xcopy /S /Q /Y /F "../textures/options.dds" "unpacked/D444E031/PC_KEY_SPC_ITALIAN.dds"
xcopy /S /Q /Y /F "../textures/options.dds" "unpacked/D444E031/PC_KEY_SPC_SPANISH.dds"
xcopy /S /Q /Y /F "../textures/options.dds" "unpacked/D444E031/PC_X.dds"
xcopy /S /Q /Y /F "../textures/r1.dds" "unpacked/D444E031/PC_KEY_0.dds"
xcopy /S /Q /Y /F "../textures/r1.dds" "unpacked/D444E031/R1.dds"
xcopy /S /Q /Y /F "../textures/share.dds" "unpacked/D444E031/PC_KEY_3.dds"
xcopy /S /Q /Y /F "../textures/square.dds" "unpacked/D444E031/PC_KEY_2.dds"
xcopy /S /Q /Y /F "../textures/triangle.dds" "unpacked/D444E031/PC_KEY_1.dds"
xcopy /S /Q /Y /F "../textures/cursor.dds" "unpacked/D444E031/PC_CURSOR.dds"
xcopy /S /Q /Y /F "../textures/dummy.dds" "unpacked/D444E031/PC_LOAD.dds"
%XNFSTPKTool% -w unpacked/D444E031.ini ../../../data/NFSCarbon.WidescreenFix/scripts/buttons-playstation.tpk
rmdir /Q /S unpacked
