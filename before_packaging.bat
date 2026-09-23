for /f "tokens=1,* delims=:" %%A in ('curl -ks https://api.github.com/repos/ThirteenAG/Ultimate-ASI-Loader/releases/latest ^| find "browser_download_url"') do (
    curl -kOL %%B
)

for /f "tokens=1,* delims=:" %%A in ('curl -ks https://api.github.com/repos/ThirteenAG/d3d8-wrapper/releases/latest ^| find "browser_download_url"') do (
    curl -kOL %%B
)

for /f "tokens=1,* delims=:" %%A in ('curl -ks https://api.github.com/repos/ThirteenAG/d3d9-wrapper/releases/latest ^| find "browser_download_url"') do (
    curl -kOL %%B
)

for /f "tokens=1,* delims=:" %%A in ('curl -ks https://api.github.com/repos/elishacloud/dxwrapper/releases/latest ^| find "browser_download_url"') do (
  echo.%%B | FIND /I "/dxwrapper.zip">Nul && (
    curl -o dxwrapper.zip -kL %%B
  )
)

for /f "tokens=1,* delims=:" %%A in ('curl -ks https://api.github.com/repos/ThirteenAG/dxwrapper/releases/latest ^| find "browser_download_url"') do (
  echo.%%B | FIND /I "/dxwrapper-scda.zip">Nul && (
    curl -o dxwrapper-scda.zip -kL %%B
  )
)

for /f "tokens=1,* delims=:" %%A in ('curl -ks https://api.github.com/repos/dege-diosg/dgVoodoo2/releases/latest ^| find "browser_download_url"') do (
  echo.%%B | find /i "dgVoodoo2_" >nul && (
    echo.%%B | find /i "_dbg" >nul || echo.%%B | find /i "_dev64" >nul || echo.%%B | find /i "API" >nul || (
      curl -o dgVoodoo2.zip -kL %%B
    )
  )
)

for /f "tokens=1,* delims=:" %%A in ('curl -ks https://api.github.com/repos/ThirteenAG/Xidi/releases/latest ^| find "browser_download_url"') do (
    curl -o xidi.zip -kL %%B
)

rem DSOAL - the latest-master release is gone, upstream now publishes builds in the "archive" release,
rem where every asset is a zip containing another zip with the actual files
rem note: %%B keeps the quotes from the json, cmd strips them when the url reaches curl
set "DSOAL_URL="
for /f "tokens=1,* delims=:" %%A in ('curl -ks https://api.github.com/repos/kcat/dsoal/releases/tags/archive ^| find "browser_download_url"') do (
  echo.%%B | FIND /I "/DSOAL_r">Nul && set "DSOAL_URL=%%B"
)
if not defined DSOAL_URL set "DSOAL_URL=https://github.com/kcat/dsoal/releases/download/archive/DSOAL_r695.zip"

del DSOAL-outer.zip 2>nul
curl -o DSOAL-outer.zip -kL %DSOAL_URL%
if not exist DSOAL-outer.zip (
  echo Failed to download DSOAL & exit /b 1
)

rmdir /s /q "DSOAL-tmp" 2>nul
7za x DSOAL-outer.zip -y -o"DSOAL-tmp" >nul
for %%F in ("DSOAL-tmp\*.zip") do move /y "%%~F" "DSOAL.zip" >nul
del DSOAL-outer.zip
rmdir /s /q "DSOAL-tmp"

if not exist DSOAL.zip (
  echo Failed to extract DSOAL & exit /b 1
)

exit /b 0