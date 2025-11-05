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

for /f "tokens=1,* delims=:" %%A in ('curl -ks https://api.github.com/repos/dege-diosg/dgVoodoo2/releases/latest ^| find "browser_download_url"') do (
  echo.%%B | FIND /I "_dbg">Nul || echo.%%B | FIND /I "_dev64">Nul || echo.%%B | FIND /I "API">Nul || ( 
    curl -o dgVoodoo2.zip -kL %%B
  )
)

for /f "tokens=1,* delims=:" %%A in ('curl -ks https://api.github.com/repos/ThirteenAG/Xidi/releases/latest ^| find "browser_download_url"') do (
    curl -o xidi.zip -kL %%B
)