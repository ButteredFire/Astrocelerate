echo off
cls

python "scripts/GenerateDirectories.py" "src" "external" "assets"
python "scripts/Bootstrap.py" configure Release
python "scripts/Bootstrap.py" build Release

pause