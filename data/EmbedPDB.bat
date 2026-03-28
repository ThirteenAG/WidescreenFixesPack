@echo off
setlocal enabledelayedexpansion
set MAX=8

for /r "." %%F in (*.asi) do (
  call :wait-for-slots
  start "" /B cmd /c ""%~dp0EmbedPDB.exe" "%%F""
)

call :wait-all
exit /b

:wait-for-slots
set running=0
for /f "tokens=*" %%A in ('tasklist /fi "imagename eq cmd.exe" /v /fo csv ^| find /i "EmbedPDB.exe"') do set /a running+=1
if %running% geq %MAX% (
  timeout /t 1 >nul
  goto wait-for-slots
)
exit /b

:wait-all
set running=0
for /f "tokens=*" %%A in ('tasklist /fi "imagename eq cmd.exe" /v /fo csv ^| find /i "EmbedPDB.exe"') do set /a running+=1
if %running% gtr 0 (
  timeout /t 1 >nul
  goto wait-all
)
exit /b