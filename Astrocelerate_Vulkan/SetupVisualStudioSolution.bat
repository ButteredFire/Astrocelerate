echo off
cls
python "scripts/GenerateDirectories.py" "HeaderDirs.cmake" "SourceFiles.cmake" "src" "external"

cmake -S . -B . -G "Visual Studio 17 2022"

pause