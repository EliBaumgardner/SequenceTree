#!/usr/bin/env bash
CH="/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"
html="$1"; out="$2"; w="$3"; h="$4"
"$CH" --headless=new --disable-gpu --hide-scrollbars --force-device-scale-factor=2 --default-background-color=00000000 --allow-file-access-from-files --window-size="$w,$h" --virtual-time-budget=3000 --screenshot="$out" "file://$html" >/dev/null 2>&1
