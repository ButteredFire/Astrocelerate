#!/bin/bash
clear

python "scripts/GenerateDirectories.py" "src" "external" "assets"
python "scripts/Bootstrap.py" configure Debug
python "scripts/Bootstrap.py" build Debug

read -p "Press any key to continue..."