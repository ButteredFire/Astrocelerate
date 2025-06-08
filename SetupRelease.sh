#!/bin/bash
clear

python "scripts/GenerateDirectories.py" "src" "external" "assets"
python "scripts/Bootstrap.py" configure Release
python "scripts/Bootstrap.py" build Release

read -p "Press any key to continue..."