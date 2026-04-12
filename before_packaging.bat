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
  echo.%%B | FIND /I "dxwrapper">Nul && echo.%%B | FIND /I "debug">Nul || (
    curl -o dxwrapper.zip -kL %%B
  )
)

for /f "tokens=1,* delims=:" %%A in ('curl -ks https://api.github.com/repos/ThirteenAG/dxwrapper/releases/latest ^| find "browser_download_url"') do (
  echo.%%B | FIND /I "dxwrapper">Nul && echo.%%B | FIND /I "debug">Nul || (
    curl -o dxwrapper-scda.zip -kL %%B
  )
)

curl -o dgVoodoo2.zip -kL "https://github.com/dege-diosg/dgVoodoo2/releases/download/v2.86.5/dgVoodoo2_86_5.zip"

for /f "tokens=1,* delims=:" %%A in ('curl -ks https://api.github.com/repos/ThirteenAG/Xidi/releases/latest ^| find "browser_download_url"') do (
    curl -o xidi.zip -kL %%B
)

for /f "tokens=1,* delims=:" %%A in ('curl -ks https://api.github.com/repos/kcat/dsoal/releases/tags/latest-master ^| find "browser_download_url"') do (
  echo.%%B | FIND /I ".zip">Nul || (
    curl -o DSOAL.7z -kL %%B
  )
)
