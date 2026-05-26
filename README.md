# rgb-controller

A native Linux GUI for controlling **ROBOBLOQ / DX-LIGHT RGB Controller** USB LED strips.

`rgb-controller` provides a simple GTK3 interface to select any color and brightness for your LED strip, with automatic device detection and persistent configuration.

---

## Features

- **Color spectrum picker** - Select any color using an intuitive rainbow spectrum interface
- **Brightness control** - Adjust LED intensity from 0 to 255
- **Direct HID communication** - No intermediate services or daemons
- **Automatic device detection** - Finds your RGB Controller automatically
- **Persistent configuration** - Remembers your last color selection
- **Native GTK3 GUI** - Lightweight, no Electron or browser dependencies

---

## Supported hardware

- **Vendor ID:** `0x1A86`
- **Product ID:** `0xFE07`
- **Manufacturer:** `ROBOBLOQ`
- **Product:** `USBHID`
- **LED count:** 65 LEDs (31 top + 17 right + 17 left)

To use with a different device, modify the VID/PID in `src/gui.cpp`.

---

## Dependencies

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake pkgconf hidapi gtkmm3
```

**Ubuntu/Debian:**
```bash
sudo apt install build-essential cmake pkg-config libhidapi-dev libgtkmm-3.0-dev
```

---

## Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

---

## Installation

```bash
./dist/install.sh
```

Then install the udev rule for device permissions:

```bash
sudo cp dist/99-rgb-controller.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Unplug and replug your USB controller.

---

## Uninstallation

```bash
./dist/uninstall.sh
```

---

## Usage

Run the application:

```bash
rgb-controller
```

The GUI will:
1. Detect your RGB Controller device
2. Show a color spectrum picker
3. Apply your color selection immediately to the LED strip
4. Save the color to `~/.config/rgb-controller/config.ini`

---

## Configuration

Color and brightness are saved automatically to:

```text
~/.config/rgb-controller/config.ini
```

Example:
```ini
static_color=FF5500
```

The color is stored as a 6-digit hex RGB value.

---

## Troubleshooting

### Device not found or permission denied

Check device permissions:
```bash
ls -l /dev/hidraw*
```

Ensure the udev rule is installed and reloaded:
```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### Device path changes after reconnecting

This is normal on Linux. The application uses automatic detection by VID/PID, so it will find your device regardless of the `/dev/hidrawX` path.

### Application crashes on startup

Check that:
- The USB device is connected
- The udev rule is installed
- You have permission to access HID devices

---

## Project structure

```text
.
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp                    # Application entry point
│   ├── gui.cpp/hpp                 # GTK3 GUI implementation
│   ├── rgb_controller_hid.cpp/hpp  # HID device communication
│   └── rgb_controller_layout.hpp   # LED strip layout (65 LEDs)
└── dist/
    ├── install.sh                  # Installation script
    ├── uninstall.sh                # Uninstallation script
    └── 99-rgb-controller.rules     # udev rule for device permissions
```

---

## License

GPL-3.0

---
