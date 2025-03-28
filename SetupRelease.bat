echo off
cls
python "scripts/GenerateDirectories.py" "src" "external" "assets"

mkdir build

cmake . -G "Visual Studio 17 2022"
cmake --build build --config Release

pause