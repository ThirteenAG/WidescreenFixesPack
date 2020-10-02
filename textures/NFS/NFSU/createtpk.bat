set "XNFSTPKTool=%cd%\..\XNFSTPKTool.exe"

rmdir /Q /S unpacked
md unpacked
%XNFSTPKTool% -2 GLOBALB.BUN.000140.tpk.bin unpacked
%XNFSTPKTool% -2 FrontB.bun.000002.tpk.bin unpacked
xcopy /S /Q /Y /F "textures/a.dds" "unpacked/8C37F6B1/PC_X.dds"
xcopy /S /Q /Y /F "textures/view.dds" "unpacked/8C37F6B1/PC_CUSTOM.dds"
xcopy /S /Q /Y /F "textures/b.dds" "unpacked/8C37F6B1/PC_BACK.dds"
xcopy /S /Q /Y /F "textures/quit.dds" "unpacked/8C37F6B1/PC_QUIT.dds"
%XNFSTPKTool% -w2 unpacked/8C37F6B1.ini unpacked/temp1.tpk
xcopy /S /Q /Y /F "textures/a.dds" "unpacked/A8AD59C0/PC_X.dds"
xcopy /S /Q /Y /F "textures/rs.dds" "unpacked/A8AD59C0/PC_DELETE.dds"
xcopy /S /Q /Y /F "textures/x.dds" "unpacked/A8AD59C0/PC_PERFORMANCE.dds"
xcopy /S /Q /Y /F "textures/ls.dds" "unpacked/A8AD59C0/PC_TUTORIAL.dds"
xcopy /S /Q /Y /F "textures/view.dds" "unpacked/A8AD59C0/PC_CUSTOM.dds"
xcopy /S /Q /Y /F "textures/b.dds" "unpacked/A8AD59C0/PC_BACK.dds"
xcopy /S /Q /Y /F "textures/y.dds" "unpacked/A8AD59C0/PC_CREATEROOM.dds"
%XNFSTPKTool% -w2 unpacked/A8AD59C0.ini unpacked/temp2.tpk
cd unpacked
copy /b temp1.tpk+temp2.tpk buttons-xbox.tpk
cd ..
xcopy /S /Q /Y /F "unpacked/buttons-xbox.tpk" "../../../data/NFSUnderground.WidescreenFix/scripts/buttons-xbox.tpk"
rmdir /Q /S unpacked


rmdir /Q /S unpacked
md unpacked
%XNFSTPKTool% -2 GLOBALB.BUN.000140.tpk.bin unpacked
%XNFSTPKTool% -2 FrontB.bun.000002.tpk.bin unpacked
xcopy /S /Q /Y /F "textures/cross.dds" "unpacked/8C37F6B1/PC_X.dds"
xcopy /S /Q /Y /F "textures/share.dds" "unpacked/8C37F6B1/PC_CUSTOM.dds"
xcopy /S /Q /Y /F "textures/circle.dds" "unpacked/8C37F6B1/PC_BACK.dds"
xcopy /S /Q /Y /F "textures/quit.dds" "unpacked/8C37F6B1/PC_QUIT.dds"
%XNFSTPKTool% -w2 unpacked/8C37F6B1.ini unpacked/temp1.tpk
xcopy /S /Q /Y /F "textures/cross.dds" "unpacked/A8AD59C0/PC_X.dds"
xcopy /S /Q /Y /F "textures/r3.dds" "unpacked/A8AD59C0/PC_DELETE.dds"
xcopy /S /Q /Y /F "textures/square.dds" "unpacked/A8AD59C0/PC_PERFORMANCE.dds"
xcopy /S /Q /Y /F "textures/l3.dds" "unpacked/A8AD59C0/PC_TUTORIAL.dds"
xcopy /S /Q /Y /F "textures/share.dds" "unpacked/A8AD59C0/PC_CUSTOM.dds"
xcopy /S /Q /Y /F "textures/circle.dds" "unpacked/A8AD59C0/PC_BACK.dds"
xcopy /S /Q /Y /F "textures/triangle.dds" "unpacked/A8AD59C0/PC_CREATEROOM.dds"
%XNFSTPKTool% -w2 unpacked/A8AD59C0.ini unpacked/temp2.tpk
cd unpacked
copy /b temp1.tpk+temp2.tpk buttons-playstation.tpk
cd ..
xcopy /S /Q /Y /F "unpacked/buttons-playstation.tpk" "../../../data/NFSUnderground.WidescreenFix/scripts/buttons-playstation.tpk"
rmdir /Q /S unpacked