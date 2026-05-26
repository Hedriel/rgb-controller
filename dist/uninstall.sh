#!/bin/sh
set -eu

TARGET_BIN="$HOME/.local/bin/rgb-controller"

rm -f "$TARGET_BIN"

echo "Removed: $TARGET_BIN"
echo "Config file was kept in ${XDG_CONFIG_HOME:-$HOME/.config}/rgb-controller/config.ini"
