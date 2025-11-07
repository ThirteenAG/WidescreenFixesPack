cd textures/NFS/NFSUC
call createtpk.bat
cd ../../..

cd textures/NFS/NFSC
call createtpk.bat
cd ../../..

cd textures/NFS/NFSMW
call createtpk.bat
cd ../../..

cd textures/NFS/NFSU2
call createtpk.bat
cd ../../..

cd textures/NFS/NFSU
call createtpk.bat
cd ../../..

cd textures/GTAVCS
call buildps2.bat
cd ../..

if "%SCCOMPILE_PASSWORD%"=="" (
    echo SCCOMPILE_PASSWORD is empty, skipping sccompile.
) else (
    cd source/SplinterCell.WidescreenFix
    call sccompile.bat %SCCOMPILE_PASSWORD% EchelonHUD ../../build/sccompile
    cd ../..
    mkdir "data\SplinterCell.WidescreenFix\system\update" 2>nul
    copy /y "build\sccompile\System\EchelonHUD.u" "data\SplinterCell.WidescreenFix\system\update\EchelonHUD.u"
)
