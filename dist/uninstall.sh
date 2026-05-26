#!/bin/sh
set -eu

TARGET_BIN="$HOME/.local/bin/rgb-controller"
TARGET_LAUNCHER="$HOME/.local/bin/rgb-controller-launcher"
TARGET_SERVICE="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user/rgb-controller.service"

systemctl --user disable --now rgb-controller.service >/dev/null 2>&1 || true
rm -f "$TARGET_BIN" "$TARGET_LAUNCHER" "$TARGET_SERVICE"
systemctl --user daemon-reload || true

echo "Removed installed binary, launcher, and user service."
echo "Config file was kept in ${XDG_CONFIG_HOME:-$HOME/.config}/rgb-controller/config.ini"
