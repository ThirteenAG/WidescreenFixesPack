cd ..

git clone https://github.com/ThirteenAG/Ultimate-ASI-Loader --recursive
cd Ultimate-ASI-Loader
premake5.exe vs2022
msbuild.exe build/Ultimate-ASI-Loader-x86.sln /t:Build /p:Configuration=Release;Platform=Win32
msbuild.exe build/Ultimate-ASI-Loader-x64.sln /t:Build /p:Configuration=Release;Platform=x64
cd ..

git clone https://github.com/ThirteenAG/d3d8-wrapper --recursive
cd d3d8-wrapper
premake5.exe vs2022
msbuild.exe build/d3d8-wrapper.sln /t:Build /p:Configuration=Release;Platform=Win32
cd ..

git clone https://github.com/ThirteenAG/d3d9-wrapper --recursive
cd d3d9-wrapper
premake5.exe vs2022
msbuild.exe build/d3d9-wrapper.sln /t:Build /p:Configuration=Release;Platform=Win32
cd ..
