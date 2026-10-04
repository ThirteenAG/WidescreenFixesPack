@echo off
setlocal
set "GITHUB_AUTH="
if defined GH_TOKEN set "GITHUB_AUTH=Authorization: Bearer %GH_TOKEN%"

for /f "tokens=1,* delims=:" %%A in ('curl --fail --retry 3 -sS -H "%GITHUB_AUTH%" https://api.github.com/repos/ThirteenAG/Ultimate-ASI-Loader/releases/latest ^| find "browser_download_url"') do (
    curl --fail --retry 3 -OL %%B
    if errorlevel 1 goto download_failed
)

for /f "tokens=1,* delims=:" %%A in ('curl --fail --retry 3 -sS -H "%GITHUB_AUTH%" https://api.github.com/repos/ThirteenAG/d3d8-wrapper/releases/latest ^| find "browser_download_url"') do (
    curl --fail --retry 3 -OL %%B
    if errorlevel 1 goto download_failed
)

for /f "tokens=1,* delims=:" %%A in ('curl --fail --retry 3 -sS -H "%GITHUB_AUTH%" https://api.github.com/repos/ThirteenAG/d3d9-wrapper/releases/latest ^| find "browser_download_url"') do (
    curl --fail --retry 3 -OL %%B
    if errorlevel 1 goto download_failed
)

for /f "tokens=1,* delims=:" %%A in ('curl --fail --retry 3 -sS -H "%GITHUB_AUTH%" https://api.github.com/repos/elishacloud/dxwrapper/releases/latest ^| find "browser_download_url"') do (
  echo.%%B | FIND /I "/dxwrapper.zip">Nul && (
    curl -o dxwrapper.zip --fail --retry 3 -L %%B
    if errorlevel 1 goto download_failed
  )
)

for /f "tokens=1,* delims=:" %%A in ('curl --fail --retry 3 -sS -H "%GITHUB_AUTH%" https://api.github.com/repos/ThirteenAG/dxwrapper/releases/latest ^| find "browser_download_url"') do (
  echo.%%B | FIND /I "/dxwrapper-scda.zip">Nul && (
    curl -o dxwrapper-scda.zip --fail --retry 3 -L %%B
    if errorlevel 1 goto download_failed
  )
)

for /f "tokens=1,* delims=:" %%A in ('curl --fail --retry 3 -sS -H "%GITHUB_AUTH%" https://api.github.com/repos/dege-diosg/dgVoodoo2/releases/latest ^| find "browser_download_url"') do (
  echo.%%B | find /i "dgVoodoo2_" >nul && (
    echo.%%B | find /i "_dbg" >nul || echo.%%B | find /i "_dev64" >nul || echo.%%B | find /i "API" >nul || (
      curl -o dgVoodoo2.zip --fail --retry 3 -L %%B
      if errorlevel 1 goto download_failed
    )
  )
)

for /f "tokens=1,* delims=:" %%A in ('curl --fail --retry 3 -sS -H "%GITHUB_AUTH%" https://api.github.com/repos/ThirteenAG/Xidi/releases/latest ^| find "browser_download_url"') do (
    curl -o xidi.zip --fail --retry 3 -L %%B
    if errorlevel 1 goto download_failed
)

rem DSOAL - the latest-master release is gone, upstream now publishes builds in the "archive" release,
rem where every asset is a zip containing another zip with the actual files
rem note: %%B keeps the quotes from the json, cmd strips them when the url reaches curl
set "DSOAL_URL="
for /f "tokens=1,* delims=:" %%A in ('curl --fail --retry 3 -sS -H "%GITHUB_AUTH%" https://api.github.com/repos/kcat/dsoal/releases/tags/archive ^| find "browser_download_url"') do (
  echo.%%B | FIND /I "/DSOAL_r">Nul && set "DSOAL_URL=%%B"
)
if not defined DSOAL_URL set "DSOAL_URL=https://github.com/kcat/dsoal/releases/download/archive/DSOAL_r695.zip"

del DSOAL-outer.zip 2>nul
curl --fail --retry 3 -o DSOAL-outer.zip -L %DSOAL_URL%
if errorlevel 1 goto download_failed
if not exist DSOAL-outer.zip (
  echo Failed to download DSOAL & goto download_failed
)

rmdir /s /q "DSOAL-tmp" 2>nul
data\7za.exe x DSOAL-outer.zip -y -o"DSOAL-tmp" >nul
for %%F in ("DSOAL-tmp\*.zip") do move /y "%%~F" "DSOAL.zip" >nul
del DSOAL-outer.zip
rmdir /s /q "DSOAL-tmp"

if not exist DSOAL.zip (
  echo Failed to extract DSOAL & goto download_failed
)

rem A failed API request can leave a FOR loop with no downloads at all.
for %%F in (Ultimate-ASI-Loader.zip Ultimate-ASI-Loader_x64.zip dxwrapper.zip dxwrapper-scda.zip dgVoodoo2.zip xidi.zip DSOAL.zip) do (
  data\7za.exe t "%%F" >nul
  if errorlevel 1 (
    echo ERROR: Missing or invalid dependency archive: %%F
    goto download_failed
  )
)

exit /b 0

:download_failed
echo ERROR: Dependency download or validation failed.
exit /b 1
