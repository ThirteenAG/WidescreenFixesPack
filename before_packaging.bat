for /f "tokens=1,* delims=:" %%A in ('curl -ks https://api.github.com/repos/ThirteenAG/Ultimate-ASI-Loader/releases/latest ^| find "browser_download_url"') do (
    curl -kOL %%B
)

for /f "tokens=1,* delims=:" %%A in ('curl -ks https://api.github.com/repos/ThirteenAG/d3d8-wrapper/releases/latest ^| find "browser_download_url"') do (
    curl -kOL %%B
)

for /f "tokens=1,* delims=:" %%A in ('curl -ks https://api.github.com/repos/ThirteenAG/d3d9-wrapper/releases/latest ^| find "browser_download_url"') do (
    curl -kOL %%B
)

for /f "tokens=1,* delims=:" %%A in ('curl -ks https://api.github.com/repos/dege-diosg/dgVoodoo2/releases/latest ^| find "browser_download_url"') do (
  echo.%%B | FIND /I "_dbg">Nul || ( 
    curl -o dgVoodoo2.zip -kL %%B
  )
)

for /f "tokens=1,* delims=:" %%A in ('curl -ks https://api.github.com/repos/ThirteenAG/Xidi/releases/latest ^| find "browser_download_url"') do (
    curl -o xidi.zip -kL %%B
)