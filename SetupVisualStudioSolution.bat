echo off
cls
python "scripts/GenerateDirectories.py" "src" "external" "assets"

cmake -S . -B . -G "Visual Studio 17 2022"
cmake --build .

pause