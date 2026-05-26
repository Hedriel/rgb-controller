#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD_BIN="$ROOT_DIR/build/rgb-controller"
TARGET_BIN="$HOME/.local/bin/rgb-controller"

if [ ! -x "$BUILD_BIN" ]; then
  echo "Build binary not found at $BUILD_BIN" >&2
  echo "Build first:" >&2
  echo "  mkdir -p build && cd build && cmake .. && make -j\$(nproc)" >&2
  exit 1
fi

mkdir -p "$HOME/.local/bin"

cp "$BUILD_BIN" "$TARGET_BIN"
chmod +x "$TARGET_BIN"

echo
echo "Installed: $TARGET_BIN"
echo
printf '%s\n' "Next steps:" \
  "  sudo cp \"$ROOT_DIR/dist/99-rgb-controller.rules\" /etc/udev/rules.d/" \
  "  sudo udevadm control --reload-rules" \
  "  sudo udevadm trigger"
